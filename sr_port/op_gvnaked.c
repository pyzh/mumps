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

#include <stdarg.h>
#include "gtm_string.h"

#include "gdsroot.h"
#include "gdskill.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdscc.h"
#include "copy.h"
#include "jnl.h"
#include "hashtab_int4.h"	/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "rtnhdr.h"
#include "mv_stent.h"		/* for COPY_SUBS_TO_GVCURRKEY macro */
#include "subscript.h"
#include "op.h"
#include "gvcst_protos.h"	/* for gvcst_root_search prototype */
#include "format_targ_key.h"
#include "gvsub2str.h"
#include "sgnl.h"
#include "mvalconv.h"
#include "tp_set_sgm.h"

GBLREF short            dollar_tlevel;
GBLREF gd_addr		*gd_header;
GBLREF bool		gv_curr_subsc_null;
GBLREF bool		gv_prev_subsc_null;
GBLREF gv_key		*gv_currkey;
GBLREF gv_namehead	*gv_target;
GBLREF sgmnt_addrs      *cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF sgm_info         *first_sgm_info;
GBLREF bool		transform;
GBLREF mstr		extnam_str;

void op_gvnaked(UNIX_ONLY_COMMA(int count_arg) mval *val_arg, ...)
{
	va_list		var;
	int		len;
	int		count;
	bool		was_null, is_null, sbs_cnt;
	mval		*val;
	short 		max_key;
	unsigned char	*ptr, *end_ptr;
	unsigned char	buff[MAX_ZWR_KEY_SZ], *end;

	error_def(ERR_GVNAKED);
	error_def(ERR_GVSUBOFLOW);
	error_def(ERR_GVIS);
	error_def(ERR_MAXNRSUBSCRIPTS);

	extnam_str.len = 0;
	if (!gv_currkey || (0 == gv_currkey->prev))
		rts_error(VARLSTCNT(1) ERR_GVNAKED);

	if ((dba_bg == gv_cur_region->dyn.addr->acc_meth) || (dba_mm == gv_cur_region->dyn.addr->acc_meth))
	{
		assert(INVALID_GV_TARGET != gv_target);
                if (dollar_tlevel && !first_sgm_info)
			tp_set_sgm();
		if (!gv_target->root || (DIR_ROOT == gv_target->root))
			gvcst_root_search();
		assert(gv_target->gd_csa == cs_addrs);
	}

	VMS_ONLY(va_count(count);)
	UNIX_ONLY(count = count_arg;)	/* i386 assembler modules may depend on unchanged count */
	if (0 >= count)
		GTMASSERT;
	gv_currkey->end = gv_currkey->prev;
	is_null = FALSE;
	was_null = (gv_currkey->end ? gv_prev_subsc_null : FALSE);
	max_key = gv_cur_region->max_key_size;
	sbs_cnt = 0;
	if (1 < count)
	{
		/* Use of naked reference can cause increase in number of subscripts.   So count the subscripts */
		ptr = &gv_currkey->base[0];
		end_ptr = ptr + gv_currkey->end;
		while (ptr < end_ptr)
			if (KEY_DELIMITER == *ptr++)
				sbs_cnt++;
		if (MAX_GVSUBSCRIPTS < count + sbs_cnt)
			rts_error(VARLSTCNT(1) ERR_MAXNRSUBSCRIPTS);
	}
	/* else naked reference will not increase number of subscripts, so do not worry about exceeding the limit */
	VAR_START(var, val_arg);
	val = val_arg;
	for ( ; ; )
	{
		COPY_SUBS_TO_GVCURRKEY(val, gv_currkey, was_null, is_null);	/* updates gv_currkey, was_null, is_null */
		if (0 < --count)
			val = va_arg(var, mval *);
		else
			break;
	}
	va_end(var);
	gv_prev_subsc_null = was_null; /* if true, it indicates there is a null subscript (except last subscript) in current key */
	gv_curr_subsc_null = is_null; /* if true, it indicates that last subscript in current key is null */
	if (was_null && (NEVER == gv_cur_region->null_subs))
		sgnl_gvnulsubsc();
	return;
}
