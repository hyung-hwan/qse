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

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
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

#include <qse/cmn/task.h>
#include "mem.h"

#if defined(_WIN64)
#	if !defined(_WIN32_WINNT)
#		define _WIN32_WINNT 0x0400
#	endif
#	include <windows.h>
#else 
#	include <setjmp.h>
#	if defined(HAVE_UCONTEXT_H)
#		include <ucontext.h>
#	endif
#	if defined(HAVE_MAKECONTEXT) && defined(HAVE_SWAPCONTEXT) && \
	   defined(HAVE_GETCONTEXT) && defined(HAVE_SETCONTEXT)
#		define USE_UCONTEXT
#	endif
#endif

struct qse_task_t
{
	qse_mmgr_t* mmgr;

	qse_task_slice_t* dead;
	qse_task_slice_t* current;

	qse_size_t count;
	qse_task_slice_t* head;
	qse_task_slice_t* tail;

#if defined(_WIN64)
	void* fiber;
#elif defined(USE_UCONTEXT)
	ucontext_t uctx;
#else
	jmp_buf backjmp;
#endif
};

struct qse_task_slice_t
{
#if defined(_WIN64)
	void* fiber;
#elif defined(USE_UCONTEXT)
	ucontext_t uctx;	
#else
	jmp_buf jmpbuf;
#endif

	qse_task_t* task;

	int id;
	qse_task_fnc_t fnc;
	void* ctx;
	qse_size_t stksize;

	qse_task_slice_t* prev;
	qse_task_slice_t* next;
};

int qse_task_init (qse_task_t* task, qse_mmgr_t* mmgr);
void qse_task_fini (qse_task_t* task);

static void purge_slice (qse_task_t* task, qse_task_slice_t* slice);
static void purge_dead_slices (qse_task_t* task);
static void purge_current_slice (qse_task_slice_t* slice, qse_task_slice_t* to);

qse_task_t* qse_task_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_task_t* task;

	task = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*task) + xtnsize);
	if (task == QSE_NULL) return QSE_NULL;
	
	if (qse_task_init (task, mmgr) <= -1)
	{
		QSE_MMGR_FREE (task->mmgr, task);
		return QSE_NULL;
	}

	QSE_MEMSET (task + 1, 0, xtnsize);
	return task;
}

void qse_task_close (qse_task_t* task)
{
	qse_task_fini (task);
	QSE_MMGR_FREE (task->mmgr, task);
}

int qse_task_init (qse_task_t* task, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (task, 0, QSE_SIZEOF(*task));
	task->mmgr = mmgr;
	return 0;
}

void qse_task_fini (qse_task_t* task)
{
	QSE_ASSERT (task->dead == QSE_NULL);
	QSE_ASSERT (task->current == QSE_NULL);

	if (task->count > 0)
	{
		/* am i closing after boot failure? */
		qse_task_slice_t* slice, * next;

		slice = task->head;
		while (slice)
		{
			next = slice->next;
			purge_slice (task, slice);
			slice = next;	
		}
			
		task->count = 0;
		task->head = QSE_NULL;
		task->tail = QSE_NULL;
	}
}

qse_mmgr_t* qse_task_getmmgr (qse_task_t* task)
{
	return task->mmgr;
}

void* qse_task_getxtn (qse_task_t* task)
{
	return (void*)(task + 1);
}

