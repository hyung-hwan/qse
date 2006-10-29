/*
 * $Id: mem.c,v 1.18 2006-10-29 13:40:33 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_mem_t* ase_lsp_openmem (
	ase_lsp_t* lsp, ase_size_t ubound, ase_size_t ubound_inc)
{
	ase_lsp_mem_t* mem;
	ase_size_t i;

	/* allocate memory */
	mem = (ase_lsp_mem_t*) ASE_LSP_MALLOC (lsp, ase_sizeof(ase_lsp_mem_t));	
	if (mem == ASE_NULL) return ASE_NULL;

	ASE_LSP_MEMSET (lsp, mem, 0, ase_sizeof(ase_lsp_mem_t));
	mem->lsp = lsp;

	/* create a new root environment frame */
	mem->frame = ase_lsp_newframe (lsp);
	if (mem->frame == ASE_NULL) 
	{
		ASE_LSP_FREE (lsp, mem);
		return ASE_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = ASE_NULL;

	/* create an array to hold temporary objects */
	/*
	mem->temp_arr = ase_lsp_arr_new (512);
	if (mem->temp_arr == ASE_NULL) 
	{
		ase_lsp_freeframe (lsp, mem->frame);
		ASE_LSP_FREE (lsp, mem);
		return ASE_NULL;
	}
	*/

	/* initialize object allocation list */
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < ASE_LSP_TYPE_COUNT; i++) 
	{
		mem->used[i] = ASE_NULL;
		mem->free[i] = ASE_NULL;
	}
	mem->locked = ASE_NULL;

	/* when "ubound" is too small, the garbage collection can
	 * be performed while making the common objects. */
	mem->nil    = ASE_NULL;
	mem->t      = ASE_NULL;
	mem->quote  = ASE_NULL;
	mem->lambda = ASE_NULL;
	mem->macro  = ASE_NULL;

	/* initialize common object pointers */
	mem->nil     = ase_lsp_makenil    (mem);
	mem->t       = ase_lsp_maketrue   (mem);
	mem->quote   = ase_lsp_makesymobj (mem, ASE_T("quote"),  5);
	mem->lambda  = ase_lsp_makesymobj (mem, ASE_T("lambda"), 6);
	mem->macro   = ase_lsp_makesymobj (mem, ASE_T("macro"),  5);

	if (mem->nil    == ASE_NULL ||
	    mem->t      == ASE_NULL ||
	    mem->quote  == ASE_NULL ||
	    mem->lambda == ASE_NULL ||
	    mem->macro  == ASE_NULL) 
	{
		ase_lsp_dispose_all (mem);
		/*ase_lsp_arr_free (mem->temp_arr);*/
		ase_lsp_freeframe (lsp, mem->frame);
		ASE_LSP_FREE (lsp, mem);
		return ASE_NULL;
	}

	return mem;
}

void ase_lsp_closemem (ase_lsp_mem_t* mem)
{
	/* dispose of the allocated objects */
	ase_lsp_dispose_all (mem);

	/* dispose of the temporary object arrs */
	/*ase_lsp_arr_free (mem->temp_arr);*/

	/* dispose of environment frames */
	ase_lsp_freeframe (mem->lsp, mem->frame);

	/* free the memory */
	ASE_LSP_FREE (mem->lsp, mem);
}

static int __add_prim (
	ase_lsp_mem_t* mem, const ase_char_t* name, ase_size_t name_len, 
	ase_lsp_prim_t pimpl, ase_size_t min_args, ase_size_t max_args)
{
	ase_lsp_obj_t* n, * p;
	
	n = ase_lsp_makesymobj (mem, name, name_len);
	if (n == ASE_NULL) return -1;

	ase_lsp_lockobj (mem->lsp, n);

	p = ase_lsp_makeprim (mem, pimpl, min_args, max_args);
	if (p == ASE_NULL) return -1;

	ase_lsp_unlockobj (mem->lsp, n);

	if (ase_lsp_setfunc(mem, n, p) == ASE_NULL) return -1;

	return 0;
}

