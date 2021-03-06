/****************************************************************
 *								*
 *	Copyright 2001, 2008 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* Standard signal processor
 *
 * If we are nesting our handlers in an improper way, this routine will
 * not return but will immediately invoke core/termination processing.
 *
 * Returns if some condition makes it inadvisable to exit now else invokes the system exit() system call.
 * For GTMSECSHR it unconditionally returns to gtmsecshr_signal_handler() which later invokes gtmsecshr_exit().
 */

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_unistd.h"
#include "gtm_stdlib.h"		/* for exit() */
#include "gtm_inet.h"

#include <signal.h>

#include "gtm_stdio.h"
#include "error.h"
#include "gtmsiginfo.h"
#include "gtmimagename.h"
#include "gt_timer.h"
#include "send_msg.h"
#include "generic_signal_handler.h"
#include "gtmmsg.h"
#include "have_crit.h"

#define	DEFER_EXIT_PROCESSING	((EXIT_PENDING_TOLERANT >= exit_state) &&			\
	(exit_handler_active || !OK_TO_INTERRUPT))
/* These fields are defined as globals not because they are used globally but
 * so they will be easily retrievable even in 'pro' cores.
 */
GBLDEF siginfo_t	exi_siginfo;

GBLDEF gtm_sigcontext_t exi_context;

GBLREF	VSIG_ATOMIC_T		forced_exit;
GBLREF	int4			forced_exit_err;
GBLREF	int4			exi_condition;
GBLREF	enum gtmImageTypes	image_type;
GBLREF	boolean_t		dont_want_core;
GBLREF	boolean_t		created_core;
GBLREF	boolean_t		need_core;
GBLREF	uint4			process_id;
GBLREF	volatile int4		exit_state;
GBLREF	volatile boolean_t	core_in_progress;
GBLREF	gtmsiginfo_t		signal_info;
GBLREF	gtmImageName		gtmImageNames[];
GBLREF	boolean_t		exit_handler_active;
GBLREF	void			(*call_on_signal)();
GBLREF	boolean_t		gtm_quiet_halt;
GBLREF	int			process_exiting;
GBLREF	volatile int4           gtmMallocDepth;         /* Recursion indicator */

