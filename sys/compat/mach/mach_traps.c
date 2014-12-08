#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/eventhandler.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/socket.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/times.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/ptrace.h>
#include <sys/signalvar.h>

#include <netinet/in.h>
#include <sys/sysproto.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_extern.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_proto.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_semaphore.h>
#include <compat/mach/mach_notify.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_vm.h>

#include <sys/mach/mach_types.h>
#include <sys/mach/ipc/ipc_types.h>
#include <sys/mach/ipc/ipc_kmsg.h>
#include <sys/mach/thread.h>
#include <sys/mach/mach_port_server.h>
#include <sys/mach/mach_vm_server.h>


#pragma clang diagnostic ignored "-Wunused-parameter"

int
sys_mach_clock_sleep_trap(struct thread *td, struct mach_clock_sleep_trap_args *uap)
{

	return (mach_clock_sleep(uap->clock_name, uap->sleep_type, uap->sleep_sec, uap->sleep_nsec, uap->wakeup_time));
}

int
sys_mach_timebase_info(struct thread *td __unused, struct mach_timebase_info_args *uap)
{

	return (mach_timebase_info(uap->info));
}

int
sys_mach_msg_overwrite_trap(struct thread *td, struct mach_msg_overwrite_trap_args *uap)
{

	return (0);
}

int
sys_mach_msg_trap(struct thread *td, struct mach_msg_trap_args *uap)
{
	return (0);
}
	
int
sys_mach_semaphore_wait_trap(struct thread *td, struct mach_semaphore_wait_trap_args *uap)
{
	return (ENOSYS);	
}

int
sys_mach_semaphore_signal_trap(struct thread *td, struct mach_semaphore_signal_trap_args *uap)
{
	return (0);
}

int
sys_mach_semaphore_wait_signal_trap(struct thread *td, struct mach_semaphore_wait_signal_trap_args *uap)
{
	return (ENOSYS);
}

int
sys_mach_semaphore_signal_thread_trap(struct thread *td, struct mach_semaphore_signal_thread_trap_args *uap)
{

	return (ENOSYS);
}

int
sys_mach_semaphore_signal_all_trap(struct thread *td, struct mach_semaphore_signal_all_trap_args *uap)
{

	return (ENOSYS);
}

int
sys_mach_task_for_pid(struct thread *td, struct mach_task_for_pid_args *uap)
{

	return (ENOSYS);
}

int
sys_mach_thread_switch(struct thread *td, struct mach_thread_switch_args *uap)
{

	return (mach_thread_switch(td, uap->thread_name, uap->option, uap->option_time));
}	

int
sys_mach_swtch_pri(struct thread *td, struct mach_swtch_pri_args *uap)
{

	return (mach_swtch_pri(td, uap->pri));
}

int
sys_mach_swtch(struct thread *td, struct mach_swtch_args *v)
{

	return (mach_swtch_pri(td, 0));
}

int
sys_mach_reply_port(struct thread *td, struct mach_reply_port_args *uap)
{
#ifdef notyet	
	struct mach_right *mr;

	mr = mach_right_get(mach_port_get(), td, MACH_PORT_TYPE_RECEIVE, 0);
	td->td_retval[0] = (register_t)mr->mr_name;
#endif
	return (0);
}