ase_lsp_obj_t* ase_lsp_alloc (ase_lsp_mem_t* mem, int type, ase_size_t size)
{
	ase_lsp_obj_t* obj;
	
	if (mem->count >= mem->ubound) ase_lsp_collectgarbage (mem);
	if (mem->count >= mem->ubound) {
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return ASE_NULL;
	}

	obj = (ase_lsp_obj_t*) ASE_LSP_MALLOC (mem->lsp, size);
	if (obj == ASE_NULL) 
	{
		ase_lsp_collectgarbage (mem);

		obj = (ase_lsp_obj_t*) ASE_LSP_MALLOC (mem->lsp, size);
		if (obj == ASE_NULL) return ASE_NULL;
	}

	ASE_LSP_TYPE(obj) = type;
	ASE_LSP_SIZE(obj) = size;
	ASE_LSP_MARK(obj) = 0;
	ASE_LSP_LOCK(obj) = 0;

	/* insert the object at the head of the used list */
	ASE_LSP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;

#if 0
	ase_dprint1 (ASE_T("mem->count: %u\n"), mem->count);
#endif

	return obj;
}

void ase_lsp_dispose (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* prev, ase_lsp_obj_t* obj)
{
	ASE_LSP_ASSERT (mem->lsp, obj != ASE_NULL);
	ASE_LSP_ASSERT (mem->lsp, mem->count > 0);

	// TODO: push the object to the free list for more 
	//       efficient memory management

	if (prev == ASE_NULL) 
		mem->used[ASE_LSP_TYPE(obj)] = ASE_LSP_LINK(obj);
	else ASE_LSP_LINK(prev) = ASE_LSP_LINK(obj);

	mem->count--;
#if 0
	ase_dprint1 (ASE_T("mem->count: %u\n"), mem->count);
#endif

	ASE_LSP_FREE (mem->lsp, obj);	
}

void ase_lsp_dispose_all (ase_lsp_mem_t* mem)
{
	ase_lsp_obj_t* obj, * next;
	ase_size_t i;

	for (i = 0; i < ASE_LSP_TYPE_COUNT; i++) 
	{
		obj = mem->used[i];

		while (obj != ASE_NULL) 
		{
			next = ASE_LSP_LINK(obj);
			ase_lsp_dispose (mem, ASE_NULL, obj);
			obj = next;
		}
	}
}

