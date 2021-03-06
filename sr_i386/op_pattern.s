#################################################################
#								#
#	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#	PAGE	,132
	.title	op_pattern.s

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"
	.INCLUDE	"mval_def.si"

	.sbttl	op_pattern
#	PAGE	+
	.text
.extern	do_patfixed
.extern	do_pattern

# PUBLIC	op_pattern
ENTRY op_pattern
	pushl	%edx
	pushl	%eax
	movl	mval_a_straddr(%edx),%eax
	#
	# This is an array of unaligned ints. If the first word is zero, then call do_pattern
	# instead of do_patfixed. Only the low order byte is significant and so it is the only
	# one we need to test. We would do this in assembly because (1) we need the assmembly
	# routine anyway to save the return value into $TEST and (2) it saves an extra level of
	# call linkage at the C level to do the decision here.
	#
	cmpb	$0,(%eax) # little endian compare of low order byte
	je	l1
	call	do_patfixed
	jmp	l2

l1:	call	do_pattern
l2:	addl	$8,%esp
	cmpl	$0,%eax
	ret
# op_pattern ENDP

# END