int
sys_mach_thread_self_trap(struct thread *td, struct mach_thread_self_trap_args *uap)
{
#ifdef notyet
	struct mach_thread_emuldata *mle;
	struct mach_right *mr;

	mle = td->td_emuldata;
	mr = mach_right_get(mle->mle_kernel, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;
#endif
	return (0);
}

int
sys_mach_task_self_trap(struct thread *td, struct mach_task_self_trap_args *uap)
{
#ifdef notyet
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)td->td_proc->p_emuldata;
	mr = mach_right_get(med->med_kernel, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;
#endif
	return (0);
}


int
sys_mach_host_self_trap(struct thread *td, struct mach_host_self_trap_args *uap)
{

#ifdef notyet
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)td->td_proc->p_emuldata;
	mr = mach_right_get(med->med_host, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;
#endif
	return (0);
}

int
sys__kernelrpc_mach_port_allocate_trap(struct thread *td __unused, struct _kernelrpc_mach_port_allocate_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;
	mach_port_name_t name;
	int error;

	if ((error = mach_port_allocate(space, uap->right, &name)) != 0)
		return (error);
	return (copyout(&name, uap->name, sizeof(*uap->name)));
}

int
sys__kernelrpc_mach_port_deallocate_trap(struct thread *td, struct _kernelrpc_mach_port_deallocate_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;

	mach_port_deallocate(space, uap->name);
	return (0);
}

int
sys__kernelrpc_mach_port_insert_right_trap(struct thread *td __unused, struct _kernelrpc_mach_port_insert_right_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;
	mach_port_t poly = CAST_MACH_NAME_TO_PORT(uap->poly);

	return (mach_port_insert_right(space, uap->name, poly, uap->polyPoly));
}

int
sys__kernelrpc_mach_port_mod_refs_trap(struct thread *td, struct _kernelrpc_mach_port_mod_refs_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;
	/*
	  mach_port_name_t target = uap->target;
	  mach_port_name_t *name = uap->name;
	  mach_port_right_t right = uap->right;
	  mach_port_delta_t delta = uap->delta;
	*/
	return (mach_port_mod_refs(space, uap->name, uap->right, uap->delta));
}

int
sys__kernelrpc_mach_port_move_member_trap(struct thread *td, struct _kernelrpc_mach_port_move_member_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;

	return (mach_port_move_member(space, uap->member, uap->after));
}

int
sys__kernelrpc_mach_port_insert_member_trap(struct thread *td, struct _kernelrpc_mach_port_insert_member_trap_args *uap)
{
	return (ENOSYS);
}

int
sys__kernelrpc_mach_port_extract_member_trap(struct thread *td, struct _kernelrpc_mach_port_extract_member_trap_args *uap)
{
	return (ENOSYS);
}

int
sys__kernelrpc_mach_port_construct_trap(struct thread *td, struct _kernelrpc_mach_port_construct_trap_args *uap)
{
	return (ENOSYS);
}

int
sys__kernelrpc_mach_port_destruct_trap(struct thread *td, struct _kernelrpc_mach_port_destruct_trap_args *uap)
{

return (ENOSYS);
}

int
sys__kernelrpc_mach_port_destroy_trap(struct thread *td, struct _kernelrpc_mach_port_destroy_trap_args *uap)
{
	thread_t mthread = current_thread();
	ipc_space_t space = mthread->task->itk_space;

	return (mach_port_destroy(space, uap->name));
}

int
sys__kernelrpc_mach_port_guard_trap(struct thread *td, struct _kernelrpc_mach_port_guard_trap_args *uap)
{
	return (ENOSYS);
}

int
sys__kernelrpc_mach_port_unguard_trap(struct thread *td, struct _kernelrpc_mach_port_unguard_trap_args *uap)
{
	return (ENOSYS);
}

int
sys__kernelrpc_mach_vm_map_trap(struct thread *td, struct _kernelrpc_mach_vm_map_trap_args *uap)
{
	int error;
	vm_offset_t addr;

	if ((error = copyin(uap->address, &addr, sizeof(addr))) != 0)
		return (error);
	error = mach_vm_map(&curthread->td_proc->p_vmspace->vm_map, &addr, uap->size, uap->mask, uap->flags, NULL, 0, 0, uap->cur_protection,
						VM_PROT_ALL, VM_INHERIT_NONE);
	if (error)
		return (error);
	return (copyout(&addr, uap->address, sizeof(addr)));
}

int
sys__kernelrpc_mach_vm_allocate_trap(struct thread *td, struct _kernelrpc_mach_vm_allocate_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; current task only */
	mach_vm_offset_t *address = uap->address;
	mach_vm_offset_t uaddr;
	mach_vm_size_t size = uap->size;
	int flags = uap->flags;
	int error;

	if ((error = copyin(address, &uaddr, sizeof(mach_vm_offset_t))))
		return (error);

	if ((error = mach_vm_allocate(&td->td_proc->p_vmspace->vm_map,
								  &uaddr, size, flags)))
		return (error);
	if ((error = copyout(&uaddr, address, sizeof(mach_vm_offset_t))))
		return (error);
	return (0);
}

int
sys__kernelrpc_mach_vm_deallocate_trap(struct thread *td, struct _kernelrpc_mach_vm_deallocate_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; current task only */

	return (mach_vm_deallocate(&td->td_proc->p_vmspace->vm_map, uap->address, uap->size));
}

int
sys__kernelrpc_mach_vm_protect_trap(struct thread *td, struct _kernelrpc_mach_vm_protect_trap_args *uap)
{
	/* mach_port_name_t target = uap->target */
	/* int set_maximum = uap->set_maximum */

	return (mach_vm_protect(&td->td_proc->p_vmspace->vm_map, uap->address, uap->size, FALSE, uap->new_protection));
}

