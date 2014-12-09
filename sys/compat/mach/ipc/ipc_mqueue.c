/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/* CMU_HIST */
/*
 * Revision 2.17.3.7  92/09/15  17:20:54  jeffreyh
 * 	Added code to detect if a race condition was lost in the NORMA
 * 	system  and our port is now a proxy (migrated out from under the
 * 	thread), in which case we return MACH_RCV_PORT_CHANGED.
 * 	[92/07/24            sjs]
 * 
 * Revision 2.17.3.6  92/06/24  17:59:57  jeffreyh
 * 	Allow norma_ipc_send to fail.
 * 	[92/06/02            dlb]
 * 
 * Revision 2.17.3.5  92/05/27  00:44:23  jeffreyh
 * 	In ipc_mqueue_receive, check whether a kmsg got queued while we called
 * 	netipc_replenish.
 * 	[92/05/12            dlb]
 * 
 * Revision 2.17.3.4.2.1  92/04/28  16:01:59  jeffreyh
 * 	Fixed race condition in NORMA system for ipc_mqueue_send().
 * 	[92/04/16            sjs]
 * 
 * Revision 2.17.3.4  92/03/28  10:09:17  jeffreyh
 * 	In error cases call norma_ipc_error_receiving instead
 * 	of norma_ipc_finish receiving.  This may eat the kmsg.
 * 	[92/03/20		dlb]
 * 
 * Revision 2.17.3.3  92/03/03  16:18:53  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  11:51:40  jeffreyh]
 * 
 * Revision 2.18  92/01/03  20:13:05  dbg
 * 	Removed THREAD_SHOULD_TERMINATE.
 * 	[91/12/19            dbg]
 * 
 * Revision 2.17.3.2  92/01/21  21:50:26  jsb
 * 	Picked up hack from dlb@osf.org to call norma_ipc_finish_receiving
 * 	before ipc_kmsg_destroy. The real fix is to use uncopyout_to_network.
 * 	[92/01/17  14:35:03  jsb]
 * 
 * Revision 2.17.3.1  92/01/03  16:35:24  jsb
 * 	Removed spurious arguments to norma_ipc_send.
 * 	Options and timeout will be handled here, not by norma_ipc_send.
 * 	[91/12/26  19:51:59  jsb]
 * 
 * 	Corrected log.
 * 	[91/12/24  14:15:11  jsb]
 *
 * Revision 2.17  91/12/15  10:40:33  jsb
 * 	Added norma_ipc_finish_receiving call to support large in-line msgs.
 * 
 * Revision 2.16  91/12/14  14:27:10  jsb
 * 	Removed ipc_fields.h hack.
 * 
 * Revision 2.15  91/11/14  16:56:07  rpd
 * 	Picked up mysterious norma changes.
 * 	[91/11/14            rpd]
 * 
 * Revision 2.14  91/08/28  11:13:34  jsb
 * 	Added seqno argument to ipc_mqueue_receive.
 * 	Also added seqno processing to ipc_mqueue_send, ipc_mqueue_move.
 * 	[91/08/10            rpd]
 * 	Fixed norma_ipc_handoff technology.
 * 	Renamed clport things to norma_ipc things.
 * 	[91/08/15  08:23:17  jsb]
 * 
 * Revision 2.13  91/08/03  18:18:27  jsb
 * 	Renamed replenish routine.
 * 	[91/08/01  23:00:06  jsb]
 * 
 * 	Removed obsolete include.
 * 	Added option, timeout parameters to ipc_clport_send.
 * 	[91/07/17  14:04:15  jsb]
 * 
 * Revision 2.12  91/06/25  10:27:34  rpd
 * 	Added some wait_result assertions.
 * 	[91/05/30            rpd]
 * 
 * Revision 2.11  91/06/17  15:46:18  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  10:44:39  jsb]
 * 
 * Revision 2.10  91/06/06  17:06:06  jsb
 * 	Added call to ip_unlock after calling ipc_clport_send.
 * 	Added support for clport handoff.
 * 	[91/06/06  16:05:12  jsb]
 * 
 * Revision 2.9  91/05/14  16:33:58  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/03/16  14:48:18  rpd
 * 	Renamed ipc_thread_{go,will_wait,will_wait_with_timeout}
 * 	to thread_{go,will_wait,will_wait_with_timeout}.
 * 	Replaced ipc_thread_block with thread_block.
 * 	[91/02/17            rpd]
 * 
 * Revision 2.7  91/02/05  17:22:24  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  15:46:33  mrt]
 * 
 * Revision 2.6  91/01/08  15:14:35  rpd
 * 	Changed continuation argument to (void (*)()).
 * 	[90/12/18            rpd]
 * 	Reorganized ipc_mqueue_receive.
 * 	[90/11/22            rpd]
 * 
 * 	Minor cleanup.
 * 	[90/11/11            rpd]
 * 
 * Revision 2.5  90/12/14  11:02:32  jsb
 * 	Changed parameters in ipc_clport_send call.
 * 	[90/12/13  21:20:13  jsb]
 * 
 * Revision 2.4  90/11/05  14:29:04  rpd
 * 	Use new io_reference and io_release.
 * 	Use new ip_reference and ip_release.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.3  90/09/28  16:54:58  jsb
 * 	Added NORMA_IPC support.
 * 	[90/09/28  14:03:24  jsb]
 * 
 * Revision 2.2  90/06/02  14:50:39  rpd
 * 	Created for new IPC.
 * 	[90/03/26  20:57:06  rpd]
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File:	ipc/ipc_mqueue.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions to manipulate IPC message queues.
 */