static void __mark_obj (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	ASE_LSP_ASSERT (lsp, obj != ASE_NULL);

	// TODO:....
	// can it be recursive?
	if (ASE_LSP_MARK(obj) != 0) return;

	ASE_LSP_MARK(obj) = 1;

	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
	{
		__mark_obj (lsp, ASE_LSP_CAR(obj));
		__mark_obj (lsp, ASE_LSP_CDR(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_FUNC) 
	{
		__mark_obj (lsp, ASE_LSP_FFORMAL(obj));
		__mark_obj (lsp, ASE_LSP_FBODY(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_MACRO) 
	{
		__mark_obj (lsp, ASE_LSP_MFORMAL(obj));
		__mark_obj (lsp, ASE_LSP_MBODY(obj));
	}
}

/*
 * ase_lsp_lockobj and ase_lsp_unlockallobjs are just called by ase_lsp_read.
 */
void ase_lsp_lockobj (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	ASE_LSP_ASSERT (lsp, obj != ASE_NULL);
	ASE_LSP_LOCK(obj) = 1;
	//ASE_LSP_MARK(obj) = 1;
}

void ase_lsp_unlockobj (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	ASE_LSP_ASSERT (lsp, obj != ASE_NULL);
	ASE_LSP_LOCK(obj) = 0;
}

void ase_lsp_unlockallobjs (ase_lsp_t* lsp, ase_lsp_obj_t* obj)
{
	ASE_LSP_ASSERT (lsp, obj != ASE_NULL);

	ASE_LSP_LOCK(obj) = 0;

	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
	{
		ase_lsp_unlockallobjs (lsp, ASE_LSP_CAR(obj));
		ase_lsp_unlockallobjs (lsp, ASE_LSP_CDR(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_FUNC) 
	{
		ase_lsp_unlockallobjs (lsp, ASE_LSP_FFORMAL(obj));
		ase_lsp_unlockallobjs (lsp, ASE_LSP_FBODY(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_MACRO) 
	{
		ase_lsp_unlockallobjs (lsp, ASE_LSP_MFORMAL(obj));
		ase_lsp_unlockallobjs (lsp, ASE_LSP_MBODY(obj));
	}
}

static void ase_lsp_markobjsinuse (ase_lsp_mem_t* mem)
{
	ase_lsp_frame_t* frame;
	ase_lsp_assoc_t* assoc;
	/*ase_lsp_arr_t* arr;*/
	/*ase_size_t       i;*/

#if 0
	ase_dprint0 (ASE_T("marking environment frames\n"));
#endif
	/* mark objects in the environment frames */
	frame = mem->frame;
	while (frame != ASE_NULL) 
	{
		assoc = frame->assoc;
		while (assoc != ASE_NULL) 
		{
			__mark_obj (mem->lsp, assoc->name);

			if (assoc->value != ASE_NULL) 
				__mark_obj (mem->lsp, assoc->value);
			if (assoc->func != ASE_NULL) 
				__mark_obj (mem->lsp, assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

#if 0
	ase_dprint0 (ASE_T("marking interim frames\n"));
#endif

	// mark objects in the interim frames
	frame = mem->brooding_frame;
	while (frame != ASE_NULL) {

		assoc = frame->assoc;
		while (assoc != ASE_NULL) {
			__mark_obj (mem->lsp, assoc->name);

			if (assoc->value != ASE_NULL) 
				__mark_obj (mem->lsp, assoc->value);
			if (assoc->func != ASE_NULL) 
				__mark_obj (mem->lsp, assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*
	ase_dprint0 (ASE_T("marking the locked object\n"));
	if (mem->locked != ASE_NULL) __mark_obj (mem->lsp, mem->locked);
	*/

#if 0
	ase_dprint0 (ASE_T("marking termporary objects\n"));
#endif
	/*
	arr = mem->temp_arr;
	for (i = 0; i < arr->size; i++) 
	{
		__mark_obj (mem->lsp, arr->buffer[i]);
	}
	*/

#if 0
	ase_dprint0 (ASE_T("marking builtin objects\n"));
#endif
	// mark common objects
	if (mem->t      != ASE_NULL) __mark_obj (mem->lsp, mem->t);
	if (mem->nil    != ASE_NULL) __mark_obj (mem->lsp, mem->nil);
	if (mem->quote  != ASE_NULL) __mark_obj (mem->lsp, mem->quote);
	if (mem->lambda != ASE_NULL) __mark_obj (mem->lsp, mem->lambda);
	if (mem->macro  != ASE_NULL) __mark_obj (mem->lsp, mem->macro);
}

static void ase_lsp_sweepunmarkedobjs (ase_lsp_mem_t* mem)
{
	ase_lsp_obj_t* obj, * prev, * next;
	ase_size_t i;

	// scan all the allocated objects and get rid of unused objects
	for (i = 0; i < ASE_LSP_TYPE_COUNT; i++) {
	//for (i = ASE_LSP_TYPE_COUNT; i > 0; /*i--*/) {
		prev = ASE_NULL;
		obj = mem->used[i];
		//obj = mem->used[--i];

#if 0
		ase_dprint1 (ASE_T("sweeping objects of type: %u\n"), i);
#endif

		while (obj != ASE_NULL) {
			next = ASE_LSP_LINK(obj);

			if (ASE_LSP_LOCK(obj) == 0 && ASE_LSP_MARK(obj) == 0) {
				// dispose of unused objects
				ase_lsp_dispose (mem, prev, obj);
			}
			else {
				// unmark the object in use
				ASE_LSP_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

void ase_lsp_collectgarbage (ase_lsp_mem_t* mem)
{
	ase_lsp_markobjsinuse (mem);
	ase_lsp_sweepunmarkedobjs (mem);
}

ase_lsp_obj_t* ase_lsp_makenil (ase_lsp_mem_t* mem)
{
	if (mem->nil != ASE_NULL) return mem->nil;
	mem->nil = ase_lsp_alloc (
		mem, ASE_LSP_OBJ_NIL, ase_sizeof(ase_lsp_obj_nil_t));
	return mem->nil;
}

ase_lsp_obj_t* ase_lsp_maketrue (ase_lsp_mem_t* mem)
{
	if (mem->t != ASE_NULL) return mem->t;
	mem->t = ase_lsp_alloc (
		mem, ASE_LSP_OBJ_TRUE, ase_sizeof(ase_lsp_obj_true_t));
	return mem->t;
}

ase_lsp_obj_t* ase_lsp_makeintobj (ase_lsp_mem_t* mem, ase_long_t value)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, 
		ASE_LSP_OBJ_INT, ase_sizeof(ase_lsp_obj_int_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_IVALUE(obj) = value;

	return obj;
}

ase_lsp_obj_t* ase_lsp_makerealobj (ase_lsp_mem_t* mem, ase_real_t value)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, 
		ASE_LSP_OBJ_REAL, ase_sizeof(ase_lsp_obj_real_t));
	if (obj == ASE_NULL) return ASE_NULL;
	
	ASE_LSP_RVALUE(obj) = value;

	return obj;
}

ase_lsp_obj_t* ase_lsp_makesymobj (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len)
{
	ase_lsp_obj_t* obj;

	// look for a sysmbol with the given name
	obj = mem->used[ASE_LSP_OBJ_SYM];
	while (obj != ASE_NULL) 
	{
		// if there is a symbol with the same name, it is just used.
		if (ase_lsp_strxncmp (
			ASE_LSP_SYMPTR(obj), 
			ASE_LSP_SYMLEN(obj), str, len) == 0) return obj;
		obj = ASE_LSP_LINK(obj);
	}

	// no such symbol found. create a new one 
	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_SYM,
		ase_sizeof(ase_lsp_obj_sym_t)+(len + 1)*ase_sizeof(ase_char_t));
	if (obj == ASE_NULL) return ASE_NULL;

	// fill in the symbol buffer
	ase_lsp_strncpy (ASE_LSP_SYMPTR(obj), str, len);

	return obj;
}

ase_lsp_obj_t* ase_lsp_makestrobj (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len)
{
	ase_lsp_obj_t* obj;

	// allocate memory for the string
	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_STR,
		ase_sizeof(ase_lsp_obj_str_t)+(len + 1)*ase_sizeof(ase_char_t));
	if (obj == ASE_NULL) return ASE_NULL;

	// fill in the string buffer
	ase_lsp_strncpy (ASE_LSP_STRPTR(obj), str, len);

	return obj;
}

ase_lsp_obj_t* ase_lsp_makecons (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* car, ase_lsp_obj_t* cdr)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_CONS, ase_sizeof(ase_lsp_obj_cons_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_CAR(obj) = car;
	ASE_LSP_CDR(obj) = cdr;

	return obj;
}

ase_lsp_obj_t* ase_lsp_makefunc (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_FUNC, ase_sizeof(ase_lsp_obj_func_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_FFORMAL(obj) = formal;
	ASE_LSP_FBODY(obj)   = body;

	return obj;
}

ase_lsp_obj_t* ase_lsp_makemacro (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_MACRO, ase_sizeof(ase_lsp_obj_macro_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_MFORMAL(obj) = formal;
	ASE_LSP_MBODY(obj)   = body;

	return obj;
}

ase_lsp_obj_t* ase_lsp_makeprim (ase_lsp_mem_t* mem, 
	ase_lsp_prim_t impl, ase_size_t min_args, ase_size_t max_args)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (
		mem, ASE_LSP_OBJ_PRIM, ase_sizeof(ase_lsp_obj_prim_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_PIMPL(obj) = impl;
	ASE_LSP_PMINARGS(obj) = min_args;
	ASE_LSP_PMAXARGS(obj) = max_args;
	return obj;
}

ase_lsp_assoc_t* ase_lsp_lookup (ase_lsp_mem_t* mem, ase_lsp_obj_t* name)
{
	ase_lsp_frame_t* frame;
	ase_lsp_assoc_t* assoc;

	ASE_LSP_ASSERT (mem->lsp, ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYM);

	frame = mem->frame;

	while (frame != ASE_NULL) 
	{
		assoc = ase_lsp_lookupinframe (mem->lsp, frame, name);
		if (assoc != ASE_NULL) return assoc;
		frame = frame->link;
	}

	return ASE_NULL;
}

ase_lsp_assoc_t* ase_lsp_setvalue (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* value)
{
	ase_lsp_assoc_t* assoc;

	assoc = ase_lsp_lookup (mem, name);
	if (assoc == ASE_NULL)
	{
		assoc = ase_lsp_insertvalueintoframe (
			mem->lsp, mem->root_frame, name, value);
		if (assoc == ASE_NULL) return ASE_NULL;
	}
	else assoc->value = value;

	return assoc;
}

ase_lsp_assoc_t* ase_lsp_setfunc (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	assoc = ase_lsp_lookup (mem, name);
	if (assoc == ASE_NULL) 
	{
		assoc = ase_lsp_insertfuncintoframe (
			mem->lsp, mem->root_frame, name, func);
		if (assoc == ASE_NULL) return ASE_NULL;
	}
	else assoc->func = func;

	return assoc;
}

ase_size_t ase_lsp_conslen (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj)
{
	ase_size_t count;

	ASE_LSP_ASSERT (mem->lsp, 
		obj == mem->nil || ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS);

	count = 0;
	/*while (obj != mem->nil) {*/
	while (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) 
	{
		count++;
		obj = ASE_LSP_CDR(obj);
	}

	return count;
}



