/****************************************************************
 *								*
 *	Copyright 2006 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_inet.h"
#include "gtm_stdio.h"

#if !defined(__MVS__) && !defined(VMS)
#include <sys/param.h>
#endif
#include <sys/time.h>
#include <errno.h>
#ifdef UNIX
#include <sys/sem.h>
#endif
#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "repl_dbg.h"
#include "repl_shutdcode.h"
#include "repl_sem.h"
#include "util.h"
#include "repl_log.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	gtmsource_options_t	gtmsource_options;
GBLREF	boolean_t		holds_sem[NUM_SEM_SETS][NUM_SRC_SEMS];

int gtmsource_statslog(void)
{
	assert(holds_sem[SOURCE][JNL_POOL_ACCESS_SEM]);
	repl_log(stderr, TRUE, TRUE, "Initiating STATSLOG operation on source server pid [%d] for secondary instance [%s]\n",
		jnlpool.gtmsource_local->gtmsource_pid, jnlpool.gtmsource_local->secondary_instname);
	if (gtmsource_options.statslog == jnlpool.gtmsource_local->statslog)
	{
		util_out_print("STATSLOG is already !AD. Not initiating change in stats log", TRUE, gtmsource_options.statslog ?
				strlen("ON") : strlen("OFF"), gtmsource_options.statslog ? "ON" : "OFF");
		return (ABNORMAL_SHUTDOWN);
	}
	if (!gtmsource_options.statslog)
	{
		jnlpool.gtmsource_local->statslog = FALSE;
		jnlpool.gtmsource_local->statslog_file[0] = '\0';
		util_out_print("STATSLOG turned OFF", TRUE);
		return (NORMAL_SHUTDOWN);
	}
	if ('\0' == gtmsource_options.log_file[0]) /* Stats log file not specified, use general log file */
	{
		util_out_print("No file specified for stats log. Using general log file !AD", TRUE,
				LEN_AND_STR(jnlpool.gtmsource_local->log_file));
		STRCPY(gtmsource_options.log_file, jnlpool.gtmsource_local->log_file);
	} else if (0 == STRCMP(jnlpool.gtmsource_local->log_file, gtmsource_options.log_file))
	{
		util_out_print("Stats log file is already !AD. Not initiating change in log file", TRUE,
				LEN_AND_STR(gtmsource_options.log_file));
		return (ABNORMAL_SHUTDOWN);
	}
	STRCPY(jnlpool.gtmsource_local->statslog_file, gtmsource_options.log_file);
	jnlpool.gtmsource_local->statslog = TRUE;
	util_out_print("Stats log turned on with file !AD", TRUE, strlen(jnlpool.gtmsource_local->statslog_file),
			jnlpool.gtmsource_local->statslog_file);
	return (NORMAL_SHUTDOWN);
}
