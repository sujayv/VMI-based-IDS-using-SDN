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
#include <time.h>
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
    unsigned long sessionptr_offset = 0,slink_offset = 0;
    char *procname = NULL;
    vmi_pid_t pid = 0;
    unsigned long initorder = 0,peb_offset = 0,tasks_offset = 0, pid_offset = 0, name_offset = 0, handletable_offset = 0,ht_offset = 0,ht_pid = 0,eprocess_offset = 0;
    unsigned long session_offset = 0,peb_ldr_offset = 0,taskrun_offset = 0;
    status_t status;
    vmi_pid_t *pidarr = malloc(32768 * sizeof(vmi_pid_t));
    vmi_pid_t *handlepidarr = malloc(32768 * sizeof(vmi_pid_t));
    vmi_pid_t *sessionpidarr = malloc(32768 * sizeof(vmi_pid_t));
    addr_t *session_space = malloc(100 * sizeof(addr_t));
    int countmain = 0, counthandle = 0, countsession = 0;
    clock_t start = 0;
    clock_t stop = 0;
    double total = 0;
    int numsyscalls = 317;
    FILE *fd = NULL;
    fd = fopen("/home/sujay/libvmi-master/examples/syscall.txt","r+");
    if(fd == NULL)
    {
    	printf("ERROR\n");
    	return 0;
    }
    for(int i=0;i<32768;i++)
    {
    	pidarr[i] = -1;
    	handlepidarr[i] = -1;
    	sessionpidarr[i] = -1;
    }
    for(int i=0;i<100;i++)
    {
    	session_space[i] = 0;
    }

    /* this is the VM or file that we are looking at */
    if (argc != 2) {
        printf("Usage: %s <vmname>\n", argv[0]);
        //return 1;
    }

    char *name = argv[1];
    printf("The system being analyzed is %s\n",name);
    //char *name;
    //int currentcycle = 0;

    /* initialize the libvmi library */
    start = clock();
    if (vmi_init(&vmi, VMI_AUTO | VMI_INIT_COMPLETE, name) == VMI_FAILURE) {
        printf("Failed to init LibVMI library.\n");
        return 1;
    }
    printf("THE SYSTEM IS NOW BEING ANALYZED...\n");
    /* init the offset values */
    if (VMI_OS_LINUX == vmi_get_ostype(vmi)) {
        tasks_offset = vmi_get_offset(vmi, "linux_tasks");
        name_offset = vmi_get_offset(vmi, "linux_name");
        pid_offset = vmi_get_offset(vmi, "linux_pid");
        taskrun_offset = 0x78;
    }
    else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) {
        tasks_offset = vmi_get_offset(vmi, "win_tasks");
        name_offset = vmi_get_offset(vmi, "win_pname");
        pid_offset = vmi_get_offset(vmi, "win_pid");
        ht_pid = 0x008;
        ht_offset = 0x010;
        eprocess_offset = 0x004;
        //printf("tasks_offset is %lu\n",tasks_offset);
        //printf("name_offset is %lu\n",name_offset);
        //printf("pid_offset is %lu\n",pid_offset);
        handletable_offset = 0x0f4;
        session_offset = 0x0e4;
        peb_offset = 0x1a8;
        peb_ldr_offset = 0x00c;
        initorder = 0x01c;
        sessionptr_offset = 0x168;
        slink_offset = 0x010;
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
    

    /* demonstrate name and id accessors */
    char *name2 = vmi_get_name(vmi);

    if (VMI_FILE != vmi_get_access_mode(vmi)) {
        uint64_t id = vmi_get_vmid(vmi);

        //printf("Process listing for VM %s (id=%"PRIu64")\n", name2, id);
    }
    else {
        //printf("Process listing for file %s\n", name2);
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

       /*if (vmi_pause_vm(vmi) != VMI_SUCCESS) {
        printf("Failed to pause VM\n");
        goto error_exit;
    }*/
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

	//printf("Printing the main process list\n");

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
        pidarr[countmain++] = pid;

        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

        if (!procname) {
            printf("Failed to find procname\n");
            goto error_exit;
        }

        /* print out the process name */
        if(pid >= 0)
        	//printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
        if (procname) {
            free(procname);
            procname = NULL;
        }
        addr_t t1 = 0;
        status = vmi_read_addr_va(vmi, current_process + sessionptr_offset, 0, &t1);
        //printf("%"PRIx64"\n", t1);
       if(t1 != 0)
        {
        	int j = 0;
        	int flag = 0;
        	while(session_space[j] != 0)
        	{
        		if(t1 == session_space[j])
        		{
        			flag = 1;
        			break;
        		}
        		else
        			j++;
        	}
        	if(flag == 0)
        	{
        		session_space[j] = t1;
        	}

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
		//printf("Printing HANDLE_TABLE links\n");
		addr_t eprocess = 0; 
		    next_list_entry = list_head1 + ht_offset;
		    list_head1 = next_list_entry;
		addr_t addrvalue = 0;
		    do {

		        current_process = next_list_entry - ht_offset;
		        vmi_read_32_va(vmi, current_process + ht_pid, 0, (uint32_t*)&pid);
		        handlepidarr[counthandle++] = pid;
		        vmi_read_addr_va(vmi,current_process + eprocess_offset,0,&eprocess_head);
		        if(eprocess_head != 0)
		        {
		        	procname = vmi_read_str_va(vmi,eprocess_head + name_offset, 0);

		        if (!procname) {
		            printf("Failed to find procname\n");
		            goto error_exit;
		        }
		        //printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
		        if (procname) {
		            free(procname);
		            procname = NULL;
		        }
		        addr_t t1 = 0;
		        status = vmi_read_addr_va(vmi, eprocess_head + sessionptr_offset, 0, &t1);
		        if(t1 != 0)
		        {
		        	int j = 0;
		        	int flag = 0;
		        	while(session_space[j] != 0)
		        	{
		        		if(t1 == session_space[j])
		        		{
		        			flag = 1;
		        			break;
		        		}
		        		else
		        			j++;
		        	}
		        	if(flag == 0)
		        	{
		        		session_space[j] = t1;
		        	}

		        }
		    }
		    else
		    {
		    	//printf("[%5d] (struct addr:%"PRIx64")\n", pid, current_process);
		    }

		    status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
		    if (status == VMI_FAILURE) {
		        printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
		        goto error_exit;
		    }

		    } while(next_list_entry != list_head1);

		//rintf("PRINTING ACCORDING TO SESSIONPROCESSLINKS\n");
		while(session_listhead == 0)
		{
			status = vmi_read_addr_va(vmi, list_head, 0, &session_listhead);
			list_head = session_listhead;
		    if (status == VMI_FAILURE) {
		        printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
		        goto error_exit;
		    }
		    //else
		        //printf("The next handle table list pointer is at %"PRIx64"\n", list_head1);
		    session_listhead = session_listhead - tasks_offset + session_offset;
		    status = vmi_read_addr_va(vmi, session_listhead, 0, &session_listhead);
		}
		//printf("The session process links start at %"PRIx64"\n",session_listhead);

		int j=0;
		while(session_space[j] != 0)
		{
			session_listhead = session_space[j];
			status = vmi_read_addr_va(vmi, session_listhead + slink_offset, 0, &session_listhead);
			next_list_entry = session_listhead;
			do {

			        current_process = next_list_entry - session_offset;
			        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);
			        sessionpidarr[countsession++] = pid;
			        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);
			        int fl = 0;
			        for(int i=0;i<32768;i++)
			        {
			        	if(sessionpidarr[i] == pid)
			        	{
			        		fl = 1;
			        		break;
			        	}
			        	else if(sessionpidarr[i] == -1)
			        		break;
			        }
			        if(fl == 0)
			        {
			        	sessionpidarr[countsession++] = pid;
			        }
			        if (!procname) {
			            printf("Failed to find procname\n");
			            goto error_exit;
			        }
			       // printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
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
			j++;
		}

/* resume the vm */
    //vmi_resume_vm(vmi);

    /* cleanup any memory associated with the LibVMI instance */
    vmi_destroy(vmi);
		int flag = 0;
		int procfound = 0;
		for(int i=0;i<32768;i++)
		{
			flag = 0;
			if(handlepidarr[i] != -1)
			{
				for(int j=0;j<32768;j++)
				{
					if(handlepidarr[i] == pidarr[j])
					{
						flag = 1;
						break;
					}
				}
				if(flag == 0 && handlepidarr[i] != 0)
				{
					procfound = 1;
					printf("Hidden process found using Handle Table with PID %5d\n",handlepidarr[i]);
				}
			}
			else
				break;
		}

		for(int i=0;i<32768;i++)
		{
			flag = 0;
			if(sessionpidarr[i] != -1)
			{
				for(int j=0;j<32768;j++)
				{
					if(pidarr[j] != -1)
					{
						if(sessionpidarr[i] == pidarr[j])
						{
							flag = 1;
							break;
						}
					}
					else
						break;
				}
				if(flag == 0 && handlepidarr[i] != 0)
				{
					procfound = 1;
					printf("Hidden process found using Session Process Links with PID %5d\n",sessionpidarr[i]);
				}
			}
			else
				break;
		}
		if(procfound == 0)
		{
			printf("There are NO hidden processes\n");
		}
	}
	else if (VMI_OS_LINUX == vmi_get_ostype(vmi)) 
	{
		//vmi_read_addr_va(vmi, list_head, 0, &list_head);
		int tostore = 8;
		fscanf(fd,"%d",&tostore);
		//printf("The value of 1st line is-------%d\n",tostore);
		if(tostore == 1)
		{
			fclose(fd);
			fd = fopen("/home/sujay/libvmi-master/examples/syscall.txt","w");
			fprintf(fd, "%d\n",0);
			list_head = vmi_translate_ksym2v(vmi, "sys_call_table");
			//printf("The sys call header is at %"PRIx64"\n", list_head);
			for(int i=0;i<=numsyscalls;i++)
			{
				status = vmi_read_addr_va(vmi, list_head, 0, &next_list_entry);
				fprintf(fd,"%lu\n", next_list_entry);
				list_head += 8;
			}
			fclose(fd);
		}
		else if(tostore == 0)
		{
			list_head = vmi_translate_ksym2v(vmi, "sys_call_table");
			int flag = 0;
			//printf("The sys call header is at %"PRIx64"\n", list_head);
			for(int i=0;i<=numsyscalls;i++)
			{
				status = vmi_read_addr_va(vmi, list_head, 0, &next_list_entry);
				unsigned long temp = next_list_entry;
				unsigned long temp1 = 0;
				fscanf(fd,"%lu",&temp1);
				if(temp != temp1)
				{
					if(flag == 0)
					{
						printf("HOOK DETECTED!!!!\nDetails:\n");
					}
					flag = 1;
					printf("The syscall number %d has a modified address which is %"PRIx64" and the original value should be %"PRIx64"\n",i,temp1,temp);
				}
				list_head +=8;
			}
			if(flag == 0)
			{
				printf("There are NO system call hooks!!!\n");
			}
			/* resume the vm */
    //vmi_resume_vm(vmi);

    /* cleanup any memory associated with the LibVMI instance */
    vmi_destroy(vmi);
		}
		
		/*next_list_entry = list_head - tasks_offset + taskrun_offset;
		list_head = next_list_entry;
		printf("Printing TASK_RUN list\n");
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


	        status = vmi_read_addr_va(vmi, next_list_entry, 0, &next_list_entry);
	        if (status == VMI_FAILURE) {
	            printf("Failed to read next pointer in loop at %"PRIx64"\n", next_list_entry);
	            goto error_exit;
	        }
	        else
	        	printf("next pointer in loop at %"PRIx64"\n", next_list_entry - taskrun_offset);

	    } while(next_list_entry != list_head);*/
	}
error_exit:

	free(pidarr);
	free(handlepidarr);
	free(sessionpidarr);
    stop = clock();
    total = (double)(((stop-start)*1000)/CLOCKS_PER_SEC);
    printf("Time taken to analyze: %.2lf milliseconds\n",total);
    //return 0;
    sleep(3);
	return 0;
}