static qse_task_slice_t* alloc_slice (
	qse_task_t* task, qse_task_fnc_t fnc,
	void* ctx, qse_size_t stksize)
{
	qse_task_slice_t* slice;

	slice = QSE_MMGR_ALLOC (task->mmgr, QSE_SIZEOF(*slice) + stksize);
	if (slice == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (slice, 0, QSE_SIZEOF(*slice));
	slice->task = task;
	slice->fnc = fnc;
	slice->ctx = ctx;
	slice->stksize = stksize;

	return slice;
}

static void link_task (qse_task_t* task, qse_task_slice_t* slice)
{
	if (task->head)
	{
		slice->next = task->head;
		task->head->prev = slice;
		task->head = slice;
	}
	else
	{
		task->head = slice;
		task->tail = slice;
	}

	task->count++;
}

#if defined(_WIN64)
#	define __CALL_BACK__ __stdcall
#else
#	define __CALL_BACK__
#endif

#if defined(USE_UCONTEXT) && \
    (QSE_SIZEOF_INT == QSE_SIZEOF_INT32_T) && \
    (QSE_SIZEOF_VOID_P == (QSE_SIZEOF_INT32_T * 2))

static void __CALL_BACK__ execute_current_slice (qse_uint32_t ptr1, qse_uint32_t ptr2)
{
	qse_task_slice_t* slice;
	qse_task_slice_t* to;

	slice = (qse_task_slice_t*)(((qse_uintptr_t)ptr1 << 32) | ptr2);

	QSE_ASSERT (slice->task->current == slice);
	to = slice->fnc (slice->task, slice, slice->ctx);

	/* the task function is now terminated. we need to
	 * purge it from the slice list and switch to the next
	 * slice. */
	purge_current_slice (slice, to);
	QSE_ASSERT (!"must never reach here...");
}

#else

static void __CALL_BACK__ execute_current_slice (qse_task_slice_t* slice)
{
	qse_task_slice_t* to;

	QSE_ASSERT (slice->task->current == slice);
	to = slice->fnc (slice->task, slice, slice->ctx);

	/* the task function is now terminated. we need to
	 * purge it from the slice list and switch to the next
	 * slice. */
	purge_current_slice (slice, to);
	QSE_ASSERT (!"must never reach here...");
}

#endif

#if defined(__WATCOMC__)
/* for watcom, i support i386/32bit only */

extern void* prepare_sp (void*);
#pragma aux prepare_sp = \
	"mov dword ptr[eax+4], esp" \
	"mov esp, eax" \
	"mov eax, dword ptr[esp+0]" \
	parm [eax] value [eax] modify [esp]

extern void* restore_sp (void);
#pragma aux restore_sp = \
	"mov esp, dword ptr[esp+4]" \
	modify [esp]

extern void* get_slice (void);
#pragma aux get_slice = \
	"mov eax, dword ptr[esp+0]" \
	value [eax]

#endif

qse_task_slice_t* qse_task_create (
	qse_task_t* task, qse_task_fnc_t fnc,
	void* ctx, qse_size_t stksize)
{
	qse_task_slice_t* slice;
	register void* tmp;

	stksize = ((stksize + QSE_SIZEOF(void*) - 1) / QSE_SIZEOF(void*)) * QSE_SIZEOF(void*);

#if defined(_WIN64)  
	slice = alloc_slice (task, fnc, ctx, 0);
	if (slice == QSE_NULL) return QSE_NULL;

	slice->fiber = CreateFiberEx (
		stksize, stksize, FIBER_FLAG_FLOAT_SWITCH, 
		execute_current_slice, slice);
	if (slice->fiber == QSE_NULL)
	{
		QSE_MMGR_FREE (task->mmgr, slice);
		return QSE_NULL;
	}

#elif defined(USE_UCONTEXT)

	slice = alloc_slice (task, fnc, ctx, stksize);
	if (slice == QSE_NULL) return QSE_NULL;
	
	if (getcontext (&slice->uctx) <= -1)
	{
		QSE_MMGR_FREE (task->mmgr, slice);
		return QSE_NULL;
	}
	slice->uctx.uc_stack.ss_sp = slice + 1;
	slice->uctx.uc_stack.ss_size = stksize;
	slice->uctx.uc_link = QSE_NULL;

	#if (QSE_SIZEOF_INT == QSE_SIZEOF_INT32_T) && \
	    (QSE_SIZEOF_VOID_P == (QSE_SIZEOF_INT32_T * 2))

	/* limited work around for unclear makecontext parameters */
	makecontext (&slice->uctx, execute_current_slice, 2, 
		(qse_uint32_t)(((qse_uintptr_t)slice) >> 32), 
		(qse_uint32_t)((qse_uintptr_t)slice & 0xFFFFFFFFu));
	#else
	makecontext (&slice->uctx, execute_current_slice, 1, slice);
	#endif

#else

	if (stksize < QSE_SIZEOF(void*) * 3) 
		stksize = QSE_SIZEOF(void*) * 3; /* for t1 & t2 */

	slice = alloc_slice (task, fnc, ctx, stksize);
	if (slice == QSE_NULL) return QSE_NULL;

	/* setjmp() doesn't allow different stacks for
	 * each execution content. let me use some assembly
	 * to change the stack pointer so that setjmp() remembers 
	 * the new stack pointer for longjmp() later. 
	 *
	 * this stack is used for the task function when
	 * longjmp() is made. */
	tmp = ((qse_uint8_t*)(slice + 1)) + stksize - QSE_SIZEOF(void*);

	tmp = (qse_uint8_t*)tmp - QSE_SIZEOF(void*);
	*(void**)tmp = QSE_NULL; /* t1 */

	tmp = (qse_uint8_t*)tmp - QSE_SIZEOF(void*);
	*(void**)tmp = slice; /* t2 */

#if defined(__WATCOMC__)

	tmp = prepare_sp (tmp);

#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))

	__asm__ volatile (
		"movq %%rsp, 8(%1)\n\t" /* t1 = %rsp */
		"movq %1, %%rsp\n\t"    /* %rsp = tmp */
		"movq 0(%%rsp), %0\n"   /* tmp = t2 */
		: "=r"(tmp)
		: "0"(tmp)
		: "%rsp", "memory"
	);

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))

	__asm__ volatile (
		"movl %%esp, 4(%1)\n\t"  /* t1 = %esp */
		"movl %1, %%esp\n\t"     /* %esp = tmp */
		"movl 0(%%esp), %0\n"    /* tmp = t2 */
		: "=r"(tmp)
		: "0"(tmp)
		: "%esp", "memory"
	);

