/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met: 1. Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the following
 * disclaimer.  2. Redistributions in binary form must reproduce the
 * above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
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
#include <sys/signal.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <vm/uma.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_iokit.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_notify.h>
#include <compat/mach/mach_services.h>
#include <compat/mach/mach_proto.h>

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_exec.h>
#endif

/* Right and port zones, list of all rights and its lock */
static uma_zone_t mach_port_zone;
static uma_zone_t mach_right_zone;

struct mach_port *mach_bootstrap_port;
struct mach_port *mach_clock_port;
struct mach_port *mach_io_master_port;
struct mach_port *mach_saved_bootstrap_port;

int
sys_mach_reply_port(struct thread *td, struct mach_reply_port_args *uap)
{
	struct mach_right *mr;

	mr = mach_right_get(mach_port_get(), td, MACH_PORT_TYPE_RECEIVE, 0);
	td->td_retval[0] = (register_t)mr->mr_name;

	return (0);
}

int
sys_mach_thread_self_trap(struct thread *td, struct mach_thread_self_trap_args *uap)
{
	struct mach_thread_emuldata *mle;
	struct mach_right *mr;

	mle = td->td_emuldata;
	mr = mach_right_get(mle->mle_kernel, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;

	return (0);
}


int
sys_mach_task_self_trap(struct thread *td, struct mach_task_self_trap_args *uap)
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)td->td_proc->p_emuldata;
	mr = mach_right_get(med->med_kernel, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;

	return (0);
}


int
sys_mach_host_self_trap(struct thread *td, struct mach_host_self_trap_args *uap)
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)td->td_proc->p_emuldata;
	mr = mach_right_get(med->med_host, td, MACH_PORT_TYPE_SEND, 0);
	td->td_retval[0] = (register_t)mr->mr_name;

	return (0);
}

static void
mach_port_deallocate(struct thread *td, mach_port_t mn)
{
	struct mach_right *mr;

	if ((mr = mach_right_check(mn, td, MACH_PORT_TYPE_REF_RIGHTS)) != NULL)
		mach_right_put(mr, MACH_PORT_TYPE_REF_RIGHTS);
}

int
sys__kernelrpc_mach_port_deallocate_trap(struct thread *td, struct _kernelrpc_mach_port_deallocate_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; */

	mach_port_deallocate(td, uap->name);
	return (0);
}

int
mach_port_deallocate_msg(struct mach_trap_args *args)
{
	mach_port_deallocate_request_t *req = args->smsg;
	mach_port_deallocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	mach_port_deallocate(args->td, req->req_name);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);
	return (0);
}