void generic_signal_handler(int sig, siginfo_t *info, void *context)
{
	boolean_t	exit_now;
	gtm_sigcontext_t	*context_ptr;
	void		(*signal_routine)();

	error_def(ERR_KRNLKILL);
	error_def(ERR_FORCEDHALT);
	error_def(ERR_KILLBYSIG);
	error_def(ERR_KILLBYSIGUINFO);
	error_def(ERR_KILLBYSIGSINFO1);
	error_def(ERR_KILLBYSIGSINFO2);
	error_def(ERR_KILLBYSIGSINFO3);
	error_def(ERR_GTMSECSHRSHUTDN);

	/* Save parameter value in global variables for easy access in core */
	dont_want_core = FALSE;		/* (re)set in case we recurse */
	created_core = FALSE;		/* we can deal with a second core if needbe */
	exi_condition = sig;
	if (NULL != info)
		exi_siginfo = *info;
	else
		memset(&exi_siginfo, 0, sizeof(*info));
#if defined(__ia64) && defined(__hpux)
	context_ptr = (gtm_sigcontext_t *)context;	/* no way to make a copy of the context */
	memset(&exi_context, 0, sizeof(exi_context));
#else
	if (NULL != context)
		exi_context = *(gtm_sigcontext_t *)context;
	else
		memset(&exi_context, 0, sizeof(exi_context));
	context_ptr = &exi_context;
#endif
	/* Check if we are fielding nested immediate shutdown signals */
	if (EXIT_IMMED <= exit_state)
	{
		switch(sig)
		{	/* If we are dealing with one of these three dangerous signals which we have
			 * already hit while attempting to shutdown once, die with core now.
			 */
			case SIGSEGV:
			case SIGBUS:
			case SIGILL:
				if (core_in_progress)
				{
					if (exit_handler_active)
						_exit(sig);
					else
						exit(sig);
				}
				core_in_progress = TRUE;
				DUMP_CORE;
				GTMASSERT;
			default:
				;
		}
	}
	switch(sig)
	{
		case SIGTERM:
			if (GTMSECSHR_IMAGE == image_type)
				forced_exit_err = ERR_GTMSECSHRSHUTDN;
			else
				forced_exit_err = ERR_FORCEDHALT;
			/* If nothing pending AND we have crit or in wcs_wtstart() or already in exit processing, wait to
			 * invoke shutdown. wcs_wtstart() manipulates the active queue that a concurrent process in crit
			 * in bt_put() might be waiting for. interrupting it can cause deadlocks (see C9C11-002178).
			 */
			if (DEFER_EXIT_PROCESSING)
			{
				forced_exit = TRUE;
				exit_state++;		/* Make exit pending, may still be tolerant though */
				assert(GTMSECSHR_IMAGE != image_type);
				return;
			}
			exit_state = EXIT_IMMED;
			if (ERR_FORCEDHALT != forced_exit_err || !gtm_quiet_halt)
			{
				send_msg(VARLSTCNT(1) forced_exit_err);
				gtm_putmsg(VARLSTCNT(1) forced_exit_err);
			}
			dont_want_core = TRUE;
			break;
		case SIGQUIT:	/* Handle SIGQUIT specially which we ALWAYS want to defer if possible as it is always sent */
			dont_want_core = TRUE;
			extract_signal_info(sig, &exi_siginfo, context_ptr, &signal_info);
			switch(signal_info.infotype)
			{
				case GTMSIGINFO_NONE:
					forced_exit_err = ERR_KILLBYSIG;
					break;
				case GTMSIGINFO_USER:
					forced_exit_err = ERR_KILLBYSIGUINFO;
					break;
				case GTMSIGINFO_ILOC + GTMSIGINFO_BADR:
					forced_exit_err = ERR_KILLBYSIGSINFO1;
					break;
				case GTMSIGINFO_ILOC:
					forced_exit_err = ERR_KILLBYSIGSINFO2;
					break;
				case GTMSIGINFO_BADR:
					forced_exit_err = ERR_KILLBYSIGSINFO3;
					break;
				default:
					exit_state = EXIT_IMMED;
					GTMASSERT;
			}
			/* If nothing pending AND we have crit or already in exit processing, wait to invoke shutdown */
			if (DEFER_EXIT_PROCESSING)
			{
				forced_exit = TRUE;
				exit_state++;		/* Make exit pending, may still be tolerant though */
				assert(GTMSECSHR_IMAGE != image_type);
				return;
			}
			exit_state = EXIT_IMMED;
			switch(signal_info.infotype)
			{
				case GTMSIGINFO_NONE:
					send_msg(VARLSTCNT(6) ERR_KILLBYSIG, 4, GTMIMAGENAMETXT(image_type), process_id, sig);
					gtm_putmsg(VARLSTCNT(6) ERR_KILLBYSIG, 4, GTMIMAGENAMETXT(image_type), process_id, sig);
					break;
				case GTMSIGINFO_USER:
					send_msg(VARLSTCNT(8) ERR_KILLBYSIGUINFO, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.send_pid, signal_info.send_uid);
					gtm_putmsg(VARLSTCNT(8) ERR_KILLBYSIGUINFO, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.send_pid, signal_info.send_uid);
					break;
				case GTMSIGINFO_ILOC + GTMSIGINFO_BADR:
					send_msg(VARLSTCNT(8) ERR_KILLBYSIGSINFO1, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr, signal_info.bad_vadr);
					gtm_putmsg(VARLSTCNT(8) ERR_KILLBYSIGSINFO1, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr, signal_info.bad_vadr);
					break;
				case GTMSIGINFO_ILOC:
					send_msg(VARLSTCNT(7) ERR_KILLBYSIGSINFO2, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr);
					gtm_putmsg(VARLSTCNT(7) ERR_KILLBYSIGSINFO2, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr);
					break;
				case GTMSIGINFO_BADR:
					send_msg(VARLSTCNT(7) ERR_KILLBYSIGSINFO3, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.bad_vadr);
					gtm_putmsg(VARLSTCNT(7) ERR_KILLBYSIGSINFO3, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.bad_vadr);
					break;
			}
			break;
#ifdef _AIX
		case SIGDANGER:
			forced_exit_err = ERR_KRNLKILL;
			/* If nothing pending AND we have crit or already in exit processing, wait to invoke shutdown */
			if (DEFER_EXIT_PROCESSING)
			{
				forced_exit = TRUE;
				exit_state++;		/* Make exit pending, may still be tolerant though */
				assert(GTMSECSHR_IMAGE != image_type);
				return;
			}
			exit_state = EXIT_IMMED;
			send_msg(VARLSTCNT(1) forced_exit_err);
			gtm_putmsg(VARLSTCNT(1) forced_exit_err);
			dont_want_core = TRUE;
			break;
#endif
		default:
			extract_signal_info(sig, &exi_siginfo, context_ptr, &signal_info);
			switch(signal_info.infotype)
			{
				case GTMSIGINFO_NONE:
					exit_state = EXIT_IMMED;
					send_msg(VARLSTCNT(6) ERR_KILLBYSIG, 4, GTMIMAGENAMETXT(image_type), process_id, sig);
					gtm_putmsg(VARLSTCNT(6) ERR_KILLBYSIG, 4, GTMIMAGENAMETXT(image_type), process_id, sig);
					break;
				case GTMSIGINFO_USER:
					/* This signal was SENT to us so it can wait until we are out of crit to cause an exit */
					forced_exit_err = ERR_KILLBYSIGUINFO;
					/* If nothing pending AND we have crit or already exiting, wait to invoke shutdown */
					if (DEFER_EXIT_PROCESSING)
					{
						assert(GTMSECSHR_IMAGE != image_type);
						forced_exit = TRUE;
						exit_state++;		/* Make exit pending, may still be tolerant though */
						need_core = TRUE;
						gtm_fork_n_core();	/* Generate "virgin" core while we can */
						return;
					}
					exit_state = EXIT_IMMED;
					send_msg(VARLSTCNT(8) ERR_KILLBYSIGUINFO, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.send_pid, signal_info.send_uid);
					gtm_putmsg(VARLSTCNT(8) ERR_KILLBYSIGUINFO, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.send_pid, signal_info.send_uid);
					break;
				case GTMSIGINFO_ILOC + GTMSIGINFO_BADR:
					exit_state = EXIT_IMMED;
					send_msg(VARLSTCNT(8) ERR_KILLBYSIGSINFO1, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr, signal_info.bad_vadr);
					gtm_putmsg(VARLSTCNT(8) ERR_KILLBYSIGSINFO1, 6, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr, signal_info.bad_vadr);
					break;
				case GTMSIGINFO_ILOC:
					exit_state = EXIT_IMMED;
					send_msg(VARLSTCNT(7) ERR_KILLBYSIGSINFO2, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr);
					gtm_putmsg(VARLSTCNT(7) ERR_KILLBYSIGSINFO2, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.int_iadr);
					break;
				case GTMSIGINFO_BADR:
					exit_state = EXIT_IMMED;
					send_msg(VARLSTCNT(7) ERR_KILLBYSIGSINFO3, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.bad_vadr);
					gtm_putmsg(VARLSTCNT(7) ERR_KILLBYSIGSINFO3, 5, GTMIMAGENAMETXT(image_type),
							process_id, sig, signal_info.bad_vadr);
					break;
				default:
					exit_state = EXIT_IMMED;
					GTMASSERT;
			}
			if (0 != signal_info.sig_err)
			{
				send_msg(VARLSTCNT(1) signal_info.sig_err);
				gtm_putmsg(VARLSTCNT(1) signal_info.sig_err);
			}
			break;
	} /* switch (sig) */
	process_exiting = TRUE;	/* set this BEFORE cancelling timers as wcs_phase2_commit_wait relies on this */
	cancel_timer(0);	/* Don't want any interruptions */
	fflush(stdout);
	if (!dont_want_core)
	{
		need_core = TRUE;
		gtm_fork_n_core();
	}
	/* As on VMS, a mupip stop does not drive the condition handlers unless we are in crit */
	if ((0 != have_crit(CRIT_HAVE_ANY_REG | CRIT_IN_COMMIT) || SIGTERM != exi_condition) && CHANDLER_EXISTS)
		DRIVECH(exi_condition);
	/* If a special routine was registered to be driven on a signal, drive it now */
	if (0 != exi_condition && call_on_signal)
	{
		signal_routine = call_on_signal;
		call_on_signal = NULL;		/* So we don't recursively call ourselves */
		(*signal_routine)();
	}
	if (GTMSECSHR_IMAGE != image_type)
	{
		assert((EXIT_IMMED <= exit_state) || !exit_handler_active);
		exit(-exi_condition);
	} else
		return;
}
