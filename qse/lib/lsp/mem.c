/*
 * $Id: mem.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

qse_lsp_mem_t* qse_lsp_openmem (
	qse_lsp_t* lsp, qse_size_t ubound, qse_size_t ubound_inc)
{
	qse_lsp_mem_t* mem;
	qse_size_t i;

	/* allocate memory */
	mem = (qse_lsp_mem_t*) QSE_LSP_ALLOC (lsp, QSE_SIZEOF(qse_lsp_mem_t));	
	if (mem == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (mem, 0, QSE_SIZEOF(qse_lsp_mem_t));
	mem->lsp = lsp;

	/* create a new root environment frame */
	mem->frame = qse_lsp_newframe (lsp);
	if (mem->frame == QSE_NULL) 
	{
		QSE_LSP_FREE (lsp, mem);
		return QSE_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = QSE_NULL;
	mem->tlink          = QSE_NULL;
	mem->tlink_count    = 0;

	/* initialize object allocation list */
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < QSE_LSP_TYPE_COUNT; i++) 
	{
		mem->used[i] = QSE_NULL;
		mem->free[i] = QSE_NULL;
	}
	mem->read = QSE_NULL;

	/* when "ubound" is too small, the garbage collection can
	 * be performed while making the common objects. */
	mem->nil    = QSE_NULL;
	mem->t      = QSE_NULL;
	mem->quote  = QSE_NULL;
	mem->lambda = QSE_NULL;
	mem->macro  = QSE_NULL;

	/* initialize common object pointers */
	mem->nil     = qse_lsp_makenil  (mem);
	mem->t       = qse_lsp_maketrue (mem);
	mem->quote   = qse_lsp_makesym  (mem, QSE_T("quote"),  5);
	mem->lambda  = qse_lsp_makesym  (mem, QSE_T("lambda"), 6);
	mem->macro   = qse_lsp_makesym  (mem, QSE_T("macro"),  5);

	if (mem->nil    == QSE_NULL ||
	    mem->t      == QSE_NULL ||
	    mem->quote  == QSE_NULL ||
	    mem->lambda == QSE_NULL ||
	    mem->macro  == QSE_NULL) 
	{
		qse_lsp_dispose_all (mem);
		qse_lsp_freeframe (lsp, mem->frame);
		QSE_LSP_FREE (lsp, mem);
		return QSE_NULL;
	}

	QSE_LSP_PERM(mem->nil)    = 1;
	QSE_LSP_PERM(mem->t)      = 1;
	QSE_LSP_PERM(mem->quote)  = 1;
	QSE_LSP_PERM(mem->lambda) = 1;
	QSE_LSP_PERM(mem->macro)  = 1;

	return mem;
}

void qse_lsp_closemem (qse_lsp_mem_t* mem)
{
	/* dispose of the allocated objects */
	qse_lsp_dispose_all (mem);

	/* dispose of environment frames */
	qse_lsp_freeframe (mem->lsp, mem->frame);

	/* free the memory */
	QSE_LSP_FREE (mem->lsp, mem);
}

qse_lsp_obj_t* qse_lsp_alloc (qse_lsp_mem_t* mem, int type, qse_size_t size)
{
	qse_lsp_obj_t* obj;
	
/* TODO: remove the following line... */
qse_lsp_gc (mem);

	if (mem->count >= mem->ubound) qse_lsp_gc (mem);
	if (mem->count >= mem->ubound) 
	{
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return QSE_NULL;
	}

	obj = (qse_lsp_obj_t*) QSE_LSP_ALLOC (mem->lsp, size);
	if (obj == QSE_NULL) 
	{
		qse_lsp_gc (mem);

		obj = (qse_lsp_obj_t*) QSE_LSP_ALLOC (mem->lsp, size);
		if (obj == QSE_NULL) 
		{
			qse_lsp_seterror (mem->lsp, QSE_LSP_ENOMEM, QSE_NULL, 0);
			return QSE_NULL;
		}
	}

	QSE_LSP_TYPE(obj) = type;
	QSE_LSP_SIZE(obj) = size;
	QSE_LSP_MARK(obj) = 0;
	QSE_LSP_PERM(obj) = 0;
	QSE_LSP_LOCK(obj) = 0;

	/* insert the object at the head of the used list */
	QSE_LSP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;

#if 0
	qse_dprint1 (QSE_T("mem->count: %u\n"), mem->count);
#endif

	return obj;
}