static int
mach_port_destroy(struct thread *td, mach_port_name_t name)
{
	struct mach_right *mr;

	if ((mr = mach_right_check(name, td, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return (ENOENT);

	MACH_PORT_UNREF(mr->mr_port);
	mr->mr_port = NULL;
	mach_right_put(mr, MACH_PORT_TYPE_ALL_RIGHTS);
	return (0);
}

int
sys__kernelrpc_mach_port_destroy_trap(struct thread *td, struct _kernelrpc_mach_port_destroy_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; */

	return (mach_port_destroy(td, uap->name));
}

int
mach_port_destroy_msg(struct mach_trap_args *args)
{
	mach_port_destroy_request_t *req = args->smsg;
	mach_port_destroy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

#ifdef DEBUG_MACH
	printf("mach_port_destroy mn = %x\n", req->req_name);
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = mach_port_destroy(args->td, req->req_name);

	mach_set_trailer(rep, *msglen);

	return (0);
}

static int
mach_port_allocate(struct thread *td, mach_port_right_t right, struct mach_right **mr)
{
	struct mach_port *mp;

	switch (right) {
	case MACH_PORT_RIGHT_RECEIVE:
		mp = mach_port_get();
		*mr = mach_right_get(mp, td, MACH_PORT_TYPE_RECEIVE, 0);
		break;

	case MACH_PORT_RIGHT_DEAD_NAME:
		*mr = mach_right_get(NULL, td, MACH_PORT_TYPE_DEAD_NAME, 0);
		break;

	case MACH_PORT_RIGHT_PORT_SET:
		*mr = mach_right_get(NULL, td, MACH_PORT_TYPE_PORT_SET, 0);
		break;

	default:
		uprintf("mach_port_allocate: unknown right %x\n",
				right);
		return (EINVAL);
		break;
	}
	return (0);
}

int
sys__kernelrpc_mach_port_allocate_trap(struct thread *td, struct _kernelrpc_mach_port_allocate_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; */
	struct mach_right *mr;
	int error;

	if ((error = mach_port_allocate(td, uap->right, &mr)) != 0)
		return (error);
	return (copyout(&mr->mr_name, uap->name, sizeof(*uap->name)));
}

int
mach_port_allocate_msg(struct mach_trap_args *args)
{
	mach_port_allocate_request_t *req = args->smsg;
	mach_port_allocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct mach_right *mr;
	int error;

	if ((error = mach_port_allocate(args->td, req->req_right, &mr)) != 0)
		return (mach_msg_error(args, EINVAL));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_name = (mach_port_name_t)mr->mr_name;

	mach_set_trailer(rep, *msglen);

	return (0);
}

static int
mach_port_insert_right(struct thread *td, mach_port_t name, mach_port_t right,
	mach_msg_type_name_t disposition)
{
	struct mach_right *mr;
	struct mach_right *nmr;

	mr = mach_right_check(right, td, MACH_PORT_TYPE_ALL_RIGHTS);
	if (mr == NULL)
		return (EPERM);

	switch (disposition) {
	case MACH_MSG_TYPE_MAKE_SEND:
	case MACH_MSG_TYPE_MOVE_SEND:
	case MACH_MSG_TYPE_COPY_SEND:
		nmr = mach_right_get(mr->mr_port,
		    td, MACH_PORT_TYPE_SEND, name);
		break;

	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	case MACH_MSG_TYPE_MOVE_SEND_ONCE:
		nmr = mach_right_get(mr->mr_port,
		    td, MACH_PORT_TYPE_SEND_ONCE, name);
		break;

	case MACH_MSG_TYPE_MOVE_RECEIVE:
		nmr = mach_right_get(mr->mr_port,
		    td, MACH_PORT_TYPE_RECEIVE, name);
		break;
	default:
		nmr = NULL;
		uprintf("mach_port_insert_right: unknown right %x\n",
		    disposition);
		break;
	}
	if (nmr == NULL)
		return (EPERM);
	return (0);
}

int
sys__kernelrpc_mach_port_insert_right_trap(struct thread *td, struct _kernelrpc_mach_port_insert_right_trap_args *uap)
{

	return (mach_port_insert_right(td, uap->name, uap->poly, uap->polyPoly));
}

int
mach_port_insert_right_msg(struct mach_trap_args *args)
{
	mach_port_insert_right_request_t *req = args->smsg;
	mach_port_insert_right_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *td = args->td;
	int error;
	mach_port_t name;
	mach_port_t right;


	name = req->req_name;
	right = req->req_poly.name;
	error = mach_port_insert_right(td, req->req_name, req->req_poly.name,
		req->req_poly.disposition);
	if (error)
		return (mach_msg_error(args, error));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_type(struct mach_trap_args *args)
{
	mach_port_type_request_t *req = args->smsg;
	mach_port_type_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *td = args->td;
	mach_port_t mn;
	struct mach_right *mr;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, td, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return (mach_msg_error(args, EPERM));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_ptype = mr->mr_type;

	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_set_attributes(struct mach_trap_args *args)
{
	mach_port_set_attributes_request_t *req = args->smsg;
	mach_port_set_attributes_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int end_offset;

	/* Sanity check req->req_count */
	end_offset = req->req_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_port_info[end_offset]))
		return (mach_msg_error(args, EINVAL));

	switch(req->req_flavor) {
	case MACH_PORT_LIMITS_INFO:
	case MACH_PORT_RECEIVE_STATUS:
	case MACH_PORT_DNREQUESTS_SIZE:
		break;
	default:
		uprintf("mach_port_set_attributes: unknown flavor %d\n",
		    req->req_flavor);
		break;
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_get_attributes(struct mach_trap_args *args)
{
	mach_port_get_attributes_request_t *req = args->smsg;
	mach_port_get_attributes_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *td = args->td;
	mach_port_t mn;
	struct mach_right *mr;

	/* Sanity check req_count */
	if (req->req_count > 10)
		return (mach_msg_error(args, EINVAL));

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, td, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return (mach_msg_error(args, EPERM));

	switch (req->req_flavor) {
	case MACH_PORT_LIMITS_INFO: {
		struct mach_port_limits *mpl;

		if (req->req_count < (sizeof(*mpl) / sizeof(rep->rep_info[0])))
			return (mach_msg_error(args, EINVAL));

		mpl = (struct mach_port_limits *)&rep->rep_info[0];
		mpl->mpl_qlimit = MACH_PORT_QLIMIT_DEFAULT; /* XXX fake limit */

		rep->rep_count = sizeof(*mpl);

		break;
	}

	case MACH_PORT_RECEIVE_STATUS: {
		struct mach_port_status *mps;
		struct mach_port *mp;

		if (req->req_count < (sizeof(*mps) / sizeof(rep->rep_info[0])))
			return (mach_msg_error(args, EINVAL));

		mps = (struct mach_port_status *)&rep->rep_info[0];
		memset(mps, 0, sizeof(*mps));

		if (mr->mr_sethead != NULL)
			mps->mps_pset = mr->mr_sethead->mr_name;
		mps->mps_seqno = 0; /* XXX */
		mps->mps_qlimit = MACH_PORT_QLIMIT_DEFAULT; /* XXX fake limit */
		if ((mp = mr->mr_port) != NULL) {
			mps->mps_mscount = mp->mp_refcount; /* XXX */
			mps->mps_msgcount = mp->mp_count;
		} else {
			mps->mps_mscount = 0;
			mps->mps_msgcount = 0;
		}
		mps->mps_sorights = 0; /* XXX */
		mps->mps_srights =  0; /* XXX */
		if (mr->mr_notify_destroyed != NULL)
			mps->mps_pdrequest = 1;
		if (mr->mr_notify_no_senders != NULL)
			mps->mps_nsrequest = 1;
		mps->mps_flags = 0; /* XXX */

		rep->rep_count = sizeof(*mps);
		break;
	}

	default:
		printf("mach_port_get_attributes: unknown flavor %d\n",
		    req->req_flavor);
		return (mach_msg_error(args, EINVAL));

		break;
	}

	*msglen = sizeof(*rep) - 10 + rep->rep_count;
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return (0);
}



/* XXX insert a recv right into a port set without removing it from another */
int
mach_port_insert_member(struct mach_trap_args *args)
{
	mach_port_insert_member_request_t *req = args->smsg;
	mach_port_insert_member_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	uprintf("Unimplemented mach_port_insert_member\n");

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	mach_set_trailer(rep, *msglen);
	return (0);
}

static int
mach_port_move_member(struct thread *td, mach_port_name_t member, mach_port_name_t after)
{
	struct mach_right *mrr;
	struct mach_right *mrs;
	struct mach_emuldata *med;

	mrr = mach_right_check(member, td, MACH_PORT_TYPE_RECEIVE);
	if (mrr == NULL)
		return (EPERM);

	mrs = mach_right_check(after, td, MACH_PORT_TYPE_PORT_SET);
	if (mrs == NULL)
		return (EPERM);

	med = td->td_proc->p_emuldata;
	rw_wlock(&med->med_rightlock);

	/* Remove it from an existing port set */
	if (mrr->mr_sethead != mrr)
		LIST_REMOVE(mrr, mr_setlist);

	/* Insert it into the new port set */
	LIST_INSERT_HEAD(&mrs->mr_set, mrr, mr_setlist);
	mrr->mr_sethead = mrs;

	rw_wunlock(&med->med_rightlock);
	return (0);
}

int
sys__kernelrpc_mach_port_move_member_trap(struct thread *td, struct _kernelrpc_mach_port_move_member_trap_args *uap)
{
	/* mach_port_name_t target = uap->target; */

	return (mach_port_move_member(td, uap->member, uap->after));
}

int
mach_port_move_member_msg(struct mach_trap_args *args)
{
	mach_port_move_member_request_t *req = args->smsg;
	mach_port_move_member_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int error;

	if ((error = mach_port_move_member(args->td, req->req_member, req->req_after)) != 0)
		return (mach_msg_error(args, error));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	rep->rep_retval = 0;
	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_request_notification(struct mach_trap_args *args)
{
	mach_port_request_notification_request_t *req = args->smsg;
	mach_port_request_notification_reply_t *rep = args->rmsg;
	struct thread *td = args->td;
	size_t *msglen = args->rsize;
	mach_port_t mn;
	struct mach_right *nmr;
	struct mach_right *tmr;
	struct mach_right *oldnmr;
	mach_port_t oldmn;

#ifdef DEBUG_MACH
	printf("mach_port_request_notification, notify = %08x, target = %08x\n",
	    req->req_notify.name, mn = req->req_name);
#endif
	mn = req->req_notify.name;
	if ((nmr = mach_right_check(mn, td, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return (mach_msg_error(args, EINVAL));

	mn = req->req_name;
	if ((tmr = mach_right_check(mn, td, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return (mach_msg_error(args, EINVAL));

#ifdef DEBUG_MACH
	if (nmr->mr_port == NULL) {
		printf("Notification right without a port\n");
		printf("mr->mr_port = %p, mr = %08x\n", nmr->mr_port, nmr->mr_name);
		return (mach_msg_error(args, EINVAL));
	}
#endif


	oldnmr = NULL;
	switch(req->req_msgid) {
	case MACH_NOTIFY_DESTROYED_MSGID:
		oldnmr = tmr->mr_notify_destroyed;
		tmr->mr_notify_destroyed = mach_right_get(nmr->mr_port,
		    td, MACH_PORT_TYPE_SEND_ONCE, req->req_notify.name);
		break;

	case MACH_NOTIFY_NO_SENDERS_MSGID:
		oldnmr = tmr->mr_notify_no_senders;
		tmr->mr_notify_no_senders = mach_right_get(nmr->mr_port,
		    td, MACH_PORT_TYPE_SEND_ONCE, req->req_notify.name);
		tmr->mr_notify_no_senders->mr_port->mp_datatype =
		    MACH_MP_NOTIFY_SYNC;
		tmr->mr_notify_no_senders->mr_port->mp_data = (void *)
		    (uintptr_t)req->req_count;
		break;

	case MACH_NOTIFY_DEAD_NAME_MSGID:
		oldnmr = tmr->mr_notify_dead_name;
		tmr->mr_notify_dead_name = mach_right_get(nmr->mr_port,
		    td, MACH_PORT_TYPE_SEND_ONCE, req->req_notify.name);
		break;

	case MACH_NOTIFY_SEND_ONCE_MSGID:
	case MACH_NOTIFY_DELETED_MSGID:
	default:
#ifdef DEBUG_MACH
		printf("unsupported notify request %d\n", req->req_msgid);
		return (mach_msg_error(args, EINVAL));
#endif
		break;
	}

	if (oldnmr != NULL) {
		oldnmr->mr_refcount++;
		oldmn = oldnmr->mr_name;
	} else {
		oldmn = (mach_port_t)MACH_PORT_NULL;
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, oldmn);
	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_get_refs(struct mach_trap_args *args)
{
	mach_port_get_refs_request_t *req = args->smsg;
	mach_port_get_refs_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct thread *td = args->td;
	mach_port_t mn;
	struct mach_right *mr;
	mach_port_right_t right = req->req_right;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, td, right)) == NULL)
		return (mach_msg_error(args, EINVAL));

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_refs = mr->mr_refcount;

	mach_set_trailer(rep, *msglen);

	return (0);
}

int
mach_port_mod_refs(struct mach_trap_args *args)
{
	mach_port_mod_refs_request_t *req = args->smsg;
	mach_port_mod_refs_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
#if 0
	struct thread *td = args->td;
	mach_port_t mn;
	struct mach_right *mr;
	mach_port_right_t right = req->req_right;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, td, right)) == NULL)
		return (mach_msg_error(args, EINVAL));

	/*
	 * Changing the refcount is likely to cause crashes,
	 * as we will free a right which might still be referenced
	 * within the kernel. Add a user refcount field?
	 */
	mr->mr_refcount += req->req_delta;
	if (mr->mr_refcount <= 0)
		mach_right_put(mr, right);
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return (0);
}

void
mach_port_init(void)
{

	mach_port_zone =
		uma_zcreate("mach_port_zone", sizeof (struct mach_port),
					NULL, NULL, NULL, NULL, 0/* align*/, 0/*flags*/);
	mach_right_zone =
		uma_zcreate("mach_right_zone", sizeof (struct mach_right),
					NULL, NULL, NULL, NULL, 0/* align*/, 0/*flags*/);

	mach_bootstrap_port = mach_port_get();
	mach_clock_port = mach_port_get();
	mach_io_master_port = mach_port_get();

	mach_bootstrap_port->mp_flags |= MACH_MP_INKERNEL;
	mach_clock_port->mp_flags |= MACH_MP_INKERNEL;
	mach_io_master_port->mp_flags |= MACH_MP_INKERNEL;

	mach_saved_bootstrap_port = mach_bootstrap_port;
}

struct mach_port *
mach_port_get(void)
{
	struct mach_port *mp;

	mp = uma_zalloc(mach_port_zone, M_WAITOK);
	memset(mp, 0, sizeof(*mp));
	mp->mp_recv = NULL;
	mp->mp_count = 0;
	mp->mp_flags = 0;
	mp->mp_datatype = MACH_MP_NONE;
	mp->mp_data = NULL;
	TAILQ_INIT(&mp->mp_msglist);
	rw_init(&mp->mp_msglock, "mach msg");

	return (mp);
}

void
mach_port_put(struct mach_port *mp)
{
	struct mach_message *mm;

#ifdef DIAGNOSTIC
	if (mp->mp_refcount > 0) {
		uprintf("mach_port_put: trying to free a referenced port\n");
		return;
	}
#endif

	rw_wlock(&mp->mp_msglock);
	while ((mm = TAILQ_FIRST(&mp->mp_msglist)) != NULL)
		mach_message_put_exclocked(mm);
	rw_wunlock(&mp->mp_msglock);
	rw_destroy(&mp->mp_msglock);

	if (mp->mp_flags & MACH_MP_DATA_ALLOCATED)
		free(mp->mp_data, M_MACH);

	uma_zfree(mach_port_zone, mp);
}

struct mach_right *
mach_right_get(struct mach_port *mp, struct thread *td, int type, mach_port_t hint)
{
	struct mach_right *mr;
	struct mach_emuldata *med;
	int rights;

#ifdef DEBUG_MACH
	if (type == 0)
		uprintf("mach_right_get: right = 0\n");
#endif
	med = (struct mach_emuldata *)td->td_proc->p_emuldata;

	if (mp != NULL)
		MACH_PORT_REF(mp);

	/* Send and receive right must return an existing right */
	rights = (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_RECEIVE);
	if (type & rights) {
		rw_rlock(&med->med_rightlock);
		LIST_FOREACH(mr, &med->med_right, mr_list) {
			if ((mr->mr_port == mp) && (mr->mr_type & rights))
				break;
		}
		rw_runlock(&med->med_rightlock);

		if (mr != NULL) {
			mr->mr_type |= type;
			if (type & MACH_PORT_TYPE_SEND)
				mr->mr_refcount++;
			KASSERT(mr->mr_port != NULL, ("port not set"));
			goto rcvck;
		}
	}

	mr = uma_zalloc(mach_right_zone, M_WAITOK|M_ZERO);

	/* XXX can move this to the zone allocator */
	mr->mr_port = mp;
	mtx_init(&mr->mr_lock, "mach right", NULL, MTX_DEF);
	mr->mr_td = td;
	mr->mr_type = type;
	mr->mr_sethead = mr;
	mr->mr_refcount = 1;
	mr->mr_notify_destroyed = NULL;
	mr->mr_notify_dead_name = NULL;
	mr->mr_notify_no_senders = NULL;

	LIST_INIT(&mr->mr_set);

	/* Insert the right in the right lists */
	if (type & MACH_PORT_TYPE_ALL_RIGHTS) {
		rw_wlock(&med->med_rightlock);
		mr->mr_name = mach_right_newname(td, hint);
#ifdef DEBUG_MACH_RIGHT
		printf("mach_right_get: insert right %x(%x)\n",
		    mr->mr_name, mr->mr_type);
#endif
		LIST_INSERT_HEAD(&med->med_right, mr, mr_list);
		rw_wunlock(&med->med_rightlock);
	}

rcvck:
	if (type & MACH_PORT_TYPE_RECEIVE) {
		/*
		 * Destroy the former receive right on this port, and
		 * register the new right.
		 */
		if (mr->mr_port->mp_recv != NULL &&
			mr->mr_port->mp_recv != mr)
			mach_right_put(mr->mr_port->mp_recv,
			    MACH_PORT_TYPE_RECEIVE);
		mr->mr_port->mp_recv = mr;
	}
	return (mr);
}

void
mach_right_put(struct mach_right *mr, int right)
{
	struct mach_emuldata *med = mr->mr_td->td_proc->p_emuldata;

	rw_wlock(&med->med_rightlock);
	mach_right_put_exclocked(mr, right);
	rw_wunlock(&med->med_rightlock);
}

void
mach_right_put_shlocked(struct mach_right *mr, int right)
{
	struct mach_emuldata *med = mr->mr_td->td_proc->p_emuldata;

	if (!rw_try_upgrade(&med->med_rightlock)) {
		/* XXX */
		rw_runlock(&med->med_rightlock);
		rw_wlock(&med->med_rightlock);
	}
	mach_right_put_exclocked(mr, right);
	rw_downgrade(&med->med_rightlock);
}

void
mach_right_put_exclocked(struct mach_right *mr, int right)
{
	struct mach_right *cmr;
	struct mach_emuldata *med;
	int lright;
	int kill_right;

	med = mr->mr_td->td_proc->p_emuldata;

#ifdef DEBUG_MACH_RIGHT
	printf("mach_right_put: mr = %p\n", mr);
	printf("right %x(%x) refcount %d, deallocate %x\n",
	    mr->mr_name, mr->mr_type, mr->mr_refcount, right);
	if ((mr->mr_type & right) == 0)
		printf("mach_right_put: dropping nonexistant right %x on %x\n",
		    right, mr->mr_name);
	LIST_FOREACH(cmr, &med->med_right, mr_list)
		if (cmr == mr)
			break;
	if (cmr == NULL) {
		printf("mach_right_put: dropping already dropped right %x\n",
		    mr->mr_name);
		return;
	}
#endif
	kill_right = 0;

	/* When receive right is deallocated, the port should die */
	lright = (right & MACH_PORT_TYPE_RECEIVE);
#ifdef DEBUG_MACH_RIGHT
	printf("mr->mr_type = %x, lright = %x, right = %x, refcount = %d\n",
	    mr->mr_type, lright, right, mr->mr_refcount);
#endif
	if (mr->mr_type & lright) {
		if (mr->mr_refcount <= 0) {
			mr->mr_type &= ~MACH_PORT_TYPE_RECEIVE;
			kill_right = 1;
		} else {
			mr->mr_type &= ~MACH_PORT_TYPE_RECEIVE;
			mr->mr_type |= MACH_PORT_TYPE_DEAD_NAME;
			mach_notify_port_dead_name(mr->mr_td, mr);
		}
		if (mr->mr_port != NULL) {
			/* There is no more receiver */
#ifdef DIAGNOSTIC
			if (mr->mr_port->mp_recv != mr)
				printf("several receiver on a single port\n");
#endif
			mr->mr_port->mp_recv = NULL;

			MACH_PORT_UNREF(mr->mr_port);
			mr->mr_port = NULL;
		}
	}

	/* send, send_once and dead_name */
	lright = (right & MACH_PORT_TYPE_REF_RIGHTS);
	if (mr->mr_type & lright) {
		mr->mr_refcount--;

		mach_notify_port_no_senders(mr->mr_td, mr);

		if (mr->mr_refcount <= 0) {
			mr->mr_type &= ~MACH_PORT_TYPE_REF_RIGHTS;
			if ((mr->mr_type & MACH_PORT_TYPE_RECEIVE) == 0)
				kill_right = 1;
		}
	}

	lright = (right & MACH_PORT_TYPE_PORT_SET);
	if ((mr->mr_type & lright) || (kill_right == 1)) {
		while ((cmr = LIST_FIRST(&mr->mr_set)) != NULL) {
			LIST_REMOVE(cmr, mr_setlist);
			cmr->mr_sethead = cmr;
		}
		mr->mr_type &= ~MACH_PORT_TYPE_PORT_SET;
		if ((mr->mr_type & MACH_PORT_TYPE_RECEIVE) == 0)
			kill_right = 1;
	}

	/* Should we kill it? */
	if (kill_right == 1) {
#ifdef DEBUG_MACH_RIGHT
		printf("mach_right_put: kill name %x\n", mr->mr_name);
#endif
		/* If the right is used for an IO notification, remove it */
		mach_iokit_cleanup_notify(mr);

		mach_notify_port_destroyed(mr->mr_td, mr);
		LIST_REMOVE(mr, mr_list);
		uma_zfree(mach_right_zone, mr);
	}
}

/*
 * Check that a process has a given right.
 */
struct mach_right *
mach_right_check(mach_port_t mn, struct thread *td, int type)
{
	struct mach_right *cmr;
	struct mach_emuldata *med;

	if ((mn == 0) || (mn == -1) || (td == NULL))
		return (NULL);

	med = (struct mach_emuldata *)td->td_proc->p_emuldata;

	rw_rlock(&med->med_rightlock);

#ifdef DEBUG_MACH_RIGHT
	printf("mach_right_check: type = %x, mn = %x\n", type, mn);
#endif
	LIST_FOREACH(cmr, &med->med_right, mr_list) {
#ifdef DEBUG_MACH_RIGHT
		printf("cmr = %p, cmr->mr_name = %x, cmr->mr_type = %x\n",
		    cmr, cmr->mr_name, cmr->mr_type);
#endif
		if (cmr->mr_name != mn)
			continue;
		if (type & cmr->mr_type)
			break;
	}

	rw_runlock(&med->med_rightlock);

	return (cmr);
}


/*
 * Find an usnused port name in a given thread.
 * Right lists should be locked.
 */
mach_port_t
mach_right_newname(struct thread *td, mach_port_t hint)
{
	struct mach_emuldata *med;
	struct mach_right *mr;
	mach_port_t newname = -1;

	med = td->td_proc->p_emuldata;

	if (hint == 0)
		hint = med->med_nextright;

	while (newname == -1) {
		LIST_FOREACH(mr, &med->med_right, mr_list)
			if (mr->mr_name == hint)
				break;
		if (mr == NULL)
			newname = hint;
		hint++;
	}

	med->med_nextright = hint;

	return (newname);
}

#ifdef DEBUG_MACH
void
mach_debug_port(void)
{
#if 0
	struct mach_emuldata *med;
	struct mach_right *mr;
	struct mach_right *mrs;
	struct proc *p;

	PROCLIST_FOREACH(p, &allproc) {
		if ((p->p_emul != &emul_mach) &&
#ifdef COMPAT_DARWIN
		    (p->p_emul != &emul_darwin) &&
#endif
		    1)
			continue;

		med = p->p_emuldata;
		LIST_FOREACH(mr, &med->med_right, mr_list) {
			if ((mr->mr_type & MACH_PORT_TYPE_PORT_SET) == 0) {
				printf("pid %d: %p(%x)=>%p",
				    p->p_pid, mr, mr->mr_type, mr->mr_port);
				if (mr->mr_port && mr->mr_port->mp_recv)
					printf("[%p]\n",
					    mr->mr_port->mp_recv->mr_sethead);
				else
					printf("[NULL]\n");

				continue;
			}

			/* Port set... */
			printf("pid %d: set %p(%x) ",
			    p->p_pid, mr, mr->mr_type);
			LIST_FOREACH(mrs, &mr->mr_set, mr_setlist) {
				printf("%p(%x)=>%p",
				    mrs, mrs->mr_type, mrs->mr_port);
				if (mrs->mr_port && mrs->mr_port->mp_recv)
					printf("[%p]",
					    mrs->mr_port->mp_recv->mr_sethead);
				else
					printf("[NULL]");

				printf(" ");
			}
			printf("\n");
		}
	}
#endif
}

#endif /* DEBUG_MACH */
