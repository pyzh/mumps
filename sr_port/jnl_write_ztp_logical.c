/****************************************************************
 *								*
 *	Copyright 2003, 2007 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_inet.h"
#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gtm_time.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsblk.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "jnl_write.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "iosp.h"
#ifdef DEBUG
#include "jnl_typedef.h"
LITREF	int	jrt_update[JRT_RECTYPES];	/* For IS_SET_KILL_ZKILL macro */
#endif

GBLREF	jnl_fence_control	jnl_fence_ctl;
GBLREF	jnlpool_ctl_ptr_t	temp_jnlpool_ctl;
GBLREF 	jnl_gbls_t		jgbl;
GBLREF	seq_num			seq_num_zero;

void	jnl_write_ztp_logical(sgmnt_addrs *csa, jnl_format_buffer *jfb)
{
	struct_jrec_ztp_upd	*jrec;
	jnl_private_control	*jpc;

	/* If REPL_WAS_ENABLED(csa) is TRUE, then we would not have gone through the code that initializes
	 * jgbl.gbl_jrec_time or jpc->pini_addr. But in this case, we are not writing the journal record
	 * to the journal buffer or journal file but write it only to the journal pool from where it gets
	 * sent across to the update process that does not care about these fields so it is ok to leave them as is.
	 */
	jpc = csa->jnl;
	assert((0 != jpc->pini_addr) || REPL_WAS_ENABLED(csa));
	assert(jgbl.gbl_jrec_time || REPL_WAS_ENABLED(csa));
	assert(csa->now_crit);
	assert(IS_SET_KILL_ZKILL(jfb->rectype));
	assert(IS_ZTP(jfb->rectype));
	jrec = (struct_jrec_ztp_upd *)jfb->buff;
	jrec->prefix.pini_addr = (0 == jpc->pini_addr) ? JNL_HDR_LEN : jpc->pini_addr;
	jrec->prefix.tn = csa->ti->curr_tn;
	jrec->prefix.time = jgbl.gbl_jrec_time;
	jrec->prefix.checksum = jfb->checksum;
	if (jgbl.forw_phase_recovery)
	{
		QWASSIGN(jrec->jnl_seqno, jgbl.mur_jrec_seqno);
		QWASSIGN(jrec->token, jgbl.mur_jrec_token_seq.token);
	} else
	{
		if (REPL_ALLOWED(csa))
			QWASSIGN(jrec->jnl_seqno, temp_jnlpool_ctl->jnl_seqno);
		else
			QWASSIGN(jrec->jnl_seqno, seq_num_zero);
		if (QWEQ(jnl_fence_ctl.token, seq_num_zero))
		{	/* generate token once after op_ztstart and use for all its mini-transactions
			 * jnl_fence_ctl.token is set to seq_num_zero in op_ztstart */
			if (REPL_ALLOWED(csa))
				QWASSIGN(jnl_fence_ctl.token, temp_jnlpool_ctl->jnl_seqno);
			else
			{
				TOKEN_SET(&jnl_fence_ctl.token, csa->ti->curr_tn, csa->regnum);
			}
		}
		QWASSIGN(jrec->token, jnl_fence_ctl.token);
	}
	JNL_WRITE_APPROPRIATE(csa, jpc, jfb->rectype, (jnl_record *)jrec, NULL, jfb);
}
