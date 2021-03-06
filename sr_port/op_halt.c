/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_stdlib.h"		/* for exit() */

#ifdef VMS
#  include <ssdef.h>
#endif
#include "op.h"

void op_halt(void)
{
#ifdef VMS
	sys$exit(SS$_NORMAL);
#else
	exit(0);
#endif
}
