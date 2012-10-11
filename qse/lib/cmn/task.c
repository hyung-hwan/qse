/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/task.h>

#if defined(_WIN64)
#	include <windows.h>
#else 
#	if defined(HAVE_UCONTEXT_H)
#		include <ucontext.h>
#	endif
#	include <setjmp.h>
#endif

typedef struct tmgr_t tmgr_t;
struct tmgr_t
{
	int count;
	int idinc;

	qse_task_t* dead;
	qse_task_t* current;
	qse_task_t* head;
	qse_task_t* tail;

#if defined(_WIN64)
	void* fiber;
#elif defined(HAVE_SWAPCONTEXT)
	ucontext_t uctx;
#else
	jmp_buf backjmp;
#endif
};

struct qse_task_t
{
     /* queue */
	tmgr_t* tmgr;

	int id;

	qse_task_fnc_t fnc;
	void* ctx;
	qse_size_t stsize;

#if defined(_WIN64)
	void* fiber;
#elif defined(HAVE_SWAPCONTEXT)
	ucontext_t uctx;	
#else
	jmp_buf jmpbuf;
#endif

	qse_task_t* prev;
	qse_task_t* next;
};

static tmgr_t* tmgr;

qse_task_t* qse_task_alloc (qse_task_fnc_t fnc, void* ctx, qse_size_t stsize)
{
	qse_task_t* task;

	task = malloc (QSE_SIZEOF(*task) + stsize);
	if (task == QSE_NULL) return QSE_NULL;

	//QSE_MEMSET (task, 0, QSE_SIZEOF(*task));
	QSE_MEMSET (task, 0, QSE_SIZEOF(*task) + stsize);
	task->tmgr = tmgr;
	task->fnc = fnc;
	task->ctx = ctx;
	task->stsize = stsize;

	if (tmgr->head)
	{
		task->next= tmgr->head;
		tmgr->head->prev = task;
		tmgr->head = task;
	}
	else
	{
		tmgr->head = task;
		tmgr->tail = task;
	}

	task->id = tmgr->idinc;
	tmgr->count++;
	tmgr->idinc++;

	return task;
}

static void purge_dead_tasks (void)
{
	qse_task_t* x;

	while (tmgr->dead)
	{
		x = tmgr->dead;
		tmgr->dead = x->next;
#if defined(_WIN64)
		DeleteFiber (x->fiber);
#endif
		free (x);
	}
}

void qse_task_schedule (void)
{
	qse_task_t* current;

	current = tmgr->current; /* old current */
	tmgr->current = current->next? current->next: tmgr->head;
		
#if defined(_WIN64)
	/* current->fiber is handled by SwitchToFiber() implicitly */
	SwitchToFiber (tmgr->current->fiber);
	purge_dead_tasks ();
#elif defined(HAVE_SWAPCONTEXT)
	swapcontext (&current->uctx, &tmgr->current->uctx);
	purge_dead_tasks ();
#else
//printf ("switch from %d to %d\n", current->id, tmgr->current->id);
	if (setjmp (current->jmpbuf) != 0) 
	{
		purge_dead_tasks ();
		return;
	}
	longjmp (tmgr->current->jmpbuf, 1);
#endif
}

