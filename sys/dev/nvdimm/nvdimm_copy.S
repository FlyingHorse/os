/*-
 * Copyright (c) 2016 Alexander Motin <mav@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include "opt_ddb.h"

#include <machine/asmacros.h>

#include "assym.s"

	.text

/*
 * ntb_copy1(%rdi=to, %rsi=from, %rdx=len)
 */
ENTRY(ntb_copy1)
	PUSH_FRAME_POINTER
	addq	%rdx,%rsi
	addq	%rdx,%rdi
	negq	%rdx
1:
	movq	(%rsi,%rdx),%rax
	movnti	%rax,(%rdi,%rdx)
	movq	8(%rsi,%rdx),%rax
	movnti	%rax,8(%rdi,%rdx)
	movq	16(%rsi,%rdx),%rax
	movnti	%rax,16(%rdi,%rdx)
	movq	24(%rsi,%rdx),%rax
	movnti	%rax,24(%rdi,%rdx)
	addq	$32,%rdx
	jne	1b
	sfence
	POP_FRAME_POINTER
	ret
END(ntb_copy1)

/*
 * ntb_copy2(%rdi=to1, %rsi=to2, %rdx=from, %rcx=len)
 */
ENTRY(ntb_copy2)
	PUSH_FRAME_POINTER
	addq	%rcx,%rsi
	addq	%rcx,%rdi
	addq	%rcx,%rdx
	negq	%rcx
1:
	movq	(%rdx,%rcx),%rax
	movnti	%rax,(%rdi,%rcx)
	movnti	%rax,(%rsi,%rcx)
	movq	8(%rdx,%rcx),%rax
	movnti	%rax,8(%rdi,%rcx)
	movnti	%rax,8(%rsi,%rcx)
	movq	16(%rdx,%rcx),%rax
	movnti	%rax,16(%rdi,%rcx)
	movnti	%rax,16(%rsi,%rcx)
	movq	24(%rdx,%rcx),%rax
	movnti	%rax,24(%rdi,%rcx)
	movnti	%rax,24(%rsi,%rcx)
	addq	$32,%rcx
	jne	1b
	sfence
	POP_FRAME_POINTER
	ret
END(ntb_copy2)

