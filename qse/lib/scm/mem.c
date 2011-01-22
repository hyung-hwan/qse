/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include "scm.h"

static qse_scm_obj_t* makeint (qse_scm_mem_t* mem, qse_long_t value);
static QSE_INLINE_ALWAYS void collect_garbage (qse_scm_mem_t* mem);
static void dispose_all (qse_scm_mem_t* mem);

qse_scm_mem_t* qse_scm_mem_init (
	qse_scm_mem_t* mem, qse_scm_t* scm,
	qse_size_t ubound, qse_size_t ubound_inc)
{
	qse_size_t i;
	int fail = 0;

	QSE_MEMSET (mem, 0, QSE_SIZEOF(qse_scm_mem_t));
	mem->scm = scm;

	/* create a new root environment frame */
	mem->frame = qse_scm_newframe (scm);
	if (mem->frame == QSE_NULL) return QSE_NULL;

	mem->root_frame     = mem->frame;
	mem->brooding_frame = QSE_NULL;
	mem->tlink          = QSE_NULL;
	mem->tlink_count    = 0;

	/* initialize object allocation list */
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < QSE_SCM_TYPE_COUNT; i++) 
	{
		mem->used[i] = QSE_NULL;
		mem->free[i] = QSE_NULL;
	}

	/* initialize read registers */
	mem->r.obj = QSE_NULL;
	mem->r.tmp = QSE_NULL;
	mem->r.stack = QSE_NULL;

	/* when "ubound" is too small, the garbage collection can
	 * be performed while making the common objects. */
	mem->nil    = QSE_NULL;
	mem->t      = QSE_NULL;
	mem->quote  = QSE_NULL;
	mem->lambda = QSE_NULL;
	mem->macro  = QSE_NULL;
	for (i = 0; i < QSE_COUNTOF(mem->num); i++) mem->num[i] = QSE_NULL;

	/* initialize common object pointers */
	mem->nil = qse_scm_makenil (mem);
	mem->t = qse_scm_maketrue (mem);
	mem->quote = qse_scm_makesym (mem, QSE_T("quote"), 5);
	mem->lambda = qse_scm_makesym (mem, QSE_T("lambda"), 6);
	mem->macro = qse_scm_makesym (mem, QSE_T("macro"), 5);

	if (mem->nil == QSE_NULL ||
	    mem->t == QSE_NULL ||
	    mem->quote == QSE_NULL ||
	    mem->lambda == QSE_NULL ||
	    mem->macro == QSE_NULL) 
	{
		fail = 1;
	}
	else
	{
		for (i = 0; i < QSE_COUNTOF(mem->num); i++)
		{
			mem->num[i] = makeint (mem, i);
			if (mem->num[i] == QSE_NULL) { fail = 1; break; }
		}
	}

	if (fail)
	{
		dispose_all (mem);
		qse_scm_freeframe (scm, mem->frame);
		return QSE_NULL;
	}

	QSE_SCM_PERM(mem->nil)    = 1;
	QSE_SCM_PERM(mem->t)      = 1;
	QSE_SCM_PERM(mem->quote)  = 1;
	QSE_SCM_PERM(mem->lambda) = 1;
	QSE_SCM_PERM(mem->macro)  = 1;
	for (i = 0; i < QSE_COUNTOF(mem->num); i++)
	{
		QSE_SCM_PERM(mem->num[i]) = 1;
	}

	/* let the read stack point to nil */
	mem->r.stack = mem->nil;

	return mem;
}

void qse_scm_mem_fini (qse_scm_mem_t* mem)
{
	/* dispose of the allocated objects */
	dispose_all (mem);

	/* dispose of environment frames */
	qse_scm_freeframe (mem->scm, mem->frame);
}

static qse_scm_obj_t* allocate (qse_scm_mem_t* mem, int type, qse_size_t size)
{
	qse_scm_obj_t* obj;
	
	if (mem->count >= mem->ubound) collect_garbage (mem);
	if (mem->count >= mem->ubound) 
	{
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return QSE_NULL;
	}

	obj = (qse_scm_obj_t*) QSE_SCM_ALLOC (mem->scm, size);
	if (obj == QSE_NULL) 
	{
		collect_garbage (mem);

		obj = (qse_scm_obj_t*) QSE_SCM_ALLOC (mem->scm, size);
		if (obj == QSE_NULL) 
		{
			qse_scm_seterror (mem->scm, QSE_SCM_ENOMEM, QSE_NULL, 0);
			return QSE_NULL;
		}
	}

	QSE_SCM_TYPE(obj) = type;
	QSE_SCM_SIZE(obj) = size;
	QSE_SCM_MARK(obj) = 0;
	QSE_SCM_PERM(obj) = 0;

	/* insert the object at the head of the used list */
	QSE_SCM_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;

#if 0
	qse_dprint1 (QSE_T("mem->count: %u\n"), mem->count);
#endif

	return obj;
}

