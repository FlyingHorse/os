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
 * Revision 2.5  91/05/14  16:35:47  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:23:15  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  15:50:24  mrt]
 * 
 * Revision 2.3  90/11/05  14:29:47  rpd
 * 	Use new ips_reference and ips_release.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.2  90/06/02  14:51:19  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:01:53  rpd]
 * 
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
 *	File:	ipc/ipc_pset.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions to manipulate IPC port sets.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/event.h>

#include <sys/mach/port.h>
#include <sys/mach/kern_return.h>
#include <sys/mach/message.h>
#include <sys/mach/ipc/ipc_mqueue.h>
#include <sys/mach/ipc/ipc_object.h>
#include <sys/mach/ipc/ipc_port.h>
#include <sys/mach/ipc/ipc_pset.h>
#include <sys/mach/ipc/ipc_right.h>
#include <sys/mach/ipc/ipc_space.h>
#include <sys/mach/ipc/ipc_print.h>

#if 0
#include <kern/thread_pool.h>
#endif
/*
 * Forward declarations
 */
void ipc_pset_add(
	ipc_pset_t	pset,
	ipc_port_t	port);

/*
 *	Routine:	ipc_pset_alloc
 *	Purpose:
 *		Allocate a port set.
 *	Conditions:
 *		Nothing locked.  If successful, the port set is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port set is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NO_SPACE		No room for an entry in the space.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_pset_alloc(
	ipc_space_t	space,
	mach_port_name_t	*namep,
	ipc_pset_t	*psetp)
{
	ipc_pset_t pset;
	mach_port_name_t name;
	kern_return_t kr;

	kr = ipc_object_alloc(space, IOT_PORT_SET,
			      MACH_PORT_TYPE_PORT_SET, 0,
			      &name, (ipc_object_t *) &pset);
	if (kr != KERN_SUCCESS)
		return kr;
	/* pset is locked */

	pset->ips_local_name = name;
	ipc_mqueue_init(&pset->ips_messages);
#if 0	
	thread_pool_init(&pset->ips_thread_pool);
