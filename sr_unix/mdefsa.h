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

#ifndef MDEFSA_included
#define MDEFSA_included

/* Declarations common to all unix mdefsp.h, to be moved here */

/* DSK_WRITE Macro needs <errno.h> to be included. Use this flavor if
   writing from the cache.
*/
#define	DSK_WRITE(reg, blk, cr, status)				\
{								\
	if (-1 == dsk_write(reg, blk, cr))			\
		status = errno;					\
	else							\
		status = 0;					\
}
/* Use this flavor if writing direct from storage (not cache buffer) */
#define	DSK_WRITE_NOCACHE(reg, blk, ptr, odv, status)		\
{								\
	if (-1 == dsk_write_nocache(reg, blk, ptr, odv))	\
		status = errno;					\
	else							\
		status = 0;					\
}

#define DOTM			".m"
#define DOTOBJ			".o"
#define GTM_DIST		"gtm_dist"
#define GTM_IMAGE_NAME		"mumps"
#define GTM_IMAGE_NAMELEN	(sizeof(GTM_IMAGE_NAME) - 1)
#define	DIR_SEPARATOR		'/'
#define	GTMSECSHR_NAME		"gtmsecshr"
#define GTMSECSHR_NAMELEN	(sizeof(GTMSECSHR_NAME) - 1)

#define	ICU_LIBFLAGS		(RTLD_NOW | RTLD_GLOBAL)

#ifdef __hpux
#  ifdef __ia64
#	define GTMSHR_IMAGE_NAME	"libgtmshr.so"
#  else
#	define GTMSHR_IMAGE_NAME	"libgtmshr.sl"
#  endif
#	define	ICU_LIBNAME		"libicuio.sl.36"
#elif defined(__MVS__)
#	define GTMSHR_IMAGE_NAME	"libgtmshr.dll"
#	define	ICU_LIBNAME		"libicuio.dll"
#else
#	define GTMSHR_IMAGE_NAME	"libgtmshr.so"
#	ifdef _AIX
	/* Conventionally, AIX archives shared objects into a static library.
	 * So we need to link with a member of the library instead of the
	 * library itself */
#		define	ICU_LIBNAME	"libicuio36.a(libicuio36."
#	else
#		define	ICU_LIBNAME	"libicuio.so.36"
#	endif
#endif

#define GTM_MAIN_FUNC		"gtm_main"

/* Prefix GT.M callback functions with "gtm_" */
#define GTM_PREFIX(func)	gtm_##func
#define cancel_timer		GTM_PREFIX(cancel_timer)
#define hiber_start		GTM_PREFIX(hiber_start)
#define hiber_start_wait_any	GTM_PREFIX(hiber_start_wait_any)
#define start_timer		GTM_PREFIX(start_timer)
#define jnlpool_detach		GTM_PREFIX(jnlpool_detach)

#endif /* MDEFSA_included */
