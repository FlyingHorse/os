/*
 * System call names.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $FreeBSD$
 * created from	FreeBSD
 */

const char *mach_syscallnames[] = {
#if defined(_KERNEL_OPT)
#endif
	"#0",			/* 0 = nosys */
	"#1",			/* 1 = nosys */
	"#2",			/* 2 = nosys */
	"#3",			/* 3 = nosys */
	"#4",			/* 4 = nosys */
	"#5",			/* 5 = nosys */
	"#6",			/* 6 = nosys */
	"#7",			/* 7 = nosys */
	"#8",			/* 8 = nosys */
	"#9",			/* 9 = nosys */
	"_kernelrpc_mach_vm_allocate_trap",			/* 10 = _kernelrpc_mach_vm_allocate_trap */
	"#11",			/* 11 = nosys */
	"_kernelrpc_mach_vm_deallocate_trap",			/* 12 = _kernelrpc_mach_vm_deallocate_trap */
	"#13",			/* 13 = nosys */
	"_kernelrpc_mach_vm_protect_trap",			/* 14 = _kernelrpc_mach_vm_protect_trap */
	"_kernelrpc_mach_vm_map_trap",			/* 15 = _kernelrpc_mach_vm_map_trap */
	"_kernelrpc_mach_port_allocate_trap",			/* 16 = _kernelrpc_mach_port_allocate_trap */
	"_kernelrpc_mach_port_destroy_trap",			/* 17 = _kernelrpc_mach_port_destroy_trap */
	"_kernelrpc_mach_port_deallocate_trap",			/* 18 = _kernelrpc_mach_port_deallocate_trap */
	"_kernelrpc_mach_port_mod_refs_trap",			/* 19 = _kernelrpc_mach_port_mod_refs_trap */
	"_kernelrpc_mach_port_move_member_trap",			/* 20 = _kernelrpc_mach_port_move_member_trap */
	"_kernelrpc_mach_port_insert_right_trap",			/* 21 = _kernelrpc_mach_port_insert_right_trap */
	"_kernelrpc_mach_port_insert_member_trap",			/* 22 = _kernelrpc_mach_port_insert_member_trap */
	"_kernelrpc_mach_port_extract_member_trap",			/* 23 = _kernelrpc_mach_port_extract_member_trap */
	"_kernelrpc_mach_port_construct_trap",			/* 24 = _kernelrpc_mach_port_construct_trap */
	"_kernelrpc_mach_port_destruct_trap",			/* 25 = _kernelrpc_mach_port_destruct_trap */
	"mach_reply_port",			/* 26 = mach_reply_port */
	"mach_thread_self_trap",			/* 27 = mach_thread_self_trap */
	"mach_task_self_trap",			/* 28 = mach_task_self_trap */
	"mach_host_self_trap",			/* 29 = mach_host_self_trap */
	"#30",			/* 30 = nosys */
	"mach_msg_trap",			/* 31 = mach_msg_trap */
	"mach_msg_overwrite_trap",			/* 32 = mach_msg_overwrite_trap */
	"mach_semaphore_signal_trap",			/* 33 = mach_semaphore_signal_trap */
	"mach_semaphore_signal_all_trap",			/* 34 = mach_semaphore_signal_all_trap */
	"mach_semaphore_signal_thread_trap",			/* 35 = mach_semaphore_signal_thread_trap */
	"mach_semaphore_wait_trap",			/* 36 = mach_semaphore_wait_trap */
	"mach_semaphore_wait_signal_trap",			/* 37 = mach_semaphore_wait_signal_trap */
	"mach_semaphore_timedwait_trap",			/* 38 = mach_semaphore_timedwait_trap */
	"mach_semaphore_timedwait_signal_trap",			/* 39 = mach_semaphore_timedwait_signal_trap */
	"#40",			/* 40 = nosys */
	"_kernelrpc_mach_port_guard_trap",			/* 41 = _kernelrpc_mach_port_guard_trap */
	"_kernelrpc_mach_port_unguard_trap",			/* 42 = _kernelrpc_mach_port_unguard_trap */
	"obs_{",			/* 43 = obsolete { */
	"mach_task_name_for_pid",			/* 44 = mach_task_name_for_pid */
	"mach_task_for_pid",			/* 45 = mach_task_for_pid */
	"mach_pid_for_task",			/* 46 = mach_pid_for_task */
	"#47",			/* 47 = nosys */
	"mach_macx_swapon",			/* 48 = mach_macx_swapon */
	"mach_macx_swapoff",			/* 49 = mach_macx_swapoff */
	"#50",			/* 50 = nosys */
	"mach_macx_triggers",			/* 51 = mach_macx_triggers */
	"mach_macx_backing_store_suspend",			/* 52 = mach_macx_backing_store_suspend */
	"mach_macx_backing_store_recovery",			/* 53 = mach_macx_backing_store_recovery */
	"#54",			/* 54 = nosys */
	"#55",			/* 55 = nosys */
	"#56",			/* 56 = nosys */
	"#57",			/* 57 = nosys */
	"#58",			/* 58 = nosys */
	"mach_swtch_pri",			/* 59 = mach_swtch_pri */
	"mach_swtch",			/* 60 = mach_swtch */
	"mach_thread_switch",			/* 61 = mach_thread_switch */
	"mach_clock_sleep_trap",			/* 62 = mach_clock_sleep_trap */
	"#63",			/* 63 = nosys */
	"#64",			/* 64 = nosys */
	"#65",			/* 65 = nosys */
	"#66",			/* 66 = nosys */
	"#67",			/* 67 = nosys */
	"#68",			/* 68 = nosys */
	"#69",			/* 69 = nosys */
	"#70",			/* 70 = nosys */
	"#71",			/* 71 = nosys */
	"#72",			/* 72 = nosys */
	"#73",			/* 73 = nosys */
	"#74",			/* 74 = nosys */
	"#75",			/* 75 = nosys */
	"#76",			/* 76 = nosys */
	"#77",			/* 77 = nosys */
	"#78",			/* 78 = nosys */
	"#79",			/* 79 = nosys */
	"#80",			/* 80 = nosys */
	"#81",			/* 81 = nosys */
	"#82",			/* 82 = nosys */
	"#83",			/* 83 = nosys */
	"#84",			/* 84 = nosys */
	"#85",			/* 85 = nosys */
	"#86",			/* 86 = nosys */
	"#87",			/* 87 = nosys */
	"#88",			/* 88 = nosys */
	"mach_timebase_info",			/* 89 = mach_timebase_info */
	"mach_wait_until",			/* 90 = mach_wait_until */
	"mk_timer_create",			/* 91 = mk_timer_create */
	"mk_timer_destroy",			/* 92 = mk_timer_destroy */
	"mk_timer_arm",			/* 93 = mk_timer_arm */
	"mk_timer_cancel",			/* 94 = mk_timer_cancel */
	"#95",			/* 95 = nosys */
	"#96",			/* 96 = nosys */
	"#97",			/* 97 = nosys */
	"#98",			/* 98 = nosys */
	"#99",			/* 99 = nosys */
	"#100",			/* 100 = nosys */
	"#101",			/* 101 = nosys */
	"#102",			/* 102 = nosys */
	"#103",			/* 103 = nosys */
	"#104",			/* 104 = nosys */
	"#105",			/* 105 = nosys */
	"#106",			/* 106 = nosys */
	"#107",			/* 107 = nosys */
	"#108",			/* 108 = nosys */
	"#109",			/* 109 = nosys */
	"#110",			/* 110 = nosys */
	"#111",			/* 111 = nosys */
	"#112",			/* 112 = nosys */
	"#113",			/* 113 = nosys */
	"#114",			/* 114 = nosys */
	"#115",			/* 115 = nosys */
	"#116",			/* 116 = nosys */
	"#117",			/* 117 = nosys */
	"#118",			/* 118 = nosys */
	"#119",			/* 119 = nosys */
	"#120",			/* 120 = nosys */
	"#121",			/* 121 = nosys */
	"#122",			/* 122 = nosys */
	"#123",			/* 123 = nosys */
	"#124",			/* 124 = nosys */
	"#125",			/* 125 = nosys */
	"#126",			/* 126 = nosys */
	"#127",			/* 127 = nosys */
};
