#################################################################
#								#
#	Copyright 2007 Fidelity Information Services, Inc	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#	PAGE	,132
	.title	op_mproflinefetch.s

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"
	.INCLUDE	"g_msf.si"

	.sbttl	op_mproflinefetch
#	PAGE	+
	.DATA
.extern	frame_pointer

	.text
.extern	fetch
.extern	stack_leak_check
.extern pcurrpos

MPR_LINEFETCH	=	0x4

# PUBLIC	op_mproflinefetch
ENTRY op_mproflinefetch
	movq	frame_pointer(REG_IP),REG64_ACCUM
	popq	msf_mpc_off(REG64_ACCUM)
	movq    REG_PV, msf_ctxt_off(REG64_ACCUM)
	movb    $0,REG8_ACCUM             # variable length argument
	call	fetch
	movl 	$MPR_LINEFETCH,REG32_ARG0
	call	pcurrpos
	call	stack_leak_check
	movq	frame_pointer(REG_IP),REG64_ACCUM
	pushq	msf_mpc_off(REG64_ACCUM)
	ret
# op_mproflinefetch ENDP

# END
