/*-
 * Copyright (c) 2002-2003, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/sysproto.h>
#include <sys/uio.h>
#include <sys/vnode.h>

#include <sys/ktrace.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_extern.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_vm.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_proto.h>

#include <sys/mach/mach_vm_server.h>
#include <sys/mach/vm_map_server.h>
#include <sys/mach/host_priv_server.h>
/*
 * Like copyin(), but operates on an arbitrary process.
 */


static int
copyin_vm_map(vm_map_t map __unused, const void *uaddr __unused, void *kaddr __unused, size_t len __unused)
{

	return (0);
}

static int
copyout_vm_map(vm_map_t map __unused, const void *kaddr __unused, void *uaddr __unused, size_t len __unused)
{

	return (0);
}

int
copyin_proc(struct proc *p, const void *uaddr, void *kaddr, size_t len)
{
	struct proc *mycp;
	struct vmspace *myvm, *tmpvm;
	struct thread *td = curthread;
	int error;

	/* XXX need to suspend other threads in curproc if we're in a user
	 * context
	 */

	/*
	 * Local copies of curproc (cp) and vmspace (myvm)
	 */
	mycp = td->td_proc;
	myvm = mycp->p_vmspace;

	/*
	 * Connect to process address space for user program.
	 */
	if (p != mycp) {
		/*
		 * Save the current address space that we are
		 * connected to.
		 */
		tmpvm = mycp->p_vmspace;

		/*
		 * Point to the new user address space, and
		 * refer to it.
		 */
		mycp->p_vmspace = p->p_vmspace;
		atomic_add_int(&mycp->p_vmspace->vm_refcnt, 1);

		/* Activate the new mapping. */
		pmap_activate(FIRST_THREAD_IN_PROC(mycp));
	}
	error = copyin(uaddr, kaddr, len);
	if (p != mycp) {
		mycp->p_vmspace = myvm;
		vmspace_free(tmpvm);
		/* Activate the old mapping. */
		pmap_activate(FIRST_THREAD_IN_PROC(mycp));
	}
	return (error);
}

/*
 * Like copyout(), but operates on an arbitrary process.
 */
int
copyout_proc(struct proc *p, const void *kaddr, void *uaddr, size_t len)
{
	struct proc *mycp;
	struct vmspace *myvm, *tmpvm;
	struct thread *td = curthread;
	int error;

	/* XXX need to suspend other threads in curproc if we're in a user
	 * context
	 */

	/*
	 * Local copies of curproc (cp) and vmspace (myvm)
	 */
	mycp = td->td_proc;
	myvm = mycp->p_vmspace;

	/*
	 * Connect to process address space for user program.
	 */
	if (p != mycp) {
		/*
		 * Save the current address space that we are
		 * connected to.
		 */
		tmpvm = mycp->p_vmspace;

		/*
		 * Point to the new user address space, and
		 * refer to it.
		 */
		mycp->p_vmspace = p->p_vmspace;
		atomic_add_int(&mycp->p_vmspace->vm_refcnt, 1);

		/* Activate the new mapping. */
		pmap_activate(FIRST_THREAD_IN_PROC(mycp));
	}
	error = copyout(kaddr, uaddr, len);
	if (p != mycp) {
		mycp->p_vmspace = myvm;
		vmspace_free(tmpvm);
		/* Activate the old mapping. */
		pmap_activate(FIRST_THREAD_IN_PROC(mycp));
	}
	return (error);
}