#endif
	*namep = name;
	*psetp = pset;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_pset_alloc_name
 *	Purpose:
 *		Allocate a port set, with a specific name.
 *	Conditions:
 *		Nothing locked.  If successful, the port set is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port set is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NAME_EXISTS	The name already denotes a right.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_pset_alloc_name(
	ipc_space_t	space,
	mach_port_name_t	name,
	ipc_pset_t	*psetp)
{
	ipc_pset_t pset;
	kern_return_t kr;


	kr = ipc_object_alloc_name(space, IOT_PORT_SET,
				   MACH_PORT_TYPE_PORT_SET, 0,
				   name, (ipc_object_t *) &pset);
	if (kr != KERN_SUCCESS)
		return kr;
	/* pset is locked */

	pset->ips_local_name = name;
	ipc_mqueue_init(&pset->ips_messages);
#if 0	
	thread_pool_init(&pset->ips_thread_pool);
#endif
	*psetp = pset;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_pset_add
 *	Purpose:
 *		Puts a port into a port set.
 *		The port set gains a reference.
 *	Conditions:
 *		Both port and port set are locked and active.
 *		The port isn't already in a set.
 *		The owner of the port set is also receiver for the port.
 */

void
ipc_pset_add(
	ipc_pset_t	pset,
	ipc_port_t	port)
{
	assert(ips_active(pset));
	assert(ip_active(port));
	assert(port->ip_pset == IPS_NULL);

	port->ip_pset = pset;
	ips_reference(pset);

	imq_lock(&port->ip_messages);
	imq_lock(&pset->ips_messages);

	/* move messages from port's queue to the port set's queue */

	ipc_mqueue_move(&pset->ips_messages, &port->ip_messages, port);
	imq_unlock(&pset->ips_messages);
	assert(ipc_kmsg_queue_empty(&port->ip_messages.imq_messages));

	/* wake up threads waiting to receive from the port */

	ipc_mqueue_changed(&port->ip_messages, MACH_RCV_PORT_CHANGED);
	assert(ipc_thread_queue_empty(&port->ip_messages.imq_threads));
	imq_unlock(&port->ip_messages);
}

/*
 *	Routine:	ipc_pset_remove
 *	Purpose:
 *		Removes a port from a port set.
 *		The port set loses a reference.
 *	Conditions:
 *		Both port and port set are locked.
 *		The port must be active.
 */

void
ipc_pset_remove(
	ipc_pset_t	pset,
	ipc_port_t	port)
{
	assert(ip_active(port));
	assert(port->ip_pset == pset);

	port->ip_pset = IPS_NULL;
	ips_release(pset);

	imq_lock(&port->ip_messages);
	imq_lock(&pset->ips_messages);

	/* move messages from port set's queue to the port's queue */

	ipc_mqueue_move(&port->ip_messages, &pset->ips_messages, port);

	imq_unlock(&pset->ips_messages);
	imq_unlock(&port->ip_messages);
}

/*
 *	Routine:	ipc_pset_move
 *	Purpose:
 *		If nset is IPS_NULL, removes port
 *		from the port set it is in.  Otherwise, adds
 *		port to nset, removing it from any set
 *		it might already be in.
 *	Conditions:
 *		The space is read-locked.
 *	Returns:
 *		KERN_SUCCESS		Moved the port.
 *		KERN_NOT_IN_SET		nset is null and port isn't in a set.
 */

kern_return_t
ipc_pset_move(
	ipc_space_t	space,
	ipc_port_t	port,
	ipc_pset_t	nset)
{
	ipc_pset_t oset;

	/*
	 *	While we've got the space locked, it holds refs for
	 *	the port and nset (because of the entries).  Also,
	 *	they must be alive.  While we've got port locked, it
	 *	holds a ref for oset, which might not be alive.
	 */

	ip_lock(port);
	assert(ip_active(port));

	oset = port->ip_pset;

	if (oset == nset) {
		/* the port is already in the new set:  a noop */

		is_read_unlock(space);
	} else if (oset == IPS_NULL) {
		/* just add port to the new set */

		ips_lock(nset);
		assert(ips_active(nset));
		is_read_unlock(space);

		ipc_pset_add(nset, port);

		ips_unlock(nset);
	} else if (nset == IPS_NULL) {
		/* just remove port from the old set */

		is_read_unlock(space);
		ips_lock(oset);

		ipc_pset_remove(oset, port);

		if (ips_active(oset))
			ips_unlock(oset);
		else {
			ips_check_unlock(oset);
			oset = IPS_NULL; /* trigger KERN_NOT_IN_SET */
		}
	} else {
		/* atomically move port from oset to nset */

		if (oset < nset) {
			ips_lock(oset);
			ips_lock(nset);
		} else {
			ips_lock(nset);
			ips_lock(oset);
		}

		is_read_unlock(space);
		assert(ips_active(nset));

		ipc_pset_remove(oset, port);
		ipc_pset_add(nset, port);

		ips_unlock(nset);
		ips_check_unlock(oset);	/* KERN_NOT_IN_SET not a possibility */
	}

	ip_unlock(port);

	return (((nset == IPS_NULL) && (oset == IPS_NULL)) ?
		KERN_NOT_IN_SET : KERN_SUCCESS);
}

/*
 *	Routine:	ipc_pset_destroy
 *	Purpose:
 *		Destroys a port_set.
 *
 *		Doesn't remove members from the port set;
 *		that happens lazily.  As members are removed,
 *		their messages are removed from the queue.
 *	Conditions:
 *		The port_set is locked and alive.
 *		The caller has a reference, which is consumed.
 *		Afterwards, the port_set is unlocked and dead.
 */

void
ipc_pset_destroy(
	ipc_pset_t	pset)
{
	assert(ips_active(pset));

	pset->ips_object.io_bits &= ~IO_BITS_ACTIVE;

	imq_lock(&pset->ips_messages);
	ipc_mqueue_changed(&pset->ips_messages, MACH_RCV_PORT_DIED);
	imq_unlock(&pset->ips_messages);

	/* XXXX Perhaps ought to verify ips_thread_pool is empty */

	ips_release(pset);	/* consume the ref our caller gave us */
	ips_check_unlock(pset);
}



static int      filt_machportattach(struct knote *kn);
static void     filt_machportdetach(struct knote *kn);
static int      filt_machport(struct knote *kn, long hint);
static void     filt_machporttouch(struct knote *kn, struct kevent64_s *kev, u_long type);
struct filterops machport_filtops = {
	.f_isfd = 0,
	.f_attach = filt_machportattach,
	.f_detach = filt_machportdetach,
	.f_event = filt_machport,
	.f_touch = filt_machporttouch,
};

static int
filt_machportattach(struct knote *kn)
{
	mach_port_name_t	name = (mach_port_name_t)kn->kn_kevent.ident;
	ipc_pset_t			pset = IPS_NULL;
	kern_return_t		kr;

	kr = ipc_object_translate(current_space(), name, MACH_PORT_RIGHT_PORT_SET,
							  (ipc_object_t *)&pset);

	if (kr != KERN_SUCCESS)
		return (kr == KERN_INVALID_NAME ? ENOENT : ENOTSUP);

	kn->kn_ptr.p_pset = pset;
	ips_reference(pset);
	knlist_add(&pset->ips_messages.imq_note, kn, 1);
	ips_unlock(pset);
	return (0);
}

static void
filt_machportdetach(struct knote *kn)
{
	ipc_pset_t	pset = kn->kn_ptr.p_pset;

	ips_lock(pset);
	knlist_remove(&pset->ips_messages.imq_note, kn, 1);
	kn->kn_ptr.p_pset = IPS_NULL;
	ips_unlock(pset);
}

static int
filt_machport(struct knote *kn, long hint __unused)
{
	mach_port_name_t        name = (mach_port_name_t)kn->kn_kevent.ident;
	ipc_pset_t              pset = IPS_NULL;	
	kern_return_t           kr;
#if 0	
	mach_msg_option_t	option;
	mach_msg_size_t		size;
#endif

	kr = ipc_object_translate(current_space(), name, MACH_PORT_RIGHT_PORT_SET,
							  (ipc_object_t *)&pset);
	if (kr != KERN_SUCCESS || pset != kn->kn_ptr.p_pset || !ips_active(pset)) {
		kn->kn_data = 0;
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		if (pset != IPS_NULL)
			ips_unlock(pset);
		return (1);
	}
	ips_reference(pset);
	ips_unlock(pset);
	
	/*  XXX check queue for messages */
	return (0);
}

static void
filt_machporttouch(struct knote *kn, struct kevent64_s *kev, u_long type)
{
	switch (type) {
	case EVENT_REGISTER:
		kn->kn_sfflags = kev->fflags;
		kn->kn_sdata = kev->data;
		kn->kn_kevent.ext[0] = kev->ext[0];
		kn->kn_kevent.ext[1] = kev->ext[1];
		break;
	case EVENT_PROCESS:
		*kev = kn->kn_kevent;
		if (kn->kn_flags & EV_CLEAR) {
			kn->kn_data = 0;
			kn->kn_fflags = 0;
		}
		break;
	default:
		panic("filt_machporttouch() - invalid type (%ld)", type);
		break;
	}
}

#if	MACH_KDB
#include <mach_kdb.h>

#include <ddb/db_output.h>

#define	printf	kdbprintf

int
ipc_list_count(
	struct ipc_kmsg *base)
{
	register int count = 0;

	if (base) {
		struct ipc_kmsg *kmsg = base;

		++count;
		while (kmsg && kmsg->ikm_next != base
			    && kmsg->ikm_next != IKM_BOGUS){
			kmsg = kmsg->ikm_next;
			++count;
		}
	}
	return(count);
}

/*
 *	Routine:	ipc_pset_print
 *	Purpose:
 *		Pretty-print a port set for kdb.
 */

void
ipc_pset_print(
	ipc_pset_t	pset)
{
	extern int indent;

	printf("pset 0x%x\n", pset);

	indent += 2;

	ipc_object_print(&pset->ips_object);
	iprintf("local_name = 0x%x\n", pset->ips_local_name);
	iprintf("%d kmsgs => 0x%x",
		ipc_list_count(pset->ips_messages.imq_messages.ikmq_base),
		pset->ips_messages.imq_messages.ikmq_base);
	printf(",rcvrs = 0x%x\n", pset->ips_messages.imq_threads.ithq_base);

	indent -=2;
}

#endif	/* MACH_KDB */
