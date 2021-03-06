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

#include "mdef.h"

#include "gtm_string.h"

#include "iosp.h"
#include "io.h"
#include "gtm_logicals.h"
#include "trans_log_name.h"
#include "zco_init.h"

GBLREF	mstr	dollar_zcompile;

void zco_init(void)
{
	int4	status;
	mstr	val, tn;
	char	buf1[MAX_TRANS_NAME_LEN]; /* buffer to hold translated name */

	error_def(ERR_LOGTOOLONG);

	if (dollar_zcompile.addr)
		free(dollar_zcompile.addr);
	val.addr = ZCOMPILE;
	val.len = sizeof(ZCOMPILE) - 1;
	status = TRANS_LOG_NAME(&val, &tn, buf1, sizeof(buf1), dont_sendmsg_on_log2long);
	if ((SS_NORMAL != status) && (SS_NOLOGNAM != status))
	{
#		ifdef UNIX
		if (SS_LOG2LONG == status)
			rts_error(VARLSTCNT(5) ERR_LOGTOOLONG, 3, val.len, val.addr, sizeof(buf1) - 1);
		else
#		endif
			rts_error(VARLSTCNT(1) status);
	}
	if (status == SS_NOLOGNAM)
		dollar_zcompile.len = 0;
	else
	{
		dollar_zcompile.len = tn.len;
		dollar_zcompile.addr = (char *) malloc (tn.len);
		memcpy (dollar_zcompile.addr, buf1, tn.len);
	}
}