int
mach_vm_map(vm_map_t map, mach_vm_address_t *address, mach_vm_size_t _size,
			mach_vm_offset_t _mask __unused, int _flags, mem_entry_name_port_t object __unused,
			memory_object_offset_t offset __unused, boolean_t copy __unused,
			vm_prot_t cur_protection, vm_prot_t max_protection, vm_inherit_t inh)
{
	vm_offset_t addr;
	size_t size;
	int flags;
	int docow, error;

	/* XXX Darwin fails on mapping a page at address 0 */
	if ((_flags & MACH_VM_FLAGS_ANYWHERE) == 0 && *address == 0)
		return (ENOMEM);

	size = round_page(_size);
	docow = error = 0;
#if 0
	/* Where Mach uses 0x00ff, we use 0x0100 */
	if ((req->req_mask & (req->req_mask + 1)) || (req->req_mask == 0))
		req->req_mask = 0;
	else
		req->req_mask += 1;
#endif
	flags = MAP_ANON;
	if ((_flags & MACH_VM_FLAGS_ANYWHERE) == 0) {
		flags |= MAP_FIXED;
		addr = trunc_page(*address);
	} else
		addr = 0;
	vm_map_lock(map);
	if (vm_map_findspace(map, addr, size, &addr)) {
		error = ENOMEM;
		goto done;
	}
	switch(inh) {
	case MACH_VM_INHERIT_SHARE:
		flags |= MAP_INHERIT_SHARE;
		break;
	case MACH_VM_INHERIT_COPY:
		flags |= MAP_COPY_ON_WRITE;
		docow = 1;
		break;
	case MACH_VM_INHERIT_NONE:
		break;
	case MACH_VM_INHERIT_DONATE_COPY:
	default:
		uprintf("mach_vm_map: unsupported inheritance flag %d\n", inh);
		break;
	}
	if (vm_map_insert(map, NULL, 0, addr, addr + size, cur_protection,
					  max_protection, docow)) {
		error = EFAULT;
		goto done;
	}
	*address = addr;
done:
	vm_map_unlock(map);
	return (error);
}