#elif defined(__GNUC__) && (defined(__mips) || defined(mips))

	__asm__ volatile (
 		"sw $sp, 4(%1)\n\t"   /* t1 = $sp */
		"move $sp, %1\n\t"    /* %sp = tmp */
		"lw %0, 0($sp)\n"     /* tmp = t2 */
		: "=r"(tmp)
		: "0"(tmp)
		: "$sp", "memory"
	);

#elif defined(__GNUC__) && defined(__arm__)
	__asm__ volatile (
		"str sp, [%1, #4]\n\t"  /* t1 = sp */
		"mov sp, %1\n"          /* sp = tmp */
		"ldr %0, [sp, #0]\n"     /* tmp = t2 */
		: "=r"(tmp)
		: "0"(tmp)
		: "sp", "memory"
	);

#else
	/* TODO: support more architecture */

	QSE_MMGR_FREE (task->mmgr, task);
	return QSE_NULL;

#endif /* __WATCOMC__ */

	/* 
	 * automatic variables like 'task' and 'newsp' exist
	 * in the old stack. i can't access them.
	 * i access some key informaton via the global
	 * variables stored before stack pointer switching.
	 *
	 * this approach makes this function thread-unsafe.
	 */

	/* when qse_task_create() is called,
	 * setjmp() saves the context and return 0.
	 *
	 * subsequently, when longjmp() is made
	 * for this saved context, setjmp() returns
	 * a non-zero value. 
	 */
	if (setjmp (((qse_task_slice_t*)tmp)->jmpbuf) != 0)
	{
		/* longjmp() is made to here. */
	#if defined(__WATCOMC__)
		tmp = get_slice ();

	#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))
		__asm__ volatile (
			"movq 0(%%rsp), %0\n"  /* tmp = t2 */
			: "=r"(tmp)
		);
	#elif defined(__GNUC__) && (defined(__i386) || defined(i386))
		__asm__ volatile (
			"movl 0(%%esp), %0\n"  /* tmp = t2 */
			: "=r"(tmp)
		);
	#elif defined(__GNUC__) && (defined(__mips) || defined(mips))
		__asm__ volatile (
			"lw %0, 0($sp)\n"    /* tmp = t2 */
			: "=r"(tmp)
		);
	#elif defined(__GNUC__) && defined(__arm__)
		__asm__ volatile (
			"ldr %0, [sp, #0]\n"    /* tmp = t2 */
			: "=r"(tmp)
		);
	#endif /* __WATCOMC__ */

		execute_current_slice ((qse_task_slice_t*)tmp);
		QSE_ASSERT (!"must never reach here....\n");
	}

	/* restore the stack pointer once i finish saving the longjmp() context.
	 * this part is reached only when qse_task_create() is invoked. */
#if defined(__WATCOMC__)

	restore_sp ();

#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))
	/* i assume that %rsp didn't change after the call to setjmp() */
	__asm__ volatile (
		"movq 8(%%rsp), %%rsp\n" /* %rsp = t1 */
		:
		:
		: "%rsp"
	);

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))

	__asm__ volatile (
		"movl 4(%%esp), %%esp\n" /* %esp = t1 */
		:
		:
		: "%esp"
	);

#elif defined(__GNUC__) && (defined(__mips) || defined(mips))

	__asm__ volatile (
		"lw $sp, 4($sp)\n" /* $sp = t1 */
		:
		:
		: "$sp"
	);

#elif defined(__GNUC__) && defined(__arm__)
	__asm__ volatile (
		"ldr sp, [sp, #4]\n"  /* sp = t1 */
		:
		:
		: "sp"
	);

#endif /* __WATCOMC__ */