void qse_lsp_dispose (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* prev, qse_lsp_obj_t* obj)
{
	QSE_ASSERT (obj != QSE_NULL);
	QSE_ASSERT (mem->count > 0);

	/* TODO: push the object to the free list for more 
	 *       efficient memory management */

	if (prev == QSE_NULL) 
		mem->used[QSE_LSP_TYPE(obj)] = QSE_LSP_LINK(obj);
	else QSE_LSP_LINK(prev) = QSE_LSP_LINK(obj);

	mem->count--;
#if 0
	qse_dprint1 (QSE_T("mem->count: %u\n"), mem->count);
#endif

	QSE_LSP_FREE (mem->lsp, obj);	
}

void qse_lsp_dispose_all (qse_lsp_mem_t* mem)
{
	qse_lsp_obj_t* obj, * next;
	qse_size_t i;

	for (i = 0; i < QSE_LSP_TYPE_COUNT; i++) 
	{
		obj = mem->used[i];

		while (obj != QSE_NULL) 
		{
			next = QSE_LSP_LINK(obj);
			qse_lsp_dispose (mem, QSE_NULL, obj);
			obj = next;
		}
	}
}

static void __mark_obj (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	QSE_ASSERT (obj != QSE_NULL);

	/* TODO: can it be recursive? */
	if (QSE_LSP_MARK(obj) != 0) return;

	QSE_LSP_MARK(obj) = 1;

	if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS) 
	{
		__mark_obj (lsp, QSE_LSP_CAR(obj));
		__mark_obj (lsp, QSE_LSP_CDR(obj));
	}
	else if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_FUNC) 
	{
		__mark_obj (lsp, QSE_LSP_FFORMAL(obj));
		__mark_obj (lsp, QSE_LSP_FBODY(obj));
	}
	else if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_MACRO) 
	{
		__mark_obj (lsp, QSE_LSP_MFORMAL(obj));
		__mark_obj (lsp, QSE_LSP_MBODY(obj));
	}
}

/*
 * qse_lsp_lockobj and qse_lsp_deepunlockobj are just called by qse_lsp_read.
 */
void qse_lsp_lockobj (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	QSE_ASSERTX (obj != QSE_NULL,
		"an object pointer should not be QSE_NULL");
	if (QSE_LSP_PERM(obj) == 0) QSE_LSP_LOCK(obj)++;
}

void qse_lsp_unlockobj (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	QSE_ASSERTX (obj != QSE_NULL,
		"an object pointer should not be QSE_NULL");

	if (QSE_LSP_PERM(obj) != 0) return;
	QSE_ASSERTX (QSE_LSP_LOCK(obj) > 0,
		"the lock count should be greater than zero to be unlocked");

	QSE_LSP_LOCK(obj)--;
}

void qse_lsp_deepunlockobj (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	QSE_ASSERTX (obj != QSE_NULL,
		"an object pointer should not be QSE_NULL");

	if (QSE_LSP_PERM(obj) == 0) 
	{
		QSE_ASSERTX (QSE_LSP_LOCK(obj) > 0,
			"the lock count should be greater than zero to be unlocked");
		QSE_LSP_LOCK(obj)--;
	}

	if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS) 
	{
		qse_lsp_deepunlockobj (lsp, QSE_LSP_CAR(obj));
		qse_lsp_deepunlockobj (lsp, QSE_LSP_CDR(obj));
	}
	else if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_FUNC) 
	{
		qse_lsp_deepunlockobj (lsp, QSE_LSP_FFORMAL(obj));
		qse_lsp_deepunlockobj (lsp, QSE_LSP_FBODY(obj));
	}
	else if (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_MACRO) 
	{
		qse_lsp_deepunlockobj (lsp, QSE_LSP_MFORMAL(obj));
		qse_lsp_deepunlockobj (lsp, QSE_LSP_MBODY(obj));
	}
}