#if 0
#include <dipc.h>
#endif


#include <sys/mach/port.h>
#include <sys/mach/message.h>
#if 0
#include <kern/assert.h>
#include <kern/counters.h>
#include <kern/ipc_sched.h>
#include <kern/misc_protos.h>
#endif
#include <sys/mach/ipc_kobject.h>

#include <sys/mach/ipc/ipc_mqueue.h>
#include <sys/mach/ipc/ipc_thread.h>
#include <sys/mach/ipc/ipc_kmsg.h>
#include <sys/mach/ipc/ipc_port.h>
#include <sys/mach/ipc/ipc_pset.h>
#include <sys/mach/ipc/ipc_space.h>
#include <sys/mach/thread.h>


#define TR_ENABLE 0
#define	dstat_decl(foo)
#define	dstat(foo)

/*
 *	Routine:	ipc_mqueue_init
 *	Purpose:
 *		Initialize a newly-allocated message queue.
 */

void
ipc_mqueue_init(
	ipc_mqueue_t	mqueue)
{
	imq_lock_init(mqueue);
	ipc_kmsg_queue_init(&mqueue->imq_messages);
	ipc_thread_queue_init(&mqueue->imq_threads);
	knlist_init(&mqueue->imq_note, &mqueue->imq_lock_data, NULL, NULL, NULL, NULL);
}

/*
 *	Routine:	ipc_mqueue_move
 *	Purpose:
 *		Move messages from one queue (source) to another (dest).
 *		Only moves messages sent to the specified port.
 *	Conditions:
 *		Both queues must be locked.
 *		(This is sufficient to manipulate port->ip_seqno.)
 */

void
ipc_mqueue_move(
	ipc_mqueue_t	dest,
	ipc_mqueue_t	source,
	ipc_port_t	port)
{
	ipc_kmsg_queue_t oldq, newq;
	ipc_thread_queue_t blockedq;
	ipc_kmsg_t kmsg, next;
	ipc_thread_t th;

	oldq = &source->imq_messages;
	newq = &dest->imq_messages;
	blockedq = &dest->imq_threads;

	for (kmsg = ipc_kmsg_queue_first(oldq); kmsg != IKM_NULL; kmsg = next) {
		next = ipc_kmsg_queue_next(oldq, kmsg);

		/* only move messages sent to port */

		if (kmsg->ikm_header.msgh_remote_port != (mach_port_t) port)
			continue;

		ipc_kmsg_rmqueue(oldq, kmsg);

		/* before adding kmsg to newq, check for a blocked receiver */

		if ((th = ipc_thread_dequeue(blockedq)) != ITH_NULL) {
			assert(ipc_kmsg_queue_empty(newq));

			/*
			 * Got a receiver.  Hand the receiver the message
			 * and process the next one.
			 */
			th->ith_state = MACH_MSG_SUCCESS;
			th->ith_kmsg = kmsg;
			th->ith_seqno = port->ip_seqno++;
			thread_go(th);
		} else {
			/* didn't find a receiver; enqueue the message */
			ipc_kmsg_enqueue(newq, kmsg);
		}
	}
}