static void purge_current_task (void)
{
	qse_task_t* current;

	if (tmgr->count == 1)
	{
		/* to purge later */
		tmgr->current->next = tmgr->dead;
		tmgr->dead = tmgr->current;

		tmgr->current = QSE_NULL;
		tmgr->head = QSE_NULL;
		tmgr->tail = QSE_NULL;
		tmgr->count = 0;
		tmgr->idinc = 0;
		
#if defined(_WIN64)
		SwitchToFiber (tmgr->fiber);
#elif defined(HAVE_SWAPCONTEXT)
		setcontext (&tmgr->uctx);
#else
		longjmp (tmgr->backjmp, 1);
#endif
		assert (!"must not reach here....");
	}
	
	current = tmgr->current;
	tmgr->current = current->next? current->next: tmgr->head;

	if (current->prev) current->prev->next = current->next;
	if (current->next) current->next->prev = current->prev;
	if (current == tmgr->head) tmgr->head = current->next;
	if (current == tmgr->tail) tmgr->tail = current->prev;
	tmgr->count--;

	/* to purge later */
	current->next = tmgr->dead;
	tmgr->dead = current;

#if defined(_WIN64)
	SwitchToFiber (tmgr->current->fiber);
#elif defined(HAVE_SWAPCONTEXT)
	setcontext (&tmgr->current->uctx);
#else
	longjmp (tmgr->current->jmpbuf, 1);
#endif
}

#if defined(_WIN64)
static void __stdcall execute_current_task (void* task)
{
	assert (tmgr->current != QSE_NULL);
	tmgr->current->fnc (tmgr->current->ctx);
	purge_current_task ();
}
#else

static void execute_current_task (void)
{
	assert (tmgr->current != QSE_NULL);

	tmgr->current->fnc (tmgr->current->ctx);

	/* the task function is now terminated. we need to
	 * purge it from the task list */
	purge_current_task ();
}
#endif

static qse_task_t* xxtask;
static void* xxoldsp;

#if defined(__WATCOMC__)
/* for watcom, i support i386/32bit only */
extern void* set_sp (void*);
#pragma aux set_sp = \
	"xchg eax, esp"  \
	parm [eax] value [eax] modify [esp]
#endif

qse_task_t* qse_maketask (qse_task_fnc_t fnc, void* ctx, qse_size_t stsize)
{
	qse_task_t* task;
	void* newsp;


#if defined(_WIN64)  
	task = qse_task_alloc (fnc, ctx, 0);
	if (task == QSE_NULL) return QSE_NULL;

	task->fiber = CreateFiberEx (stsize, stsize, FIBER_FLAG_FLOAT_SWITCH, execute_current_task, QSE_NULL);
	if (task->fiber == QSE_NULL)
	{
/* TODO: delete task */
		return QSE_NULL;
	}

#elif defined(HAVE_SWAPCONTEXT)

	task = qse_task_alloc (fnc, ctx, stsize);
	if (task == QSE_NULL) return QSE_NULL;
	
	if (getcontext (&task->uctx) <= -1)
	{
/* TODO: delete task */
		return QSE_NULL;
	}
	task->uctx.uc_stack.ss_sp = task + 1;
	task->uctx.uc_stack.ss_size = stsize;
	task->uctx.uc_link = QSE_NULL;
	makecontext (&task->uctx, execute_current_task, 0);

#else

	task = qse_task_alloc (fnc, ctx, stsize);
	if (task == QSE_NULL) return QSE_NULL;

	/* setjmp() doesn't allow different stacks for
	 * each execution content. let me use some assembly
	 * to change the stack pointer so that setjmp() remembers 
	 * the new stack pointer for longjmp() later. 
	 *
	 * this stack is used for the task function when
	 * longjmp() is made. */
	xxtask = task;
	newsp = ((qse_uint8_t*)(task + 1)) + stsize - QSE_SIZEOF(void*);

#if defined(__WATCOMC__)

	xxoldsp = set_sp (newsp);

#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))

	/*
	__asm__ volatile (
		"xchgq %0, %%rsp\n"
		: "=r"(xxoldsp)
		: "0"(newsp) 
		: "%rsp"
	);
	*/

	__asm__ volatile (
		"movq %%rsp, %0\n\t" 
		"movq %1, %%rsp\n" 
		: "=m"(xxoldsp)
		: "r"(newsp)
		: "%rsp", "memory"
	);

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))
	__asm__ volatile (
		"xchgl %0, %%esp\n"
		: "=r"(xxoldsp)
		: "0"(newsp)
		: "%esp"
	);