static void dispose (
	qse_scm_mem_t* mem, qse_scm_obj_t* prev, qse_scm_obj_t* obj)
{
	QSE_ASSERT (obj != QSE_NULL);
	QSE_ASSERT (mem->count > 0);

	/* TODO: push the object to the free list for more 
	 *       efficient memory management */

	if (prev == QSE_NULL) 
		mem->used[QSE_SCM_TYPE(obj)] = QSE_SCM_LINK(obj);
	else QSE_SCM_LINK(prev) = QSE_SCM_LINK(obj);

	mem->count--;
#if 0
	qse_dprint1 (QSE_T("mem->count: %u\n"), mem->count);
#endif

	QSE_SCM_FREE (mem->scm, obj);	
}

static void dispose_all (qse_scm_mem_t* mem)
{
	qse_scm_obj_t* obj, * next;
	qse_size_t i;

	for (i = 0; i < QSE_SCM_TYPE_COUNT; i++) 
	{
		obj = mem->used[i];

		while (obj != QSE_NULL) 
		{
			next = QSE_SCM_LINK(obj);
			dispose (mem, QSE_NULL, obj);
			obj = next;
		}
	}
}

static void mark_obj (qse_scm_mem_t* mem, qse_scm_obj_t* obj)
{
	QSE_ASSERT (obj != QSE_NULL);

	/* TODO: can it be non-recursive? */
	if (QSE_SCM_MARK(obj) != 0) return;

	QSE_SCM_MARK(obj) = 1;

	switch (QSE_SCM_TYPE(obj))
	{
		case QSE_SCM_OBJ_CONS:
			mark_obj (mem, QSE_SCM_CAR(obj));
			mark_obj (mem, QSE_SCM_CDR(obj));
			break;
	
		case QSE_SCM_OBJ_FUNC:
			mark_obj (mem, QSE_SCM_FFORMAL(obj));
			mark_obj (mem, QSE_SCM_FBODY(obj));
			break;

		case QSE_SCM_OBJ_MACRO:
			mark_obj (mem, QSE_SCM_MFORMAL(obj));
			mark_obj (mem, QSE_SCM_MBODY(obj));
			break;
	}
}