#endif

	link_task (task, slice);
	return slice;
}

int qse_task_boot (qse_task_t* task, qse_task_slice_t* to)
{
	QSE_ASSERT (task->current == QSE_NULL);

	if (to == QSE_NULL) to = task->head; 

#if defined(_WIN64)
	task->fiber = ConvertThreadToFiberEx (QSE_NULL, FIBER_FLAG_FLOAT_SWITCH);
	if (task->fiber == QSE_NULL) return -1;

	task->current = to;
	SwitchToFiber (task->current->fiber);
	ConvertFiberToThread ();

#elif defined(USE_UCONTEXT)

	task->current = to;
	if (swapcontext (&task->uctx, &task->current->uctx) <= -1) 
	{
		task->current = QSE_NULL;
		return -1;
	}

#else
	if (setjmp (task->backjmp) != 0) 
	{
		/* longjmp() back */
		goto done;
	}
	
	task->current = to;
	longjmp (task->current->jmpbuf, 1);
	QSE_ASSERT (!"must never reach here");

done:
#endif

	QSE_ASSERT (task->current == QSE_NULL);
	QSE_ASSERT (task->count == 0);

	if (task->dead) purge_dead_slices (task);
	return 0;
}

/* NOTE for __WATCOMC__.
   when the number of parameters is more than 2 for qse_task_schedule(),
   this setjmp()/longjmp() based tasking didn't work.

   if i change this to
void qse_task_schedule (qse_task_slice_t* from, qse_task_slice_t* to)
{
	qse_task_t* task = from->task
	....
}

   it worked. i stopped investigating this problem. so i don't know the
   real cause of this problem. let me get back to this later when i have
   time for this.
*/

void qse_task_schedule (
	qse_task_t* task, qse_task_slice_t* from, qse_task_slice_t* to)
{
	QSE_ASSERT (from != QSE_NULL);

	if (to == QSE_NULL)
	{
		/* round-robin if the target is not specified */
		to = from->next? from->next: task->head;
	}

	QSE_ASSERT (task == to->task);
	QSE_ASSERT (task == from->task);
	QSE_ASSERT (task->current == from);

	if (to == from) return;
	task->current = to;

#if defined(_WIN64)
	/* current->fiber is handled by SwitchToFiber() implicitly */
	SwitchToFiber (to->fiber);
	if (task->dead) purge_dead_slices (task);
#elif defined(USE_UCONTEXT)
	swapcontext (&from->uctx, &to->uctx);
	if (task->dead) purge_dead_slices (task);
#else
	if (setjmp (from->jmpbuf) != 0) 
	{
		if (task->dead) purge_dead_slices (task);
		return;
	}
	longjmp (to->jmpbuf, 1);
#endif
}

static void purge_dead_slices (qse_task_t* task)
{
	qse_task_slice_t* slice;

	while (task->dead)
	{
		slice = task->dead;
		task->dead = slice->next;
		purge_slice (task, slice);
	}
}

static void purge_slice (qse_task_t* task, qse_task_slice_t* slice)
{
#if defined(_WIN64)
	if (slice->fiber) DeleteFiber (slice->fiber);
#endif
	QSE_MMGR_FREE (task->mmgr, slice);
}

static void purge_current_slice (qse_task_slice_t* slice, qse_task_slice_t* to)
{
	qse_task_t* task = slice->task;

	QSE_ASSERT (task->current == slice);

	if (task->count == 1)
	{
		/* to purge later */
		slice->next = task->dead;
		task->dead = slice;

		task->current = QSE_NULL;
		task->head = QSE_NULL;
		task->tail = QSE_NULL;
		task->count = 0;
		
#if defined(_WIN64)
		SwitchToFiber (task->fiber);
#elif defined(USE_UCONTEXT)
		setcontext (&task->uctx);
#else
		longjmp (task->backjmp, 1);
#endif
		QSE_ASSERT (!"must not reach here....");
	}
	
	task->current = (to && to != slice)? to: (slice->next? slice->next: task->head);

	if (slice->prev) slice->prev->next = slice->next;
	if (slice->next) slice->next->prev = slice->prev;
	if (slice == task->head) task->head = slice->next;
	if (slice == task->tail) task->tail = slice->prev;
	task->count--;

	/* to purge later */
	slice->next = task->dead;
	task->dead = slice;

#if defined(_WIN64)
	SwitchToFiber (task->current->fiber);
#elif defined(USE_UCONTEXT)
	setcontext (&task->current->uctx);
#else
	longjmp (task->current->jmpbuf, 1);
#endif
}