/*
 *	Routine:	ipc_mqueue_changed
 *	Purpose:
 *		Wake up receivers waiting in a message queue.
 *	Conditions:
 *		The message queue is locked.
 */

void
ipc_mqueue_changed(
	ipc_mqueue_t		mqueue,
	mach_msg_return_t	mr)
{
	ipc_thread_t th;

	while ((th = ipc_thread_dequeue(&mqueue->imq_threads)) != ITH_NULL) {
		th->ith_state = mr;
		thread_go(th);
	}
}

/*
 *	Routine:	ipc_mqueue_send
 *	Purpose:
 *		Send a message to a port.  The message holds a reference
 *		for the destination port in the msgh_remote_port field.
 *
 *		If unsuccessful, the caller still has possession of
 *		the message and must do something with it.  If successful,
 *		the message is queued, given to a receiver, destroyed,
 *		or handled directly by the kernel via mach_msg.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	The message was accepted.
 *		MACH_SEND_TIMED_OUT	Caller still has message.
 *		MACH_SEND_INTERRUPTED	Caller still has message.
 */

mach_msg_return_t
ipc_mqueue_send(
	ipc_kmsg_t		kmsg,
	mach_msg_option_t	option,
	mach_msg_timeout_t	timeout)
{
	ipc_port_t port;
	kern_return_t           save_wait_result;

	port = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
	assert(IP_VALID(port));

	ip_lock(port);

	if (port->ip_receiver == ipc_space_kernel) {
		ipc_kmsg_t reply;

		/*
		 *	We can check ip_receiver == ipc_space_kernel
		 *	before checking that the port is active because
		 *	ipc_port_dealloc_kernel clears ip_receiver
		 *	before destroying a kernel port.
		 */

		assert(ip_active(port));
		ip_unlock(port);

		reply = ipc_kobject_server(kmsg);
		if (reply != IKM_NULL)
			ipc_mqueue_send_always(reply);

		return MACH_MSG_SUCCESS;
	}

	if (kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_CIRCULAR) {
		ip_unlock(port);

		/* don't allow the creation of a circular loop */

		ipc_kmsg_destroy(kmsg);
		return MACH_MSG_SUCCESS;
	}

	for (;;) {
		ipc_thread_t self;

		/*
		 *	Can't deliver to a dead port.
		 *	However, we can pretend it got sent
		 *	and was then immediately destroyed.
		 */

		if (!ip_active(port)) {
			/*
			 *	We can't let ipc_kmsg_destroy deallocate
			 *	the port right, because we might end up
			 *	in an infinite loop trying to deliver
			 *	a send-once notification.
			 */

			ip_release(port);
			ip_check_unlock(port);
			kmsg->ikm_header.msgh_remote_port = MACH_PORT_NULL;
			ipc_kmsg_destroy(kmsg);
			return MACH_MSG_SUCCESS;
		}

		/*
		 *  Don't block if:
		 *	1) We're under the queue limit.
		 *	2) Caller used the MACH_SEND_ALWAYS internal option.
		 *	3) Message is sent to a send-once right.
		 */

		if ((port->ip_msgcount < port->ip_qlimit) ||
		    (option & MACH_SEND_ALWAYS) ||
		    (MACH_MSGH_BITS_REMOTE(kmsg->ikm_header.msgh_bits) ==
						MACH_MSG_TYPE_PORT_SEND_ONCE))
			break;

		/* must block waiting for queue to clear */

		self = current_thread();

		if (option & MACH_SEND_TIMEOUT) {
			if (timeout == 0) {
				ip_unlock(port);
				return MACH_SEND_TIMED_OUT;
			}

			self->ith_state = MACH_SEND_IN_PROGRESS;
			ipc_thread_enqueue(&port->ip_blocked, self);

			thread_will_wait_with_timeout(self, timeout);

		} else {

			self->ith_state = MACH_SEND_IN_PROGRESS;
			ipc_thread_enqueue(&port->ip_blocked, self);

			thread_will_wait(self);
		}

	 	ip_unlock(port);
		counter(c_ipc_mqueue_send_block++);
		thread_block((void (*)(void)) 0);

		/* Save proper wait_result in case we block */
		save_wait_result = self->wait_result;
		if (option & MACH_SEND_TIMEOUT) {
			/* XXX */
#if 0
			callout_reset(&current_thread()->timer, timeout*hz, NULL);
#endif			
		}
		ip_lock(port);

		/* why did we wake up? */

		if (self->ith_state == MACH_MSG_SUCCESS)
			continue;
		assert(self->ith_state == MACH_SEND_IN_PROGRESS);

		/*
		 *	Thread wakeup-reason field tells us why
		 *	the wait was interrupted.
		 */

		switch (save_wait_result) {
		    case THREAD_INTERRUPTED:
			/* send was interrupted - give up */

			ip_unlock(port);
			return MACH_SEND_INTERRUPTED;

		    case THREAD_TIMED_OUT:
			/* timeout expired */

			assert(option & MACH_SEND_TIMEOUT);
			timeout = 0;
			break;

		    case THREAD_RESTART:
		    default:
			panic("ipc_mqueue_send");
		}
	}

	(void) ipc_mqueue_deliver(port, kmsg, TRUE);

	return MACH_MSG_SUCCESS;
}