#elif defined(__GNUC__) && (defined(__mips) || defined(mips))
	__asm__ volatile (
		"sw $sp, %0\n\t" /* store $sp to xxoldsp */
		"move $sp, %1\n" /* move the register content for newsp to $sp */
		: "=m"(xxoldsp)
		: "r"(newsp)
		: "$sp", "memory"
	);
	/*
	__asm__ volatile (
		"move %0, $sp\n\t"
		"move $sp, %1\n"
		: "=&r"(xxoldsp)
		: "r"(newsp)
		: "$sp", "memory"
	);
	*/

#elif defined(__GNUC__) && defined(__arm__)
	__asm__ volatile (
		"str sp, %0\n\t" 
		"mov sp, %1\n"
		: "=m"(xxoldsp)
		: "r"(newsp)
		: "sp", "memory"
	);

/* TODO: support more architecture */
#else

	/* TODO: destroy task */
	//tmgr->errnum = QSE_TMGR_ENOIMPL;
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

	/* when qse_maketask() is called,
	 * setjmp() saves the context and return 0.
	 *
	 * subsequently, when longjmp() is made
	 * for this saved context, setjmp() returns
	 * a non-zero value. 
	 */
	if (setjmp (xxtask->jmpbuf) != 0)
	{
		/* longjmp() is made to here. */
		execute_current_task ();
		assert (!"must never reach here....\n");
	}

	/* restore the stack pointer once i finish saving the longjmp() context.
	 * this part is reached only when qse_maketask() is invoked. */
#if defined(__WATCOMC__)

	set_sp (xxoldsp);

#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))
	__asm__ volatile (
		"movq %0, %%rsp\n"
		: 
		: "m"(xxoldsp) /*"r"(xxoldsp)*/
		: "%rsp" 
	);

#elif defined(__GNUC__) && (defined(__i386) || defined(i386))
	__asm__ volatile (
		"movl %0, %%esp\n" 
		: 
		: "r"(xxoldsp)
		: "%esp" 
	);

#elif defined(__GNUC__) && (defined(__mips) || defined(mips))
	__asm__ volatile (
		"lw $sp, %0\n" /*"move $sp, %0\n" */
		: 
		: "m"(xxoldsp) /* "r"(xxoldsp) */
		: "$sp"
	);
#elif defined(__GNUC__) && defined(__arm__)
	__asm__ volatile (
		"ldr sp, %0\n" 
		: 
		: "m"(xxoldsp)
		: "sp"
	);
#endif /* __WATCOMC__ */

#endif

	return task;
}

int qse_gettaskid (qse_task_t* task)
{
	return task->id;
}

int qse_task_boot (void)
{
	if (tmgr->count <= 0) return -1;

#if defined(_WIN64)
	tmgr->fiber = ConvertThreadToFiberEx (QSE_NULL, FIBER_FLAG_FLOAT_SWITCH);
	if (tmgr->fiber == QSE_NULL) 
	{
/*TODO: destroy all the tasks created */
		return -1;
	}

	tmgr->current = tmgr->tail;
	SwitchToFiber (tmgr->current->fiber);
	ConvertFiberToThread ();

#elif defined(HAVE_SWAPCONTEXT)

	tmgr->current = tmgr->tail;
	if (swapcontext (&tmgr->uctx, &tmgr->current->uctx) <= -1)
	{
/*TODO: destroy all the tasks created */
		return -1;
	}

#else
	if (setjmp (tmgr->backjmp) != 0) 
	{
		/* longjmp() back */
		goto done;
	}
	
	tmgr->current = tmgr->tail;
	longjmp (tmgr->current->jmpbuf, 1);
	assert (!"must never reach here");

done:
#endif

	assert (tmgr->current == QSE_NULL);
	assert (tmgr->count == 0);

	purge_dead_tasks ();
	printf ("END OF TASK_BOOT...\n");
	return 0;
}