#if 0
int
mach_vm_map_msg(struct mach_trap_args *args)
{
	mach_vm_map_request_t *req = args->smsg;
	mach_vm_map_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int error;

#ifdef DEBUG_MACH_VM
	printf("mach_vm_map(addr = %p, size = 0x%08lx, obj = 0x%x, "
	    "mask = 0x%08lx, flags = 0x%x, offset = 0x%08llx, "
	    "copy = %d, cur_prot = 0x%x, max_prot = 0x%x, inh = 0x%x);\n",
	    (void *)req->req_address, (long)req->req_size, req->req_object.name,
	    (long)req->req_mask, req->req_flags, (off_t)req->req_offset,
	    req->req_copy, req->req_cur_protection, req->req_max_protection,
	    req->req_inheritance);
#endif

	error = mach_vm_map(args->ttd, &req->req_address, req->req_size,
						req->req_mask, req->req_flags, req->req_cur_protection,
						req->req_max_protection, req->req_inheritance);
	if (error)
		return (mach_msg_error(args, error));

	rep->rep_retval = req->req_address;
	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif

int
mach_vm_allocate(vm_map_t map, vm_offset_t *addr, size_t _size, int flags)
{
	size_t size = round_page(_size);
	vm_offset_t daddr;
	vm_prot_t prot, protmax;

	prot = VM_PROT_READ|VM_PROT_WRITE;
	protmax = VM_PROT_ALL;
	KASSERT(addr != NULL, ("invalid addr passed"));

	daddr = trunc_page(*addr);
	vm_map_lock(map);
	if ((flags & MACH_VM_FLAGS_ANYWHERE) &&
		(vm_map_findspace(map, 0, size, &daddr))) {
		vm_map_unlock(map);
		return (ENOMEM);
	} else {
		/* Address range must be all in user VM space. */
		if (daddr < vm_map_min(map) ||
		    daddr + size > vm_map_max(map))
			return (EINVAL);
		if (daddr + size < daddr)
			return (EINVAL);

	}
	if (vm_map_insert(map, NULL, 0, daddr, daddr + size, prot, protmax, 0)) {
		vm_map_unlock(map);
		return (EFAULT);
	}
	vm_map_unlock(map);
	*addr = daddr;
	return (0);
}

#if 0
int
mach_vm_allocate_msg(struct mach_trap_args *args)
{
	mach_vm_allocate_request_t *req = args->smsg;
	mach_vm_allocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct proc *tp = args->ttd->td_proc;
	vm_offset_t addr;
	size_t size;
	int error, flags;;

	addr = req->req_address;
	size = req->req_size;
	flags = req->req_flags;

#ifdef DEBUG_MACH_VM
	printf("mach_vm_allocate(addr = %p, size = 0x%08x);\n",
	    (void *)addr, size);
#endif

	if ((error = mach_vm_allocate(&tp->p_vmspace->vm_map, &addr, size, flags)))
		return (mach_msg_error(args, error));
	rep->rep_address = addr;
#ifdef DEBUG_MACH_VM
	printf("vm_allocate: success at %p\n", (void *)rep->rep_address);
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif

int
mach_vm_deallocate(vm_map_t target __unused, mach_vm_address_t addr, mach_vm_size_t len)
{
	struct munmap_args cup;

	cup.addr = (void *)addr;
	cup.len = len;
	return (sys_munmap(curthread, &cup));
}

#if 0
int
mach_vm_deallocate_msg(struct mach_trap_args *args)
{
	mach_vm_deallocate_request_t *req = args->smsg;
	mach_vm_deallocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *ttd = args->ttd;
	int error;

#ifdef DEBUG_MACH_VM
	printf("mach_vm_deallocate(addr = %p, size = 0x%08lx);\n",
	    (void *)req->req_address, (long)req->req_size);
#endif

	if ((error = mach_vm_deallocate(ttd, req->req_address, req->req_size)))
		return (mach_msg_error(args, error));

	rep->rep_retval = ttd->td_retval[0];
	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif
/*
 * XXX This server message Id clashes with bootstrap_look_up.
 * Is there a way to resolve this easily?
 */
#if 0
int
mach_vm_wire(struct mach_trap_args *args)
{
	mach_vm_wire_request_t *req = args->smsg;
	mach_vm_wire_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *ttd = args->ttd;
	int error;

#ifdef DEBUG_MACH_VM
	printf("mach_vm_wire(addr = %p, size = 0x%08x, prot = 0x%x);\n",
	    (void *)req->req_address, req->req_size, req->req_access);
#endif

	memset(&rep, 0, sizeof(*rep));

	if ((req->req_access & ~VM_PROT_ALL) != 0)
		return (mach_msg_error(args, EINVAL));

	/*
	 * Mach maintains a count of how many times a page is wired
	 * and unwire it once the count is zero. We cannot do that yet.
	 */
	if (req->req_access == 0) {
		struct munlock_args cup;

		cup.addr = (void *)req->req_address;
		cup.len = req->req_size;
		error = sys_munlock(ttd, &cup);
	} else {
		struct mlock_args cup;

		cup.addr = (void *)req->req_address;
		cup.len = req->req_size;
		error = sys_mlock(ttd, &cup);
	}
	if (error != 0)
		return (mach_msg_error(args, error));

	if ((error = vm_map_protect(&ttd->td_proc->p_vmspace->vm_map,
	    req->req_address, req->req_address + req->req_size,
	    req->req_access, 0)) != 0)
		return (mach_msg_error(args, error));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif

int
mach_vm_protect(vm_map_t target_task __unused, vm_offset_t addr, size_t len,
				boolean_t setmax __unused, vm_prot_t prot)
{
	struct mprotect_args cup;

	cup.addr = (void *)addr;
	cup.len = len;
	cup.prot = prot;

	return (sys_mprotect(curthread, &cup));
}

#if 0
int
mach_vm_protect_msg(struct mach_trap_args *args)
{
	mach_vm_protect_request_t *req = args->smsg;
	mach_vm_protect_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int error;

	if ((error = mach_vm_protect(args->ttd, req->req_addr, req->req_size, req->req_prot)) != 0)
		return (mach_msg_error(args, error));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif

#ifdef USE_OBSOLETE
int
sys_mach_map_fd(struct thread *td, struct mach_map_fd_args *uap)
{
	/* {
		syscallarg(int) fd;
		syscallarg(mach_vm_offset_t) offset;
		syscallarg(mach_vm_offset_t *) va;
		syscallarg(mach_boolean_t) findspace;
		syscallarg(mach_vm_size_t) size;
	} */

	struct munmap_args cup;
	struct mmap_args mmap_cup;
	void *va;
	int error;


	if ((error = copyin(uap->va, (void *)&va, sizeof(va))) != 0)
		return (error);

	mmap_cup.addr = va;
	mmap_cup.len = uap->size;
	mmap_cup.prot = VM_PROT_ALL;
	mmap_cup.flags = MAP_SHARED;
	mmap_cup.fd = uap->fd;
	mmap_cup.pos = uap->offset;
	if (uap->findspace == 0) {
		/* Make some free space XXX probably not The Right Way */
		cup.addr = va;
		cup.len = uap->size;
		(void)sys_munmap(td, &cup);
		mmap_cup.flags |= MAP_FIXED;
	}

	if ((error = sys_mmap(td, &mmap_cup)) != 0)
		return (error);

	if ((error = copyout((void *)&td->td_retval[0], uap->va, sizeof(va))) != 0)
		return (error);

	return (0);
}
#endif

int
mach_vm_inherit(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size,
				vm_inherit_t new_inheritance)
{
	struct minherit_args cup;
	int error;

	cup.addr = (void *)address;
	cup.len = size;
	/* Flags map well between Mach and NetBSD */
	cup.inherit = new_inheritance;

	if ((error = sys_minherit(curthread, &cup)) != 0)
		return (mach_msg_error(NULL, error));

	return (0);
}

int
mach_vm_map_page_query(
	vm_map_t target_map,
	vm_offset_t offset,
	integer_t *disposition,
	integer_t *ref_count
)
{
	return (KERN_NOT_SUPPORTED);
}

int
_mach_make_memory_entry(
	vm_map_t target_task,
	memory_object_size_t *size,
	memory_object_offset_t offset,
	vm_prot_t permission,
	mem_entry_name_port_t *object_handle,
	mem_entry_name_port_t parent_handle
)
{	

	return (KERN_NOT_SUPPORTED);
}

int
mach_make_memory_entry(vm_map_t target_task, vm_size_t *size,
					   vm_offset_t offset, vm_prot_t permission,
					   mem_entry_name_port_t *object_handle,
					   mem_entry_name_port_t parent_handle)
{	

	return (KERN_NOT_SUPPORTED);
}


int
mach_make_memory_entry_64(vm_map_t target_task, memory_object_size_t *size,
						  memory_object_offset_t offset, vm_prot_t permission,
						  mach_port_t *object_handle,
						  mem_entry_name_port_t parent_handle)
{	

	return (KERN_NOT_SUPPORTED);
}


#ifdef notyet
int
mach_make_memory_entry(vm_map_t target_task, memory_object_size_t *size,
					   memory_object_offset_t offset, vm_prot_t permission,
					   mem_entry_name_port_t *object_handle,
					   mem_entry_name_port_t parent_handle)
{
	struct mach_port *mp;
	struct mach_right *mr;
	struct mach_memory_entry *mme;

	printf("mach_make_memory_entry_64, offset 0x%lx, size 0x%lx\n",
	    (u_long)req->req_offset, (u_long)req->req_size);

	mp = mach_port_get();
	mp->mp_flags |= (MACH_MP_INKERNEL | MACH_MP_DATA_ALLOCATED);
	mp->mp_datatype = MACH_MP_MEMORY_ENTRY;
	
	mme = malloc(sizeof(*mme), M_MACH, M_WAITOK);
	mme->mme_proc = td->td_proc;
	mme->mme_offset = req->req_offset;
	mme->mme_size = req->req_size;
	mp->mp_data = mme;
	
	mr = mach_right_get(mp, td, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);

	rep->rep_size = req->req_size;
	
	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_vm_region(struct mach_trap_args *args)
{
	mach_vm_region_request_t *req = args->smsg;
	mach_vm_region_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *ttd = args->ttd;
	struct mach_vm_region_basic_info *rbi;
	struct vm_map *map;
	struct vm_map_entry *vme;
	int error;

	/* Sanity check req_count */
	if (req->req_count > 9)
		return (mach_msg_error(args, EINVAL));

	/*
	 * MACH_VM_REGION_BASIC_INFO is the only
	 * supported flavor in Darwin.
	 */
	if (req->req_flavor != MACH_VM_REGION_BASIC_INFO)
		return (mach_msg_error(args, EINVAL));
	if (req->req_count != (sizeof(*rbi) / sizeof(int))) /* This is 8 */
		return (mach_msg_error(args, EINVAL));
	*msglen = sizeof(*rep) + ((req->req_count - 9) * sizeof(int));

	map = &ttd->td_proc->p_vmspace->vm_map;

	vm_map_lock(map);
	error = vm_map_lookup_entry(map, req->req_addr, &vme);
	vm_map_unlock(map);

	if (error == 0)
		return (mach_msg_error(args, ENOMEM));

	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, 0); /* XXX Why this null name */

	rep->rep_addr = vme->start;
	rep->rep_size = vme->end - vme->start;
	rep->rep_count = req->req_count;

	rbi = (struct mach_vm_region_basic_info *)&rep->rep_info[0];
	rbi->protection = vme->protection;
	rbi->inheritance = 1; /* vme->inheritance */
	rbi->shared = 0; /* XXX how can we know? */
	rbi->offset = vme->offset;
	rbi->behavior = MACH_VM_BEHAVIOR_DEFAULT; /* XXX What is it? */
	rbi->user_wired_count = vme->wired_count;

	/* XXX Why this? */
	*(short *)((u_long)&rbi->user_wired_count + sizeof(short)) = 1;

	mach_set_trailer(rep, *msglen);

	return (0);
}


int
mach_vm_region_64(struct mach_trap_args *args)
{
	mach_vm_region_64_request_t *req = args->smsg;
	mach_vm_region_64_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *ttd = args->ttd;
	struct mach_vm_region_basic_info_64 *rbi;
	struct vm_map *map;
	struct vm_map_entry *vme;
	int error;

	/* Sanity check req_count */
	if (req->req_count > 10)
		return (mach_msg_error(args, EINVAL));

	/*
	 * MACH_VM_REGION_BASIC_INFO is the only
	 * supported flavor in Darwin.
	 */
	if (req->req_flavor != MACH_VM_REGION_BASIC_INFO)
		return (mach_msg_error(args, EINVAL));
	if (req->req_count != (sizeof(*rbi) / sizeof(int))) /* This is 8 */
		return (mach_msg_error(args, EINVAL));
	*msglen = sizeof(*rep) + ((req->req_count - 9) * sizeof(int));

	map = &ttd->td_proc->p_vmspace->vm_map;

	vm_map_lock(map);
	error = vm_map_lookup_entry(map, req->req_addr, &vme);
	vm_map_unlock(map);

	if (error == 0)
		return (mach_msg_error(args, ENOMEM));

	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, 0);	/* XXX null port ? */

	rep->rep_size = PAGE_SIZE; /* XXX Why? */
	rep->rep_count = req->req_count;

	rbi = (struct mach_vm_region_basic_info_64 *)&rep->rep_info[0];
	rbi->protection = vme->protection;
	rbi->inheritance = 1; /* vme->inheritance */
	rbi->shared = 0; /* XXX how can we know? */
	rbi->offset = vme->offset;
	rbi->behavior = MACH_VM_BEHAVIOR_DEFAULT; /* XXX What is it? */
	rbi->user_wired_count = vme->wired_count;
	
	/* XXX Why this? */
	*(short *)((u_long)&rbi->user_wired_count + sizeof(short)) = 1;

	mach_set_trailer(rep, *msglen);

	return (0);
}
#endif

int
mach_vm_msync(vm_map_t target_task __unused, mach_vm_address_t addr, mach_vm_size_t size,
		vm_sync_t flags)
{
	struct msync_args cup;
	int error;

	cup.addr = (void *)addr;
	cup.len = size;
	cup.flags = 0;
	if (flags & MACH_VM_SYNC_ASYNCHRONOUS)
		cup.flags |= MS_ASYNC;
	if (flags & MACH_VM_SYNC_SYNCHRONOUS)
		cup.flags |= MS_SYNC;
	if (flags & MACH_VM_SYNC_INVALIDATE)
		cup.flags |= MS_INVALIDATE;
	error = sys_msync(curthread, &cup);
	
	if (error)
		return (mach_msg_error(NULL, error));
	return (0);
}

/* XXX Do it for remote task */
int
mach_vm_copy(vm_map_t target_task, mach_vm_address_t src, mach_vm_size_t size,
		mach_vm_address_t dst)
		
{
	char *tmpbuf;
	int error;

#ifdef DEBUG_MACH_VM
	printf("mach_vm_copy: src = 0x%08lx, size = 0x%08lx, addr = 0x%08lx\n",
	    (long)req->req_src, (long)req->req_size, (long)req->req_addr);
#endif
	if ((src & (PAGE_SIZE - 1)) ||
	    (dst & (PAGE_SIZE - 1)) ||
	    (size & (PAGE_SIZE - 1)))
		return (mach_msg_error(NULL, EINVAL));


	tmpbuf = malloc(PAGE_SIZE, M_TEMP, M_WAITOK);

	/* Is there an easy way of dealing with that efficiently? */
	do {
		if ((error = copyin((void*)src, tmpbuf, PAGE_SIZE)) != 0)
			goto out;

		if ((error = copyout(tmpbuf, (void *)dst, PAGE_SIZE)) != 0)
			goto out;

		src += PAGE_SIZE;
		dst += PAGE_SIZE;
		size -= PAGE_SIZE;
	} while (size > 0);


	free(tmpbuf, M_TEMP);
	return (0);

out:
	free(tmpbuf, M_TEMP);
	return (mach_msg_error(NULL, error));
}

int
mach_vm_read(vm_map_t map, mach_vm_address_t addr, mach_vm_size_t size,
			 vm_offset_t *data, mach_msg_type_number_t *dataCnt)
{
	caddr_t tbuf;
	vm_offset_t dstaddr;
	vm_prot_t prot, protmax;
	int error;

	size = round_page(size);
	prot = VM_PROT_READ|VM_PROT_WRITE;
	protmax = VM_PROT_ALL;

	if ((error = mach_vm_allocate(map, &dstaddr, size, 0)))
		return (mach_msg_error(NULL, ENOMEM));
	/*
	 * Copy the data from the target process to the current process
	 * This is reasonable for small chunk of data, but we should
	 * remap COW for areas bigger than a page.
	 */
	tbuf = malloc(size, M_MACH, M_WAITOK);

	if ((error = copyin_vm_map(map, (caddr_t)addr, tbuf, size)) != 0) {
		printf("copyin_proc error = %d, addr = %lx, size = %zx\n", error, addr, size);
		free(tbuf, M_MACH);
		return (mach_msg_error(NULL, EFAULT));
	}

	if ((error = copyout(tbuf, (void *)dstaddr, size)) != 0) {
		printf("copyout error = %d\n", error);
		free(tbuf, M_MACH);
		return (mach_msg_error(NULL, EFAULT));
	}

	if (error == 0)
		ktrmool(tbuf, size, (void *)va);

	free(tbuf, M_MACH);
	return (0);
}

int
mach_vm_write(vm_map_t target_task, mach_vm_address_t address, vm_offset_t data,
	mach_msg_type_number_t dataCnt)
{
	size_t size = dataCnt;
	char *tbuf;
	int error;

	/*
	 * Copy the data from the current process to the target process
	 * This is reasonable for small chunk of data, but we should
	 * remap COW for areas bigger than a page.
	 */
	tbuf = malloc(size, M_MACH, M_WAITOK);

	if ((error = copyin((void *)address, tbuf, size)) != 0) {
		printf("copyin error = %d\n", error);
		free(tbuf, M_MACH);
		return (mach_msg_error(NULL, EFAULT));
	}

	if ((error = copyout_vm_map(target_task, tbuf, (void *)data, size)) != 0) {
		printf("copyout_proc error = %d\n", error);
		free(tbuf, M_MACH);
		return (mach_msg_error(NULL, EFAULT));
	}

	free(tbuf, M_MACH);

	return (0);
}

int
mach_vm_machine_attribute(vm_map_t target_task, mach_vm_address_t addr, mach_vm_size_t size,
						  vm_machine_attribute_t attribute, vm_machine_attribute_val_t *valuep)
{
	int error = 0;
	vm_machine_attribute_val_t value;


	if ((error = copyin(valuep, &value, sizeof(value))))
		return (mach_msg_error(NULL, error));

	switch (attribute) {
	case MACH_MATTR_CACHE:
		switch(value) {
		case MACH_MATTR_VAL_CACHE_FLUSH:
		case MACH_MATTR_VAL_DCACHE_FLUSH:
		case MACH_MATTR_VAL_ICACHE_FLUSH:
		case MACH_MATTR_VAL_CACHE_SYNC:
			error = cpu_mach_vm_machine_attribute(target_task, addr, size, &value);
			break;
		default:
#ifdef DEBUG_MACH
			printf("unimplemented value %d\n", value);
#endif
			error = EINVAL;
			break;
		}
		break;

	case MACH_MATTR_MIGRATE:
	case MACH_MATTR_REPLICATE:
	default:
#ifdef DEBUG_MACH
		printf("unimplemented attribute %d\n", attribute);
#endif
		error = EINVAL;
		break;
	}


	if (error)
		return (native_to_mach_errno[error]);

	return (copyout(&value, valuep, sizeof(value)));
}

int
mach_vm_behavior_set(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, vm_behavior_t new_behavior)
{

	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_page_info(vm_map_t target_task, mach_vm_address_t address,
				  vm_page_info_flavor_t flavor, vm_page_info_t info,
				  mach_msg_type_number_t *infoCnt)
{

		return (KERN_NOT_SUPPORTED);
}

int
mach_vm_page_query(
	vm_map_t target_map,
	mach_vm_offset_t offset,
	integer_t *disposition,
	integer_t *ref_count
)
{

		return (KERN_NOT_SUPPORTED);
}

int
mach_vm_purgable_control(
	vm_map_t target_task,
	mach_vm_address_t address,
	vm_purgable_t control,
	int *state
	)
{

	return (KERN_NOT_SUPPORTED);
}
	
int
mach_vm_read_list(
	vm_map_t target_task,
	mach_vm_read_entry_t data_list,
	natural_t count
	)
{
	return (KERN_NOT_SUPPORTED);
}


int
vm_read_list(
	vm_map_t target_task,
	vm_read_entry_t data_list,
	natural_t count
	)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_read_overwrite(
	vm_map_t target_task,
	mach_vm_address_t address,
	mach_vm_size_t size,
	mach_vm_address_t data,
	mach_vm_size_t *outsize
)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_region(
	vm_map_t target_task,
	mach_vm_address_t *address,
	mach_vm_size_t *size,
	vm_region_flavor_t flavor,
	vm_region_info_t info,
	mach_msg_type_number_t *infoCnt,
	mach_port_t *object_name
	)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_region_info(
	vm_map_t task,
	vm_address_t address,
	vm_info_region_t *region,
	vm_info_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt
)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_region_info_64(
	vm_map_t task,
	vm_address_t address,
	vm_info_region_64_t *region,
	vm_info_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt
)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_region_recurse(
	vm_map_t target_task,
	mach_vm_address_t *address,
	mach_vm_size_t *size,
	natural_t *nesting_depth,
	vm_region_recurse_info_t info,
	mach_msg_type_number_t *infoCnt
	)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_mapped_pages_info(
	vm_map_t task,
	page_address_array_t *pages,
	mach_msg_type_number_t *pagesCnt
)
{
	return (KERN_NOT_SUPPORTED);
}

	
int
mach_vm_remap(
	vm_map_t target_task,
	mach_vm_address_t *target_address,
	mach_vm_size_t size,
	mach_vm_offset_t mask,
	int flags,
	vm_map_t src_task,
	mach_vm_address_t src_address,
	boolean_t copy,
	vm_prot_t *cur_protection,
	vm_prot_t *max_protection,
	vm_inherit_t inheritance
	)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_wire(
	host_priv_t host_priv,
	vm_map_t task,
	mach_vm_address_t address,
	mach_vm_size_t size,
	vm_prot_t desired_access
)
{
	return (KERN_NOT_SUPPORTED);
}

int
mach_vm_wire_32(
	host_priv_t host_priv,
	vm_map_t task,
	vm_address_t address,
	vm_size_t size,
	vm_prot_t desired_access
)
{
	return (KERN_NOT_SUPPORTED);
}

int
task_wire(vm_map_t target_task, boolean_t must_wire)
{

	return (KERN_NOT_SUPPORTED);
}

int
vm_allocate_cpm
(
	host_priv_t host_priv,
	vm_map_t task,
	vm_address_t *address,
	vm_size_t size,
	int flags
)
{

	return (KERN_NOT_SUPPORTED);
}