/*
 * ipc_mqueue_deliver: give the kmsg to a waiting receiver or else enqueue
 * it on the port or pset.
 *
 * ipc_mqueue_deliver used to be the last block of code in ipc_mqueue_send.
 * It is split out here so that the same code can be used to deliver DIPC
 * kmsgs or meta_kmsgs.
 *
 * The port must be locked on entry; it will be unlocked on exit.  This
 * routine CAN NOT FAIL when called from thread context.  However, from
 * interrupt context it may not be able to complete its duties due to
 * lock contention.
 *
 * Returns TRUE on successful delivery.
 */
#if	TRACE_BUFFER
int	tr_ipc_mqueue_deliver = 0;
#define	TR_IPC_MQEN(fmt, port, kmsg, thread_context)			\
	tr_start();							\
	if (tr_ipc_mqueue_deliver)					\
		tr4((fmt), (port), (kmsg), (thread_context));
#define	TR_IPC_MQEX(fmt, kmsg)						\
	if (tr_ipc_mqueue_deliver)					\
		tr2((fmt), (kmsg));					\
	tr_stop();
#else	/* TRACE_BUFFER */
#define	TR_IPC_MQEN(fmt, port, kmsg, thread_context)
#define	TR_IPC_MQEX(fmt, kmsg)
#endif	/* TRACE_BUFFER */

dstat_decl(unsigned int	c_imd_waiting_receiver = 0;)
dstat_decl(unsigned int	c_imd_enqueued = 0;)

