/****************************************************************
 *								*
 *	Copyright 2001, 2005 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include <sys/mman.h>
#include "gtm_unistd.h"
#include "gtm_socket.h"
#include <sys/time.h>
#include <errno.h>
#include "gtm_fcntl.h"
#include "gtm_inet.h"
#include "gtm_stdio.h"
#include "gtm_string.h"
#include "gtm_time.h"

#ifdef UNIX
#include "gtm_ipc.h"
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/un.h>
#ifndef __MVS__
#include <sys/param.h>
#endif
#elif defined(VMS)
#include <psldef.h>
#include <descrip.h> /* Required for gtmrecv.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gtmrecv.h"
#include "repl_dbg.h"
#include "jnl.h"
#include "buddy_list.h"
#include "hashtab_mname.h"	/* needed for muprec.h */
#include "hashtab_int4.h"	/* needed for muprec.h */
#include "hashtab_int8.h"	/* needed for muprec.h */
#include "muprec.h"
#include "iosp.h"
#include "repl_shutdcode.h"
#include "eintr_wrappers.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "repl_sem.h"
#ifdef VMS
#include "repl_shm.h"
#endif
#include "repl_log.h"
#ifdef UNIX
#include "mutex.h"
#endif
#include "gtm_event_log.h"
#include "mupip_exit.h"
#include "read_db_files_from_gld.h"
#include "updproc.h"

GBLREF	gd_region		*gv_cur_region;
GBLREF	recvpool_addrs		recvpool;
GBLREF  gld_dbname_list		*upd_db_files;
GBLREF	boolean_t	        pool_init;
GBLREF	jnlpool_addrs	        jnlpool;
GBLREF	jnlpool_ctl_ptr_t	jnlpool_ctl;
GBLREF	void                    (*call_on_signal)();

void  updproc_stop(boolean_t exit)
{
	int4		status;
	int		fclose_res;

	call_on_signal = NULL;	/* Don't reenter on error */
	if (pool_init)
	{
		rel_lock(jnlpool.jnlpool_dummy_reg);
		/* nullify jnlpool_ctl before detaching from jnlpool since if it is the other way, we might be interrupted
		 * by the periodic timer routines and end up in jnl_write_epoch_rec() routine that dereferences jnlpool_ctl
		 * since it is non-NULL although it has been detached from and is no longer valid memory.
		 */
		jnlpool_ctl = NULL;
#ifdef UNIX
		mutex_cleanup(jnlpool.jnlpool_dummy_reg);
		SHMDT(jnlpool.jnlpool_ctl);
#elif defined(VMS)
		if (SS$_NORMAL != (status = detach_shm(jnlpool.shm_range)))
			repl_log(stderr, TRUE, TRUE, "Error detaching from jnlpool : %s\n", REPL_STR_ERROR);
		if (SS$_NORMAL != (status = signoff_from_gsec(jnlpool.shm_lockid)))
			repl_log(stderr, TRUE, TRUE, "Error dequeueing lock on jnlpool global section : %s\n", REPL_STR_ERROR);
#else
#error Unsupported Platform
#endif
		jnlpool.jnlpool_ctl = NULL;
		pool_init = FALSE;
	}
	recvpool.upd_proc_local->upd_proc_shutdown = NORMAL_SHUTDOWN;
	recvpool.upd_proc_local->upd_proc_pid = 0;
#ifdef UNIX
	SHMDT(recvpool.recvpool_ctl);
#elif defined(VMS)
	if(SS$_NORMAL != (status = detach_shm(recvpool.shm_range)))
		repl_log(stderr, TRUE, TRUE, "Update process could not detach from recvpool : %s\n", REPL_STR_ERROR);
	if (SS$_NORMAL != (status = signoff_from_gsec(recvpool.shm_lockid)))
		repl_log(stderr, TRUE, TRUE, "Error dequeueing lock on recvpool global section : %s\n", REPL_STR_ERROR);
#else
#error Unsupported Platform
#endif
	recvpool.recvpool_ctl = NULL;
	gtm_event_log_close();
	if (exit)
		mupip_exit(SS_NORMAL);
	return;
}

void updproc_sigstop(void)
{
	updproc_stop(FALSE);
	return;
}

void updproc_end(void)
{
	updproc_stop(TRUE);
	return;
}
