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

#ifndef INIT_SECSHR_ADDRS_INCLUDED
#define INIT_SECSHR_ADDRS_INCLUDED

void init_secshr_addrs(gd_addr_fn_ptr getnxtgdr, cw_set_element *cwsetaddrs,
		       sgm_info **firstsiaddrs, sgm_info **firstsibyftokaddrs,
		       unsigned char *cwsetdepthaddrs, uint4 epid,
		       uint4 icnt, int4 gtmospagesize, gd_region **jpool_reg_address,
		       inctn_opcode_t *inctn_opcode_address,
		       inctn_detail_t *inctn_detail_address, short *dollar_tlevel_address,
		       int4 *update_trans_address, sgmnt_addrs **cs_addrs_address,
		       boolean_t *kip_incremented_address, boolean_t *need_kip_incr_address,
		       trans_num *start_tn_address);

#include "dpgbldir.h"		/* for "get_next_gdr" used by INVOKE_INIT_SECSHR_ADDRS macro */

GBLREF	sgm_info		*first_sgm_info;	/* List of all regions (unsorted) in TP transaction */
GBLREF	sgm_info		*first_tp_si_by_ftok;	/* List of READ or UPDATED regions sorted on ftok order */
GBLREF	cw_set_element		cw_set[];
GBLREF	unsigned char		cw_set_depth;
GBLREF	uint4			process_id;
GBLREF	uint4			image_count;
GBLREF	jnlpool_addrs		jnlpool;
GBLREF	inctn_opcode_t		inctn_opcode;
GBLREF	inctn_detail_t		inctn_detail;			/* holds detail to fill in to inctn jnl record */
GBLREF	short			dollar_tlevel;
GBLREF	int4			update_trans;
GBLREF	sgmnt_addrs		*cs_addrs;
GBLREF	boolean_t 		kip_incremented;
GBLREF	boolean_t		need_kip_incr;
GBLREF	trans_num		start_tn;

#define	INVOKE_INIT_SECSHR_ADDRS								\
	init_secshr_addrs(get_next_gdr, cw_set, &first_sgm_info, &first_tp_si_by_ftok,		\
				&cw_set_depth, process_id, image_count, OS_PAGE_SIZE,		\
				&jnlpool.jnlpool_dummy_reg, &inctn_opcode, &inctn_detail,	\
				&dollar_tlevel, &update_trans, &cs_addrs,			\
				&kip_incremented, &need_kip_incr, &start_tn);

#endif /* INIT_SECSHR_ADDRS_INCLUDED */