mach_msg_return_t
ipc_mqueue_deliver(
	register ipc_port_t	port,
	register ipc_kmsg_t	kmsg,
	boolean_t		thread_context)
{
	ipc_mqueue_t mqueue;
	ipc_pset_t pset;
	ipc_thread_t receiver;
	ipc_thread_queue_t receivers;
	TR_DECL("ipc_mqueue_deliver");

	TR_IPC_MQEN("enter: port 0x%x kmsg 0x%x thd_ctxt %d", port, kmsg,
		    thread_context);

	assert(IP_VALID(port));
	assert(ip_active(port));

	{
		pset = port->ip_pset;
		if (pset == IPS_NULL)
			mqueue = &port->ip_messages;
		else
			mqueue = &pset->ips_messages;
			imq_lock(mqueue);
	}

	port->ip_msgcount++;
	assert(port->ip_msgcount > 0);

	receivers = &mqueue->imq_threads;

	/*
	 *	Can unlock the port now that the msg queue is locked
	 *	and we know the port is active.  While the msg queue
	 *	is locked, we have control of the kmsg, so the ref in
	 *	it for the port is still good.  If the msg queue is in
	 *	a set (dead or alive), then we're OK because the port
	 *	is still a member of the set and the set won't go away
	 *	until the port is taken out, which tries to lock the
	 *	set's msg queue to remove the port's msgs.
	 */

	ip_unlock(port);


	/* Check for a receiver for the message. */

	receiver = ipc_thread_queue_first(receivers);

	if (receiver == ITH_NULL) {
		/* no receivers; queue kmsg */
		dstat(++c_imd_enqueued);
		ipc_kmsg_enqueue_macro(&mqueue->imq_messages, kmsg);
		imq_unlock(mqueue);
		TR_IPC_MQEX("exit: no receiver", 0);
		return MACH_MSG_SUCCESS;
	}

	dstat(++c_imd_waiting_receiver);
	ipc_thread_rmqueue_first_macro(receivers, receiver);
	assert(ipc_kmsg_queue_empty(&mqueue->imq_messages));

	disable_preemption();

	/*
	 * Got a valid receiver.  Hand the receiver the message.
	 */

	receiver->ith_state = MACH_MSG_SUCCESS;
	receiver->ith_kmsg = kmsg;
	receiver->ith_seqno = port->ip_seqno++;

	imq_unlock(mqueue);

	thread_go(receiver);
	enable_preemption();

	TR_IPC_MQEX("exit: wakeup 0x%x", receiver);
	return MACH_MSG_SUCCESS;
}


/*
 *	Routine:	ipc_mqueue_copyin
 *	Purpose:
 *		Convert a name in a space to a message queue.
 *	Conditions:
 *		Nothing locked.  If successful, the message queue
 *		is returned locked and caller gets a ref for the object.
 *		This ref ensures the continued existence of the queue.
 *	Returns:
 *		MACH_MSG_SUCCESS	Found a message queue.
 *		MACH_RCV_INVALID_NAME	The space is dead.
 *		MACH_RCV_INVALID_NAME	The name doesn't denote a right.
 *		MACH_RCV_INVALID_NAME
 *			The denoted right is not receive or port set.
 *		MACH_RCV_IN_SET		Receive right is a member of a set.
 */