static void mark_objs_in_use (qse_scm_mem_t* mem)
{
	qse_scm_frame_t* frame;
	qse_scm_assoc_t* assoc;
	qse_scm_tlink_t* tlink;
	/*qse_scm_arr_t* arr;*/
	qse_size_t       i;

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
			mark_obj (mem, assoc->name);

			if (assoc->value != QSE_NULL) 
				mark_obj (mem, assoc->value);
			if (assoc->func != QSE_NULL) 
				mark_obj (mem, assoc->func);

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
			mark_obj (mem, assoc->name);

			if (assoc->value != QSE_NULL) 
				mark_obj (mem, assoc->value);
			if (assoc->func != QSE_NULL) 
				mark_obj (mem, assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*qse_dprint0 (QSE_T("marking the read object\n"));*/
	if (mem->r.obj) mark_obj (mem, mem->r.obj);
	if (mem->r.tmp) mark_obj (mem, mem->r.tmp);
	if (mem->r.stack) mark_obj (mem, mem->r.stack);

	/* qse_dprint0 (QSE_T("marking the temporary objects\n"));*/
	for (tlink = mem->tlink; tlink != QSE_NULL; tlink = tlink->link)
	{
		mark_obj (mem, tlink->obj);
	}

#if 0
	qse_dprint0 (QSE_T("marking builtin objects\n"));
#endif
	/* mark common objects */
	if (mem->t) mark_obj (mem, mem->t);
	if (mem->nil) mark_obj (mem, mem->nil);
	if (mem->quote) mark_obj (mem, mem->quote);
	if (mem->lambda) mark_obj (mem, mem->lambda);
	if (mem->macro) mark_obj (mem, mem->macro);

	for (i = 0; i < QSE_COUNTOF(mem->num); i++) 
	{
		if (mem->num[i]) mark_obj (mem, mem->num[i]);
	}
}

//#include <qse/cmn/stdio.h>
static void sweep_unmarked_objs (qse_scm_mem_t* mem)
{
	qse_scm_obj_t* obj, * prev, * next;
	qse_size_t i;

	/* scan all the allocated objects and get rid of unused objects */
	for (i = 0; i < QSE_SCM_TYPE_COUNT; i++) 
	{
		prev = QSE_NULL;
		obj = mem->used[i];

#if 0
		qse_dprint1 (QSE_T("sweeping objects of type: %u\n"), i);
#endif
		while (obj != QSE_NULL) 
		{
			next = QSE_SCM_LINK(obj);

			if (QSE_SCM_MARK(obj) == 0 && 
			    QSE_SCM_PERM(obj) == 0) 
			{
				/* dispose of unused objects */
/*
if (i == QSE_SCM_OBJ_INT)
qse_printf (QSE_T("disposing....%d [%d]\n"), i, (int)QSE_SCM_IVAL(obj));
if (i == QSE_SCM_OBJ_REAL)
qse_printf (QSE_T("disposing....%d [%Lf]\n"), i, (double)QSE_SCM_RVAL(obj));
else if (i == QSE_SCM_OBJ_SYM)
qse_printf (QSE_T("disposing....%d [%s]\n"), i, QSE_SCM_SYMPTR(obj));
else if (i == QSE_SCM_OBJ_STR)
qse_printf (QSE_T("disposing....%d [%s]\n"), i, QSE_SCM_STRPTR(obj));
else
qse_printf (QSE_T("disposing....%d\n"), i);
*/
				dispose (mem, prev, obj);
			}
			else 
			{
				/* unmark the object in use */
				QSE_SCM_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

static QSE_INLINE_ALWAYS void collect_garbage (qse_scm_mem_t* mem)
{
	mark_objs_in_use (mem);
	sweep_unmarked_objs (mem);
}

void qse_scm_gc (qse_scm_t* scm)
{
	collect_garbage (&scm->mem);
}

qse_scm_obj_t* qse_scm_makenil (qse_scm_mem_t* mem)
{
	if (mem->nil != QSE_NULL) return mem->nil;
	mem->nil = allocate (
		mem, QSE_SCM_OBJ_NIL, QSE_SIZEOF(qse_scm_obj_nil_t));
	return mem->nil;
}

qse_scm_obj_t* qse_scm_maketrue (qse_scm_mem_t* mem)
{
	if (mem->t != QSE_NULL) return mem->t;
	mem->t = allocate (
		mem, QSE_SCM_OBJ_TRUE, QSE_SIZEOF(qse_scm_obj_true_t));
	return mem->t;
}

static qse_scm_obj_t* makeint (qse_scm_mem_t* mem, qse_long_t value)
{
	qse_scm_obj_t* obj;

	obj = allocate (mem, 
		QSE_SCM_OBJ_INT, QSE_SIZEOF(qse_scm_obj_int_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_SCM_IVAL(obj) = value;

	return obj;
}

qse_scm_obj_t* qse_scm_makeint (qse_scm_mem_t* mem, qse_long_t value)
{
	if (value >= 0 && value < QSE_COUNTOF(mem->num)) return mem->num[value];
	return makeint (mem, value);
}

qse_scm_obj_t* qse_scm_makereal (qse_scm_mem_t* mem, qse_real_t value)
{
	qse_scm_obj_t* obj;

	obj = allocate (mem, 
		QSE_SCM_OBJ_REAL, QSE_SIZEOF(qse_scm_obj_real_t));
	if (obj == QSE_NULL) return QSE_NULL;
	
	QSE_SCM_RVAL(obj) = value;

	return obj;
}

qse_scm_obj_t* qse_scm_makesym (
	qse_scm_mem_t* mem, const qse_char_t* str, qse_size_t len)
{
	qse_scm_obj_t* obj;

/* TODO: use rbt or htb ... */

	/* look for a sysmbol with the given name */
	obj = mem->used[QSE_SCM_OBJ_SYM];
	while (obj != QSE_NULL) 
	{
		/* if there is a symbol with the same name, it is just used. */
		if (qse_strxncmp (
			QSE_SCM_SYMPTR(obj), 
			QSE_SCM_SYMLEN(obj), 
			str, len) == 0) return obj;
		obj = QSE_SCM_LINK(obj);
	}

	/* no such symbol found. create a new one */
	obj = allocate (mem, QSE_SCM_OBJ_SYM,
		QSE_SIZEOF(qse_scm_obj_sym_t)+(len + 1)*QSE_SIZEOF(qse_char_t));
	if (obj == QSE_NULL) return QSE_NULL;

	/* fill in the symbol buffer */
	qse_strncpy (QSE_SCM_SYMPTR(obj), str, len);

	return obj;
}

qse_scm_obj_t* qse_scm_makestr (
	qse_scm_mem_t* mem, const qse_char_t* str, qse_size_t len)
{
	qse_scm_obj_t* obj;

	/* allocate memory for the string */
	obj = allocate (mem, QSE_SCM_OBJ_STR,
		QSE_SIZEOF(qse_scm_obj_str_t)+(len + 1)*QSE_SIZEOF(qse_char_t));
	if (obj == QSE_NULL) return QSE_NULL;

	/* fill in the string buffer */
	qse_strncpy (QSE_SCM_STRPTR(obj), str, len);

	return obj;
}

qse_scm_obj_t* qse_scm_makecons (
	qse_scm_mem_t* mem, qse_scm_obj_t* car, qse_scm_obj_t* cdr)
{
	qse_scm_obj_t* obj;

	obj = allocate (mem,
		QSE_SCM_OBJ_CONS, QSE_SIZEOF(qse_scm_obj_cons_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_SCM_CAR(obj) = car;
	QSE_SCM_CDR(obj) = cdr;

	return obj;
}

qse_scm_obj_t* qse_scm_makefunc (
	qse_scm_mem_t* mem, qse_scm_obj_t* formal, qse_scm_obj_t* body)
{
	qse_scm_obj_t* obj;

	obj = allocate (mem,
		QSE_SCM_OBJ_FUNC, QSE_SIZEOF(qse_scm_obj_func_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_SCM_FFORMAL(obj) = formal;
	QSE_SCM_FBODY(obj)   = body;

	return obj;
}

qse_scm_obj_t* qse_scm_makemacro (
	qse_scm_mem_t* mem, qse_scm_obj_t* formal, qse_scm_obj_t* body)
{
	qse_scm_obj_t* obj;

	obj = allocate (mem, 
		QSE_SCM_OBJ_MACRO, QSE_SIZEOF(qse_scm_obj_macro_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_SCM_MFORMAL(obj) = formal;
	QSE_SCM_MBODY(obj)   = body;

	return obj;
}

qse_scm_obj_t* qse_scm_makeprim (qse_scm_mem_t* mem, 
	qse_scm_prim_t impl, qse_size_t min_args, qse_size_t max_args)
{
	qse_scm_obj_t* obj;

	obj = allocate (
		mem, QSE_SCM_OBJ_PRIM, QSE_SIZEOF(qse_scm_obj_prim_t));
	if (obj == QSE_NULL) return QSE_NULL;

	QSE_SCM_PIMPL(obj) = impl;
	QSE_SCM_PMINARGS(obj) = min_args;
	QSE_SCM_PMAXARGS(obj) = max_args;
	return obj;
}

qse_scm_assoc_t* qse_scm_lookup (qse_scm_mem_t* mem, qse_scm_obj_t* name)
{
	qse_scm_frame_t* frame;
	qse_scm_assoc_t* assoc;

	QSE_ASSERT (QSE_SCM_TYPE(name) == QSE_SCM_OBJ_SYM);

	frame = mem->frame;

	while (frame != QSE_NULL) 
	{
		assoc = qse_scm_lookupinframe (mem->scm, frame, name);
		if (assoc != QSE_NULL) return assoc;
		frame = frame->link;
	}

	return QSE_NULL;
}

qse_scm_assoc_t* qse_scm_setvalue (
	qse_scm_mem_t* mem, qse_scm_obj_t* name, qse_scm_obj_t* value)
{
	qse_scm_assoc_t* assoc;

	assoc = qse_scm_lookup (mem, name);
	if (assoc == QSE_NULL)
	{
		assoc = qse_scm_insvalueintoframe (
			mem->scm, mem->root_frame, name, value);
		if (assoc == QSE_NULL) return QSE_NULL;
	}
	else assoc->value = value;

	return assoc;
}

qse_scm_assoc_t* qse_scm_setfunc (
	qse_scm_mem_t* mem, qse_scm_obj_t* name, qse_scm_obj_t* func)
{
	qse_scm_assoc_t* assoc;

	assoc = qse_scm_lookup (mem, name);
	if (assoc == QSE_NULL) 
	{
		assoc = qse_scm_insfuncintoframe (
			mem->scm, mem->root_frame, name, func);
		if (assoc == QSE_NULL) return QSE_NULL;
	}
	else assoc->func = func;

	return assoc;
}

qse_size_t qse_scm_conslen (qse_scm_mem_t* mem, qse_scm_obj_t* obj)
{
	qse_size_t count;

	QSE_ASSERT (
		obj == mem->nil || QSE_SCM_TYPE(obj) == QSE_SCM_OBJ_CONS);

	count = 0;
	/*while (obj != mem->nil) */
	while (QSE_SCM_TYPE(obj) == QSE_SCM_OBJ_CONS) 
	{
		count++;
		obj = QSE_SCM_CDR(obj);
	}

	return count;
}



