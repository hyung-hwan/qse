/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EQSERESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_SI_SPL_H_
#define _QSE_SI_SPL_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_SUPPORT_SPL

typedef volatile qse_uint32_t qse_spl_t;

#define QSE_SPL_INIT (0)

#if defined(QSE_HAVE_INLINE)
	static QSE_INLINE void qse_spl_init (qse_spl_t* spl) {	*spl = QSE_SPL_INIT; }
#else
#	define qse_spl_init(spl) ((*(spl)) = QSE_SPL_INIT)
#endif

#if defined(QSE_HAVE_SYNC_LOCK_TEST_AND_SET) && defined(QSE_HAVE_SYNC_LOCK_RELEASE)
	/* ======================================================================= 
	 * MODERN COMPILERS WITH BUILTIN ATOMICS 
	 * ======================================================================= */

#if defined(QSE_HAVE_INLINE)
	static QSE_INLINE int qse_spl_trylock (qse_spl_t* spl) { return !__sync_lock_test_and_set(spl, 1); }
	static QSE_INLINE void qse_spl_lock (qse_spl_t* spl) { do {} while(__sync_lock_test_and_set(spl, 1)); }
	static QSE_INLINE void qse_spl_unlock (qse_spl_t* spl) { __sync_lock_release(spl); }
#else
#	define qse_spl_trylock(spl) (!__sync_lock_test_and_set(spl, 1))
#	define qse_spl_lock(spl) do {} while(__sync_lock_test_and_set(spl, 1))
#	define qse_spl_unlock(spl) (__sync_lock_release(spl))
#endif

#elif defined(_SCO_DS)
/* ======================================================================= 
 * SCO DEVELOPEMENT SYSTEM
 *
 *  NOTE: when the asm macros were indented, the compiler/linker ended up
 *        with undefined symbols. never indent qse_spl_xxx macros.
 * ======================================================================= */
asm int qse_spl_trylock (qse_spl_t* spl)
{
%reg spl
	movl   $1, %eax
	xchgl  (spl), %eax
	xorl   $1, %eax     / return zero on failure, non-zero on success

%mem spl
	movl  spl,  %ecx
	movl  $1,     %eax
	xchgl (%ecx), %eax
	xorl  $1,     %eax  / return zero on failure, non-zero on success
}

#if 0
/* i can't figure out how to make jump labels unique when there are
 * multiple occurrences of qse_spl_lock(). so let me just use the while loop
 * instead. */ 
asm void qse_spl_lock (qse_spl_t* spl)
{
%reg spl
.lock_set_loop:
	movl  $1, %eax
	xchgl (spl), %eax
	testl %eax, %eax      / set ZF to 1 if eax is zero, 0 if eax is non-zero
	jne    .lock_set_loop / if ZF is 0(eax is non-zero), loop around

%mem spl
.lock_set_loop:
	movl  spl,  %ecx
	movl  $1,     %eax
	xchgl (%ecx), %eax
	testl %eax, %eax      / set ZF to 1 if eax is zero, 0 if eax is non-zero
	jne   .lock_set_loop  / if ZF is 0(eax is non-zero), loop around
}
#else
#define qse_spl_lock(x) do {} while(!spl_trylock(x))
#endif

#if 0
asm void qse_spl_unlock (moo_uint8_t* spl)
{
%reg spl
	movl  $0,      %eax
	xchgl (spl), %eax

%mem spl
	movl  spl,  %ecx
	movl  $0,     %eax
	xchgl (%ecx), %eax
}
#else
asm void qse_spl_unlock (qse_spl_t* spl)
{
	/* don't need xchg as movl on an aligned data is atomic */
	/* mfence is 0F AE F0 */
%reg spl
	.byte 0x0F
	.byte 0xAE
	.byte 0xF0
	movl $0, (spl)

%mem spl
	.byte 0x0F
	.byte 0xAE
	.byte 0xF0
	movl spl, %ecx
	movl $0, (%ecx)
}
#endif


#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))

	static QSE_INLINE int qse_spl_trylock (qse_spl_t* spl) 
	{
		register int x = 1;
		__asm__ volatile (
			"xchgl %0, (%2)\n"
			: "=r"(x)
			: "0"(x), "r"(spl)
			: "memory"
		);
		return !x;
	}
	static QSE_INLINE void qse_spl_lock (qse_spl_t* spl) 
	{
		register int x = 1;
		do
		{
			__asm__ volatile (
				"xchgl %0, (%2)\n"
				: "=r"(x)
				: "0"(x), "r"(spl)
				: "memory"
			);
		}
		while (x);

	}
	static QSE_INLINE void qse_spl_unlock (qse_spl_t* spl) 
	{
	#if defined(__x86_64) || defined(__amd64)
		__asm__ volatile (
			"mfence\n\t"
			"movl $0, (%0)\n"
			:
			:"r"(spl)
			:"memory"
		);
	#else
		__asm__ volatile (
			"movl $0, (%0)\n"
			:
			:"r"(spl)
			:"memory"
		);
	#endif
	}

#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__powerpc) || defined(__powerpc__) || defined(__ppc))

	static QSE_INLINE int qse_spl_trylock (qse_spl_t* spl) 
	{

		/* lwarx	RT, RA, RB
		 *  RT Specifies target general-purpose register where result of operation is stored.
		 *  RA Specifies source general-purpose register for EA calculation.
		 *  RB Specifies source general-purpose register for EA calculation.
		 * 
		 * If general-purpose register (GPR) RA = 0, the effective address (EA) is the
		 * content of GPR RB. Otherwise, the EA is the sum of the content of GPR RA 
		 * plus the content of GPR RB.

		 * The lwarx instruction loads the word from the location in storage specified
		 * by the EA into the target GPR RT. In addition, a reservation on the memory
		 * location is created for use by a subsequent stwcx. instruction.

		 * The lwarx instruction has one syntax form and does not affect the 
		 * Fixed-Point Exception Register. If the EA is not a multiple of 4, 
		 * the results are boundedly undefined.
		 */

		unsigned int rc;

		__asm__ volatile (
			"__back:\n"
			"lwarx        %0,0,%1\n"  /* load and reserve. rc(%0) = *spl(%1) */
			"cmpwi        cr0,%0,0\n" /* cr0 = (rc compare-with 0) */
			"li           %0,0\n"     /* rc = 0(failure) */
			"bne          cr0,__exit\n"   /* if cr0 != 0, goto _exit; */
			"li           %0,1\n"     /* rc = 1(success) */
			"stwcx.       %0,0,%1\n"  /* *spl(%1) = 1(value in rc) if reserved */
			"bne          cr0,__back\n"   /* if reservation is lost, goto __back */
		#if 1
			"lwsync\n"
		#else
			"isync\n"
		#endif
			"__exit:\n"
			: "=&r"(rc)
			: "r"(spl)
			: "cr0", "memory"
		);

		return rc;
	}
	static QSE_INLINE void qse_spl_lock (qse_spl_t* spl) 
	{
		while (!qse_spl_trylock(spl)) /* nothing */;
	}
	static QSE_INLINE void qse_spl_unlock (qse_spl_t* spl) 
	{
		__asm__ volatile (
		#if 1
			"lwsync\n" 
		#elif 0
			"sync\n" 
		#else
			"eieio\n" 
		#endif
			:
			:
			: "memory"
		);
		*spl = 0;
	}

#elif defined(QSE_SPL_NO_UNSUPPORTED_ERROR)
	/* don't raise the compile time error */
	#undef QSE_SUPPORT_SPL
#else
	#undef QSE_SUPPORT_SPL
#	error UNSUPPORTED
#endif


#endif