mach_msg_return_t
ipc_mqueue_copyin(
	ipc_space_t	space,
	mach_port_name_t	name,
	ipc_mqueue_t	*mqueuep,
	ipc_object_t	*objectp)
{
	ipc_entry_t entry;
	ipc_entry_bits_t bits;
	ipc_object_t object;
	ipc_mqueue_t mqueue;

	is_read_lock(space);
	if (!space->is_active) {
		is_read_unlock(space);
		return MACH_RCV_INVALID_NAME;
	}

	entry = ipc_entry_lookup(space, name);
	if (entry == IE_NULL) {
		is_read_unlock(space);
		return MACH_RCV_INVALID_NAME;
	}

	bits = entry->ie_bits;
	object = entry->ie_object;

	if (bits & MACH_PORT_TYPE_RECEIVE) {
		ipc_port_t port;
		ipc_pset_t pset;

		port = (ipc_port_t) object;
		assert(port != IP_NULL);

		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);
		is_read_unlock(space);

		pset = port->ip_pset;
		if (pset != IPS_NULL) {
			ips_lock(pset);
			if (ips_active(pset)) {
				ips_unlock(pset);
				ip_unlock(port);
				return MACH_RCV_IN_SET;
			}

			ipc_pset_remove(pset, port);
			ips_check_unlock(pset);
			assert(port->ip_pset == IPS_NULL);
		}

		mqueue = &port->ip_messages;
	} else if (bits & MACH_PORT_TYPE_PORT_SET) {
		ipc_pset_t pset;

		pset = (ipc_pset_t) object;
		assert(pset != IPS_NULL);

		ips_lock(pset);
		assert(ips_active(pset));
		assert(pset->ips_local_name == name);
		is_read_unlock(space);

		mqueue = &pset->ips_messages;
	} else {
		is_read_unlock(space);
		return MACH_RCV_INVALID_NAME;
	}

	/*
	 *	At this point, the object is locked and active,
	 *	the space is unlocked, and mqueue is initialized.
	 */

	io_reference(object);
	imq_lock(mqueue);
	io_unlock(object);

	*objectp = object;
	*mqueuep = mqueue;
	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	ipc_mqueue_receive
 *	Purpose:
 *		Receive a message from a message queue.
 *
 *		If continuation is non-zero, then we might discard
 *		our kernel stack when we block.  We will continue
 *		after unblocking by executing continuation.
 *
 *		If resume is true, then we are resuming a receive
 *		operation after a blocked receive discarded our stack.
 *	Conditions:
 *		The message queue is locked; it will be returned unlocked.
 *
 *		Our caller must hold a reference for the port or port set
 *		to which this queue belongs, to keep the queue
 *		from being deallocated.  Furthermore, the port or set
 *		must have been active when the queue was locked.
 *
 *		The kmsg is returned with clean header fields
 *		and with the circular bit turned off.
 *	Returns:
 *		MACH_MSG_SUCCESS	Message returned in kmsgp.
 *		MACH_RCV_TOO_LARGE	Message size returned in kmsgp.
 *		MACH_RCV_TIMED_OUT	No message obtained.
 *		MACH_RCV_INTERRUPTED	No message obtained.
 *		MACH_RCV_PORT_DIED	Port/set died; no message.
 *		MACH_RCV_PORT_CHANGED	Port moved into set; no msg.
 *
 */

dstat_decl(unsigned int	c_imr_message_present = 0;)
dstat_decl(unsigned int	c_imr_block = 0;)
dstat_decl(unsigned int c_imr_calls = 0;)
dstat_decl(unsigned int c_imr_connect_reply_race = 0;)
dstat_decl(unsigned int c_imfr_calls = 0;)

