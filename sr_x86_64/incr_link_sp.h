/****************************************************************
 *								*
 *	Copyright 2007 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef INCR_LINK_SP_INCLUDED
#define INCR_LINK_SP_INCLUDED

#include "rtnhdr.h"
#include <elf.h>

#define NATIVE_HDR_LEN	(sizeof(Elf64_Ehdr))

#define COFFHDRLEN         (sizeof(int4) * 8)      /* Size of old masscomp-coff header before routine hdr */

#define GET_RTNHDR_ADDR(sym)	(unsigned char *)(sym)

#endif