static void __mark_objs_in_use (qse_lsp_mem_t* mem)
{
	qse_lsp_frame_t* frame;
	qse_lsp_assoc_t* assoc;
	qse_lsp_tlink_t* tlink;
	/*qse_lsp_arr_t* arr;*/
	/*qse_size_t       i;*/

#if 0
	qse_dprint0 (QSE_T("marking environment frames\n"));
#endif
	/* mark objects in the environment frames */
	frame = mem->frame;
	while (frame != QSE_NULL) 
	{
		assoc = frame->assoc;
		while (assoc != QSE_NULL) 
		{
			__mark_obj (mem->lsp, assoc->name);

			if (assoc->value != QSE_NULL) 
				__mark_obj (mem->lsp, assoc->value);
			if (assoc->func != QSE_NULL) 
				__mark_obj (mem->lsp, assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

#if 0
	qse_dprint0 (QSE_T("marking interim frames\n"));
#endif

	/* mark objects in the interim frames */
	frame = mem->brooding_frame;
	while (frame != QSE_NULL) 
	{
		assoc = frame->assoc;
		while (assoc != QSE_NULL) 
		{
			__mark_obj (mem->lsp, assoc->name);

			if (assoc->value != QSE_NULL) 
				__mark_obj (mem->lsp, assoc->value);
			if (assoc->func != QSE_NULL) 
				__mark_obj (mem->lsp, assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/* qse_dprint0 (QSE_T("marking the read object\n"));*/
	if (mem->read != QSE_NULL) __mark_obj (mem->lsp, mem->read);

	/* qse_dprint0 (QSE_T("marking the temporary objects\n"));*/
	for (tlink = mem->tlink; tlink != QSE_NULL; tlink = tlink->link)
	{
		__mark_obj (mem->lsp, tlink->obj);
	}

#if 0
	qse_dprint0 (QSE_T("marking builtin objects\n"));
#endif
	/* mark common objects */
	if (mem->t      != QSE_NULL) __mark_obj (mem->lsp, mem->t);
	if (mem->nil    != QSE_NULL) __mark_obj (mem->lsp, mem->nil);
	if (mem->quote  != QSE_NULL) __mark_obj (mem->lsp, mem->quote);
	if (mem->lambda != QSE_NULL) __mark_obj (mem->lsp, mem->lambda);
	if (mem->macro  != QSE_NULL) __mark_obj (mem->lsp, mem->macro);
}

//#include <qse/utl/stdio.h>
static void __sweep_unmarked_objs (qse_lsp_mem_t* mem)
{
	qse_lsp_obj_t* obj, * prev, * next;
	qse_size_t i;

	/* scan all the allocated objects and get rid of unused objects */
	for (i = 0; i < QSE_LSP_TYPE_COUNT; i++) 
	{
		prev = QSE_NULL;
		obj = mem->used[i];

#if 0
		qse_dprint1 (QSE_T("sweeping objects of type: %u\n"), i);
#endif
		while (obj != QSE_NULL) 
		{
			next = QSE_LSP_LINK(obj);

			if (QSE_LSP_LOCK(obj) == 0 && 
			    QSE_LSP_MARK(obj) == 0 && 
			    QSE_LSP_PERM(obj) == 0) 
			{
				/* dispose of unused objects */
/*
if (i == QSE_LSP_OBJ_INT)
qse_printf (QSE_T("disposing....%d [%d]\n"), i, (int)QSE_LSP_IVAL(obj));
if (i == QSE_LSP_OBJ_REAL)
qse_printf (QSE_T("disposing....%d [%Lf]\n"), i, (double)QSE_LSP_RVAL(obj));
else if (i == QSE_LSP_OBJ_SYM)
qse_printf (QSE_T("disposing....%d [%s]\n"), i, QSE_LSP_SYMPTR(obj));
else if (i == QSE_LSP_OBJ_STR)
qse_printf (QSE_T("disposing....%d [%s]\n"), i, QSE_LSP_STRPTR(obj));
else
qse_printf (QSE_T("disposing....%d\n"), i);
*/
				qse_lsp_dispose (mem, prev, obj);
			}
			else 
			{
				/* unmark the object in use */
				QSE_LSP_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

void qse_lsp_gc (qse_lsp_mem_t* mem)
{
	__mark_objs_in_use (mem);
	__sweep_unmarked_objs (mem);
}

qse_lsp_obj_t* qse_lsp_makenil (qse_lsp_mem_t* mem)
{
	if (mem->nil != QSE_NULL) return mem->nil;
	mem->nil = qse_lsp_alloc (
		mem, QSE_LSP_OBJ_NIL, QSE_SIZEOF(qse_lsp_obj_nil_t));
	return mem->nil;
}

qse_lsp_obj_t* qse_lsp_maketrue (qse_lsp_mem_t* mem)
{
	if (mem->t != QSE_NULL) return mem->t;
	mem->t = qse_lsp_alloc (
		mem, QSE_LSP_OBJ_TRUE, QSE_SIZEOF(qse_lsp_obj_true_t));
	return mem->t;
}

qse_lsp_obj_t* qse_lsp_makeintobj (qse_lsp_mem_t* mem, qse_long_t value)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (mem, 
		QSE_LSP_OBJ_INT, QSE_SIZEOF(qse_lsp_obj_int_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_LSP_IVAL(obj) = value;

	return obj;
}

qse_lsp_obj_t* qse_lsp_makerealobj (qse_lsp_mem_t* mem, qse_real_t value)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (mem, 
		QSE_LSP_OBJ_REAL, QSE_SIZEOF(qse_lsp_obj_real_t));
	if (obj == QSE_NULL) return QSE_NULL;
	
	QSE_LSP_RVAL(obj) = value;

	return obj;
}

qse_lsp_obj_t* qse_lsp_makesym (
	qse_lsp_mem_t* mem, const qse_char_t* str, qse_size_t len)
{
	qse_lsp_obj_t* obj;

	/* look for a sysmbol with the given name */
	obj = mem->used[QSE_LSP_OBJ_SYM];
	while (obj != QSE_NULL) 
	{
		/* if there is a symbol with the same name, it is just used. */
		if (qse_strxncmp (
			QSE_LSP_SYMPTR(obj), 
			QSE_LSP_SYMLEN(obj), 
			str, len) == 0) return obj;
		obj = QSE_LSP_LINK(obj);
	}

	/* no such symbol found. create a new one */
	obj = qse_lsp_alloc (mem, QSE_LSP_OBJ_SYM,
		QSE_SIZEOF(qse_lsp_obj_sym_t)+(len + 1)*QSE_SIZEOF(qse_char_t));
	if (obj == QSE_NULL) return QSE_NULL;

	/* fill in the symbol buffer */
	qse_strncpy (QSE_LSP_SYMPTR(obj), str, len);

	return obj;
}

qse_lsp_obj_t* qse_lsp_makestr (
	qse_lsp_mem_t* mem, const qse_char_t* str, qse_size_t len)
{
	qse_lsp_obj_t* obj;

	/* allocate memory for the string */
	obj = qse_lsp_alloc (mem, QSE_LSP_OBJ_STR,
		QSE_SIZEOF(qse_lsp_obj_str_t)+(len + 1)*QSE_SIZEOF(qse_char_t));
	if (obj == QSE_NULL) return QSE_NULL;

	/* fill in the string buffer */
	qse_strncpy (QSE_LSP_STRPTR(obj), str, len);

	return obj;
}

qse_lsp_obj_t* qse_lsp_makecons (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* car, qse_lsp_obj_t* cdr)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (mem,
		QSE_LSP_OBJ_CONS, QSE_SIZEOF(qse_lsp_obj_cons_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_LSP_CAR(obj) = car;
	QSE_LSP_CDR(obj) = cdr;

	return obj;
}

qse_lsp_obj_t* qse_lsp_makefunc (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* formal, qse_lsp_obj_t* body)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (mem,
		QSE_LSP_OBJ_FUNC, QSE_SIZEOF(qse_lsp_obj_func_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_LSP_FFORMAL(obj) = formal;
	QSE_LSP_FBODY(obj)   = body;

	return obj;
}

qse_lsp_obj_t* qse_lsp_makemacro (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* formal, qse_lsp_obj_t* body)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (mem, 
		QSE_LSP_OBJ_MACRO, QSE_SIZEOF(qse_lsp_obj_macro_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_LSP_MFORMAL(obj) = formal;
	QSE_LSP_MBODY(obj)   = body;

	return obj;
}

qse_lsp_obj_t* qse_lsp_makeprim (qse_lsp_mem_t* mem, 
	qse_lsp_prim_t impl, qse_size_t min_args, qse_size_t max_args)
{
	qse_lsp_obj_t* obj;

	obj = qse_lsp_alloc (
		mem, QSE_LSP_OBJ_PRIM, QSE_SIZEOF(qse_lsp_obj_prim_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_LSP_PIMPL(obj) = impl;
	QSE_LSP_PMINARGS(obj) = min_args;
	QSE_LSP_PMAXARGS(obj) = max_args;
	return obj;
}

qse_lsp_assoc_t* qse_lsp_lookup (qse_lsp_mem_t* mem, qse_lsp_obj_t* name)
{
	qse_lsp_frame_t* frame;
	qse_lsp_assoc_t* assoc;

	QSE_ASSERT (QSE_LSP_TYPE(name) == QSE_LSP_OBJ_SYM);

	frame = mem->frame;

	while (frame != QSE_NULL) 
	{
		assoc = qse_lsp_lookupinframe (mem->lsp, frame, name);
		if (assoc != QSE_NULL) return assoc;
		frame = frame->link;
	}

	return QSE_NULL;
}

qse_lsp_assoc_t* qse_lsp_setvalue (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* name, qse_lsp_obj_t* value)
{
	qse_lsp_assoc_t* assoc;

	assoc = qse_lsp_lookup (mem, name);
	if (assoc == QSE_NULL)
	{
		assoc = qse_lsp_insvalueintoframe (
			mem->lsp, mem->root_frame, name, value);
		if (assoc == QSE_NULL) return QSE_NULL;
	}
	else assoc->value = value;

	return assoc;
}

qse_lsp_assoc_t* qse_lsp_setfunc (
	qse_lsp_mem_t* mem, qse_lsp_obj_t* name, qse_lsp_obj_t* func)
{
	qse_lsp_assoc_t* assoc;

	assoc = qse_lsp_lookup (mem, name);
	if (assoc == QSE_NULL) 
	{
		assoc = qse_lsp_insfuncintoframe (
			mem->lsp, mem->root_frame, name, func);
		if (assoc == QSE_NULL) return QSE_NULL;
	}
	else assoc->func = func;

	return assoc;
}

qse_size_t qse_lsp_conslen (qse_lsp_mem_t* mem, qse_lsp_obj_t* obj)
{
	qse_size_t count;

	QSE_ASSERT (
		obj == mem->nil || QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS);

	count = 0;
	/*while (obj != mem->nil) {*/
	while (QSE_LSP_TYPE(obj) == QSE_LSP_OBJ_CONS) 
	{
		count++;
		obj = QSE_LSP_CDR(obj);
	}

	return count;
}