mach_msg_return_t
ipc_mqueue_receive(
	ipc_mqueue_t		mqueue,
	mach_msg_option_t	option,
	mach_msg_size_t		max_size,
	mach_msg_timeout_t	timeout,
	boolean_t		resume,
	void			(*continuation)(void),
	ipc_kmsg_t		*kmsgp,
	mach_port_seqno_t	*seqnop)
{
	ipc_port_t port;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;
	ipc_kmsg_queue_t kmsgs = &mqueue->imq_messages;
	ipc_thread_t self = current_thread();
	kern_return_t	save_wait_result;

#if	NCPUS > 1 && defined(PARAGON860) && defined(ASMP)
	MUST_BE_MASTER("ipc_mqueue_receive()");
#endif
	assert( !resume );
	dstat(++c_imr_calls);

	for (;;) {
		kmsg = ipc_kmsg_queue_first(kmsgs);
		if (kmsg != IKM_NULL) {
			dstat(++c_imr_message_present);
			ipc_kmsg_rmqueue_first_macro(kmsgs, kmsg);
			port = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
			seqno = port->ip_seqno++;
			break;
		}

		dstat(++c_imr_block;)

		/* must block waiting for a message */

		if (option & MACH_RCV_TIMEOUT) {
			if (timeout == 0) {
				imq_unlock(mqueue);
				return MACH_RCV_TIMED_OUT;
			}

			self->ith_state = MACH_RCV_IN_PROGRESS_TIMED;
			self->ith_msize = max_size;
			ipc_thread_enqueue_macro(&mqueue->imq_threads, self);

			thread_will_wait_with_timeout(self, timeout);

		} else {

			self->ith_state = MACH_RCV_IN_PROGRESS;
			self->ith_msize = max_size;
			ipc_thread_enqueue_macro(&mqueue->imq_threads, self);

			thread_will_wait(self);
		}

		imq_unlock(mqueue);
		if (continuation != (void (*)(void)) 0) {
			counter(c_ipc_mqueue_receive_block_user++);
		} else {
			counter(c_ipc_mqueue_receive_block_kernel++);
		}
		thread_block(continuation);
		if (option & MACH_RCV_TIMEOUT) {
			/* XXX */
		}

		/* Save proper wait_result in case we block */
		save_wait_result = self->wait_result;
		imq_lock(mqueue);

		/* why did we wake up? */

		if (self->ith_state == MACH_MSG_SUCCESS) {
			/* pick up the message that was handed to us */
			kmsg = self->ith_kmsg;
			seqno = self->ith_seqno;
			port = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
			break;
		}

		switch (self->ith_state) {
		    case MACH_RCV_PORT_DIED:
		    case MACH_RCV_PORT_CHANGED:
			/* something bad happened to the port/set */

			imq_unlock(mqueue);
			return self->ith_state;

		    case MACH_RCV_IN_PROGRESS:
		    case MACH_RCV_IN_PROGRESS_TIMED:
			/*
			 *	Awakened for other than IPC completion.
			 *	Remove ourself from the waiting queue,
			 *	then check the wakeup cause.
			 */

			ipc_thread_rmqueue(&mqueue->imq_threads, self);

			switch (save_wait_result) {
			    case THREAD_INTERRUPTED:
				/* receive was interrupted - give up */

				imq_unlock(mqueue);
				return MACH_RCV_INTERRUPTED;

			    case THREAD_TIMED_OUT:
				/* timeout expired */

				assert(option & MACH_RCV_TIMEOUT);
				assert(self->ith_state == 
				       MACH_RCV_IN_PROGRESS_TIMED); 

				timeout = 0;
				break;

			    case THREAD_RESTART:
			    default:
				panic("ipc_mqueue_receive: bad wait_result");
			}
			break;

		    default:
			panic("ipc_mqueue_receive: strange ith_state");
		}
	}

	/* we have a kmsg; unlock the msg queue */

	imq_unlock(mqueue);

	*kmsgp = kmsg;
	*seqnop = seqno;

	mr = ipc_mqueue_finish_receive(kmsgp, port, option, max_size);
	return mr;
}

mach_msg_return_t
ipc_mqueue_finish_receive(
	ipc_kmsg_t		*kmsgp,
	ipc_port_t		port,
	mach_msg_option_t	option,
	mach_msg_size_t		max_size)
{
	ipc_kmsg_t		kmsg;
	mach_msg_return_t	mr;
	ipc_thread_t 		self = current_thread();

	mr = MACH_MSG_SUCCESS;
	kmsg = *kmsgp;
	dstat(++c_imfr_calls);

	/* check sizes */
	if (mr == MACH_MSG_SUCCESS) {
		if (kmsg->ikm_header.msgh_size >
				(max_size - REQUESTED_TRAILER_SIZE(option))) {
			/* the receive buffer isn't large enough */
			mr = MACH_RCV_TOO_LARGE;
		} else if (self->ith_scatter_list != MACH_MSG_BODY_NULL) {
			/* verify the scatter list */
			mr = ipc_kmsg_check_scatter(kmsg,
					   	self->ith_option,
						&self->ith_scatter_list, 
						&self->ith_scatter_list_size);
		}
	}

	if (mr == MACH_MSG_SUCCESS) {
		assert((kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_CIRCULAR)
									== 0);
		assert(port == (ipc_port_t) kmsg->ikm_header.msgh_remote_port);
	}

	ip_lock(port);

	if (ip_active(port)) {
		ipc_thread_queue_t senders;
		ipc_thread_t sender;

		assert(port->ip_msgcount > 0);
		port->ip_msgcount--;

		{
			senders = &port->ip_blocked;
			sender = ipc_thread_queue_first(senders);
	
			if ((sender != ITH_NULL) &&
			    (port->ip_msgcount < port->ip_qlimit)) {
				ipc_thread_rmqueue(senders, sender);
				sender->ith_state = MACH_MSG_SUCCESS;
				thread_go(sender);
			}
		}
	}

	ip_unlock(port);

	*kmsgp = kmsg;
	return mr;
}
