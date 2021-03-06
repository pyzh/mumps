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
#include "gdsroot.h"
#include "gdskill.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "collseq.h"
#include "gdsfhead.h"
#include "gdscc.h"
#include "copy.h"
#include "filestruct.h"
#include "jnl.h"
#include "buddy_list.h"		/* needed for tp.h */
#include "hashtab_mname.h"
#include "hashtab_int4.h"	/* needed for tp.h */
#include "tp.h"
#include "change_reg.h"
#include "targ_alloc.h"
#include "gvcst_protos.h"	/* for gvcst_root_search prototype */
#include "min_max.h"
#include "hashtab.h"

GBLREF gv_namehead	*gv_target;
GBLREF short            dollar_trestart;
GBLREF gv_key		*gv_currkey;
GBLREF gd_region	*gv_cur_region;
GBLREF gd_binding	*gd_map, *gd_map_top;

void gv_bind_name(gd_addr *addr, mstr *targ)
{
	gd_binding		*map;
	ht_ent_mname		*tabent;
	mname_entry		 gvent;
	int			res;
	boolean_t		added;
	enum db_acc_method	acc_meth;
	gd_region		*reg;
	gvnh_reg_t		*gvnh_reg;

	gd_map = addr->maps;
	gd_map_top = gd_map + addr->n_maps;
	gvent.var_name.addr = targ->addr;
	gvent.var_name.len = MIN(targ->len, MAX_MIDENT_LEN);
	COMPUTE_HASH_MNAME(&gvent);
	if ((NULL != (tabent = lookup_hashtab_mname((hash_table_mname *)addr->tab_ptr, &gvent)))
		&& (NULL != (gvnh_reg = (gvnh_reg_t *)tabent->value)))
	{
		reg = gvnh_reg->gd_reg;
		if (!reg->open)
		{
			gv_init_reg(reg);	/* could modify gvnh_reg->gvt if multiple regions map to same db file */
			assert(0 == gvnh_reg->gvt->clue.end);
		}
		gv_target = gvnh_reg->gvt;
		gv_cur_region = reg;
		acc_meth = gv_cur_region->dyn.addr->acc_meth;
		if (dollar_trestart)
			gv_target->clue.end = 0;
	} else
	{
		map = gd_map + 1;	/* get past local locks */
		for (; (res = memcmp(gvent.var_name.addr, &(map->name[0]), gvent.var_name.len)) >= 0; map++)
		{
			assert(map < gd_map_top);
			if (0 == res && 0 != map->name[gvent.var_name.len])
				break;
		}
		if (!map->reg.addr->open)
			gv_init_reg(map->reg.addr);
		gv_cur_region = map->reg.addr;
		acc_meth = gv_cur_region->dyn.addr->acc_meth;
		if ((dba_cm == acc_meth) || (dba_usr == acc_meth))
		{
			gv_target = malloc(sizeof(gv_namehead) + gvent.var_name.len);
			memset(gv_target, 0, sizeof(gv_namehead) + gvent.var_name.len);
			gv_target->gvname.var_name.addr = (char *)gv_target + sizeof(gv_namehead);
			gv_target->nct = 0;
			gv_target->collseq = NULL;
			gv_target->regcnt = 1;
			memcpy(gv_target->gvname.var_name.addr, gvent.var_name.addr, gvent.var_name.len);
			gv_target->gvname.var_name.len = gvent.var_name.len;
			gv_target->gvname.hash_code = gvent.hash_code;
		} else
		{
			assert(gv_cur_region->max_key_size <= MAX_KEY_SZ);
			gv_target = (gv_namehead *)targ_alloc(gv_cur_region->max_key_size, &gvent, gv_cur_region);
		}
		gvnh_reg = (gvnh_reg_t *)malloc(sizeof(gvnh_reg_t));
		gvnh_reg->gvt = gv_target;
		gvnh_reg->gd_reg = gv_cur_region;
		if (NULL != tabent)
		{	/* Since the global name was found but gv_target was null and now we created a new gv_target,
			 * the hash table key must point to the newly created gv_target->gvname. */
			tabent->key = gv_target->gvname;
			tabent->value = (char *)gvnh_reg;
		} else
		{
			added = add_hashtab_mname((hash_table_mname *)addr->tab_ptr, &gv_target->gvname, gvnh_reg, &tabent);
			assert(added);
		}
	}
	change_reg();
	memcpy(gv_currkey->base, gvent.var_name.addr, gvent.var_name.len);
	gv_currkey->base[gvent.var_name.len] = 0;
	gvent.var_name.len++;
	gv_currkey->base[gvent.var_name.len] = 0;
	gv_currkey->end = gvent.var_name.len;
	gv_currkey->prev = 0;
	if ((dba_bg == acc_meth) || (dba_mm == acc_meth))
	{
		if ((0 == gv_target->root) || (DIR_ROOT == gv_target->root))
			gvcst_root_search();
	}
	return;
}
