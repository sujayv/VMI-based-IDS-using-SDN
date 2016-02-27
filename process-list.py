#!/usr/bin/env python

import sys
import pyvmi
import pdb

def get_processes(vmi):
	vmi.pause_vm()
	type = vmi.get_ostype()
	if(type == "Linux"):
		tasks_offset = vmi.get_offset("linux_tasks")
		name_offset = vmi.get_offset("linux_name")
		pid_offset = vmi.get_offset("linux_pid")
		current_process = vmi.translate_ksym2v("init_task")
	else:
		tasks_offset = vmi.get_offset("win_tasks")
		name_offset = vmi.get_offset("win_pname")
		pid_offset = vmi.get_offset("win_pid")
		current_process = vmi.read_addr_ksym("PSInitialSystemProcess")

	list_head = current_process + tasks_offset
	current_list_entry = list_head


	next_list_entry = vmi.read_addr_va(current_list_entry,0)
	
	while next_list_entry != list_head:
		pid = vmi.read_32_va(current_process + pid_offset, 0)
		procname = vmi.read_str_va(current_process + name_offset, 0)
		if(procname == None):
			print "failed to find procname"
		print pid
		print procname
		current_list_entry = next_list_entry
		current_process = current_list_entry - tasks_offset

		next_list_entry = vmi.read_addr_va(current_list_entry,0)
	vmi.resume_vm()

def main(argv):
	vmi = pyvmi.init(argv[1], "complete")
	get_processes(vmi)
if __name__ == '__main__':
	main(sys.argv)
