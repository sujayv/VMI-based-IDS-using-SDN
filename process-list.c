/* The LibVMI Library is an introspection library that simplifies access to 
 * memory in a target virtual machine or in a file containing a dump of 
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Bryan D. Payne (bdpayne@acm.org)
 *
 * This file is part of LibVMI.
 *
 * LibVMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * LibVMI is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LibVMI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>

#include <libvmi/libvmi.h>

int main (int argc, char **argv)
{
    vmi_instance_t vmi;
    unsigned char *memory = NULL;
    uint32_t offset;
    addr_t list_head = 0, next_list_entry = 0, list_head1 = 0,session_listhead = 0;
    addr_t current_process = 0;
    addr_t tmp_next = 0;
    addr_t next_module = 0;
    addr_t eprocess_head = 0;
    addr_t peb_listhead =  0;
    char *procname = NULL;
    vmi_pid_t pid = 0;
    unsigned long initorder = 0,peb_offset = 0,tasks_offset = 0, pid_offset = 0, name_offset = 0, handletable_offset = 0,ht_offset = 0,ht_pid = 0,eprocess_offset = 0;
    unsigned long session_offset = 0,peb_ldr_offset = 0,taskrun_offset = 0;
    status_t status;

    /* this is the VM or file that we are looking at */
    if (argc != 2) {
        printf("Usage: %s <vmname>\n", argv[0]);
        return 1;
    }

    char *name = argv[1];

    /* initialize the libvmi library */
    if (vmi_init(&vmi, VMI_AUTO | VMI_INIT_COMPLETE, name) == VMI_FAILURE) {
        printf("Failed to init LibVMI library.\n");
        return 1;
    }

    /* init the offset values */
    if (VMI_OS_LINUX == vmi_get_ostype(vmi)) {
        tasks_offset = vmi_get_offset(vmi, "linux_tasks");
        name_offset = vmi_get_offset(vmi, "linux_name");
        pid_offset = vmi_get_offset(vmi, "linux_pid");
        taskrun_offset = 0x1e8;
    }
    else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) {
        tasks_offset = vmi_get_offset(vmi, "win_tasks");
        name_offset = vmi_get_offset(vmi, "win_pname");
        pid_offset = vmi_get_offset(vmi, "win_pid");
        ht_pid = 0x008;
        ht_offset = 0x010;
        eprocess_offset = 0x004;
        printf("tasks_offset is %lu\n",tasks_offset);
        printf("name_offset is %lu\n",name_offset);
        printf("pid_offset is %lu\n",pid_offset);
        handletable_offset = 0x0f4;
        session_offset = 0x0e4;
        peb_offset = 0x1a8;
        peb_ldr_offset = 0x00c;
        initorder = 0x01c;
        //printf("handletable_offset is %lu\n",handletable_offset);
        //printf("sessionprocesslinks_offset is %lu\n",session_offset);
    }

    if (0 == tasks_offset) {
        printf("Failed to find win_tasks\n");
        goto error_exit;
    }
    if (0 == pid_offset) {
        printf("Failed to find win_pid\n");
        goto error_exit;
    }
    if (0 == name_offset) {
        printf("Failed to find win_pname\n");
        goto error_exit;
    }

    /* pause the vm for consistent memory access */
    if (vmi_pause_vm(vmi) != VMI_SUCCESS) {
        printf("Failed to pause VM\n");
        goto error_exit;
    } // if

    /* demonstrate name and id accessors */
    char *name2 = vmi_get_name(vmi);

    if (VMI_FILE != vmi_get_access_mode(vmi)) {
        uint64_t id = vmi_get_vmid(vmi);

        printf("Process listing for VM %s (id=%"PRIu64")\n", name2, id);
    }
    else {
        printf("Process listing for file %s\n", name2);
    }
    free(name2);

    /* get the head of the list */
    if (VMI_OS_LINUX == vmi_get_ostype(vmi)) {
        /* Begin at PID 0, the 'swapper' task. It's not typically shown by OS
         *  utilities, but it is indeed part of the task list and useful to
         *  display as such.
         */
        list_head = vmi_translate_ksym2v(vmi, "init_task") + tasks_offset;
    }
    else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) {

        // find PEPROCESS PsInitialSystemProcess
        if(VMI_FAILURE == vmi_read_addr_ksym(vmi, "PsActiveProcessHead", &list_head)) {
            printf("Failed to find PsActiveProcessHead\n");
            goto error_exit;
        }
        else
        	//printf("The next active process links pointer is at %"PRIx64"\n", list_head);
        status = vmi_read_addr_va(vmi, list_head - tasks_offset + handletable_offset, 0, &list_head1);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
            goto error_exit;
        }
        else
            //printf("The next handle table list pointer is at %"PRIx64"\n", list_head1);
        status = vmi_read_addr_va(vmi, list_head - tasks_offset + session_offset, 0, &session_listhead);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", session_listhead);
            goto error_exit;
        }
        status = vmi_read_addr_va(vmi, list_head - tasks_offset + peb_offset, 0, &peb_listhead);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", peb_listhead);
            goto error_exit;
        }
    }

	printf("Printing the main process list\n");

	next_list_entry = list_head;

    /* walk the task list */
    do {

        current_process = next_list_entry - tasks_offset;

        /* Note: the task_struct that we are looking at has a lot of
         * information.  However, the process name and id are burried
         * nice and deep.  Instead of doing something sane like mapping
         * this data to a task_struct, I'm just jumping to the location
         * with the info that I want.  This helps to make the example
         * code cleaner, if not more fragile.  In a real app, you'd
         * want to do this a little more robust :-)  See
         * include/linux/sched.h for mode details */

        /* NOTE: _EPROCESS.UniqueProcessId is a really VOID*, but is never > 32 bits,
         * so this is safe enough for x64 Windows for example purposes */
        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);

        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

        if (!procname) {
            printf("Failed to find procname\n");
            goto error_exit;
        }

        /* print out the process name */
        if(pid >= 0)
        	printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
        if (procname) {
            free(procname);
            procname = NULL;
        }

        /* follow the next pointer */

        status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
            goto error_exit;
        }

    } while(next_list_entry != list_head);

	if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) 
	{
		printf("Printing HANDLE_TABLE links\n");
		    next_list_entry = list_head1 + ht_offset;
		    list_head1 = next_list_entry;
		addr_t addrvalue = 0;
		    do {

		        current_process = next_list_entry - ht_offset;
		        vmi_read_32_va(vmi, current_process + ht_pid, 0, (uint32_t*)&pid);
		        vmi_read_addr_va(vmi,current_process + eprocess_offset,0,&eprocess_head);

		        if(eprocess_head != 0)
		        {
		        	procname = vmi_read_str_va(vmi,eprocess_head + name_offset, 0);

		        if (!procname) {
		            printf("Failed to find procname\n");
		            goto error_exit;
		        }
		        printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
		        if (procname) {
		            free(procname);
		            procname = NULL;
		        }
		    }
		    else
		    {
		    	printf("[%5d] (struct addr:%"PRIx64")\n", pid, current_process);
		    }

		    status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
		    if (status == VMI_FAILURE) {
		        printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
		        goto error_exit;
		    }

		    } while(next_list_entry != list_head1);


		printf("PRINTING ACCORDING TO SESSIONPROCESSLINKS\n");
		while(session_listhead == 0)
		{
			status = vmi_read_addr_va(vmi, list_head, 0, &session_listhead);
			list_head = session_listhead;
		    if (status == VMI_FAILURE) {
		        printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
		        goto error_exit;
		    }
		    else
		        printf("The next handle table list pointer is at %"PRIx64"\n", list_head1);
		    session_listhead = session_listhead - tasks_offset + session_offset;
		    status = vmi_read_addr_va(vmi, session_listhead, 0, &session_listhead);
		}
		printf("The session process links start at %"PRIx64"\n",session_listhead);

		next_list_entry = session_listhead;
		do {

		        current_process = next_list_entry - session_offset;
		        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);

		        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

		        if (!procname) {
		            printf("Failed to find procname\n");
		            goto error_exit;
		        }
		        printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
		        if (procname) {
		            free(procname);
		            procname = NULL;
		        }

		        status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
		        if (status == VMI_FAILURE) {
		            printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
		            goto error_exit;
		        }

		    } while(next_list_entry != session_listhead);

		printf("ENTERED _PEB withvalue %"PRIx64"\n", peb_listhead);
		status = vmi_read_addr_va(vmi, peb_listhead+peb_ldr_offset, 0, &peb_listhead);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", peb_listhead+peb_ldr_offset);
            goto error_exit;
        }
        else
        	printf("ENTERED _PEB_LDR_DATA withvalue %"PRIx64"\n", peb_listhead);
		status = vmi_read_addr_va(vmi, peb_listhead+initorder, 1, &peb_listhead);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", peb_listhead+initorder);
            goto error_exit;
        }
		next_module = peb_listhead;
		list_head = next_module;
		while (1) {

	        addr_t tmp_next = 0;

	        vmi_read_addr_va(vmi, next_module, 0, &tmp_next);

	        /* if we are back at the list head, we are done */
	        if (list_head == tmp_next) {
	            break;
	        }

	        /* print out the module name */

	        /* Note: the module struct that we are looking at has a string
	         * directly following the next / prev pointers.  This is why you
	         * can just add the length of 2 address fields to get the name.
	         * See include/linux/module.h for mode details */
	        if (VMI_OS_LINUX == vmi_get_ostype(vmi)) {
	            char *modname = NULL;

	            if (VMI_PM_IA32E == vmi_get_page_mode(vmi)) {   // 64-bit paging
	                modname = vmi_read_str_va(vmi, next_module + 16, 0);
	            }
	            else {
	                modname = vmi_read_str_va(vmi, next_module + 8, 0);
	            }
	            printf("%s\n", modname);
	            free(modname);
	        }
	        else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) {

	            unicode_string_t *us = NULL;

	            /*
	             * The offset 0x58 and 0x2c is the offset in the _LDR_DATA_TABLE_ENTRY structure
	             * to the BaseDllName member.
	             * These offset values are stable (at least) between XP and Windows 7.
	             */

	            if (VMI_PM_IA32E == vmi_get_page_mode(vmi)) {
	                us = vmi_read_unicode_str_va(vmi, next_module + 0x48, 0);
	            } else {
	                us = vmi_read_unicode_str_va(vmi, next_module + 0x2c, 0);
	            }

	            unicode_string_t out = { 0 };
	            //         both of these work
	            if (us &&
	                VMI_SUCCESS == vmi_convert_str_encoding(us, &out,
	                                                        "UTF-8")) {
	                printf("%s\n", out.contents);
	                //            if (us && 
	                //                VMI_SUCCESS == vmi_convert_string_encoding (us, &out, "WCHAR_T")) {
	                //                printf ("%ls\n", out.contents);
	                free(out.contents);
	            }   // if
	            if (us)
	                vmi_free_unicode_str(us);
	        }
	        next_module = tmp_next;
	    }
	}

	else if (VMI_OS_LINUX == vmi_get_ostype(vmi)) 
	{
		vmi_read_addr_va(vmi, list_head, 0, &list_head);
		next_list_entry = list_head - tasks_offset + taskrun_offset;
		list_head = next_list_entry;
		printf("Printing TASK_RUN list\n");
		/* walk the task list */
	    do {

	        current_process = next_list_entry - taskrun_offset;
	        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);

	        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

	        if (!procname) {
	            printf("Failed to find procname\n");
	            goto error_exit;
	        }
	        	printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
	        if (procname) {
	            free(procname);
	            procname = NULL;
	      	}

	        /* follow the next pointer */

	        status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
	        if (status == VMI_FAILURE) {
	            printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
	            goto error_exit;
	        }
	        else
	        	printf("next pointer in loop at %"PRIx64"\n", next_list_entry - taskrun_offset);

	    } while(next_list_entry != list_head);
	}
error_exit:
    /* resume the vm */
    vmi_resume_vm(vmi);

    /* cleanup any memory associated with the LibVMI instance */
    vmi_destroy(vmi);

    return 0;
}
