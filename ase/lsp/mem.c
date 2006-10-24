/*
 * $Id: mem.c,v 1.12 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_mem_t* ase_lsp_mem_new (ase_size_t ubound, ase_size_t ubound_inc)
{
	ase_lsp_mem_t* mem;
	ase_size_t i;

	// allocate memory
	mem = (ase_lsp_mem_t*) ase_malloc (ase_sizeof(ase_lsp_mem_t));	
	if (mem == ASE_NULL) return ASE_NULL;

	// create a new root environment frame
	mem->frame = ase_lsp_frame_new ();
	if (mem->frame == ASE_NULL) {
		ase_free (mem);
		return ASE_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = ASE_NULL;

	// create an array to hold temporary objects
	mem->temp_array = ase_lsp_array_new (512);
	if (mem->temp_array == ASE_NULL) {
		ase_lsp_frame_free (mem->frame);
		ase_free (mem);
		return ASE_NULL;
	}

	// initialize object allocation list
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < ASE_LSP_TYPE_COUNT; i++) {
		mem->used[i] = ASE_NULL;
		mem->free[i] = ASE_NULL;
	}
	mem->locked = ASE_NULL;

	// when "ubound" is too small, the garbage collection can
	// be performed while making the common objects.
	mem->nil    = ASE_NULL;
	mem->t      = ASE_NULL;
	mem->quote  = ASE_NULL;
	mem->lambda = ASE_NULL;
	mem->macro  = ASE_NULL;

	// initialize common object pointers
	mem->nil     = ase_lsp_make_nil    (mem);
	mem->t       = ase_lsp_make_true   (mem);
	mem->quote   = ase_lsp_make_symbol (mem, ASE_T("quote"));
	mem->lambda  = ase_lsp_make_symbol (mem, ASE_T("lambda"));
	mem->macro   = ase_lsp_make_symbol (mem, ASE_T("macro"));

	if (mem->nil    == ASE_NULL ||
	    mem->t      == ASE_NULL ||
	    mem->quote  == ASE_NULL ||
	    mem->lambda == ASE_NULL ||
	    mem->macro  == ASE_NULL) {
		ase_lsp_dispose_all (mem);
		ase_lsp_array_free (mem->temp_array);
		ase_lsp_frame_free (mem->frame);
		ase_free (mem);
		return ASE_NULL;
	}

	return mem;
}

void ase_lsp_mem_free (ase_lsp_mem_t* mem)
{
	ase_assert (mem != ASE_NULL);

	// dispose of the allocated objects
	ase_lsp_dispose_all (mem);

	// dispose of the temporary object arrays
	ase_lsp_array_free (mem->temp_array);

	// dispose of environment frames
	ase_lsp_frame_free (mem->frame);

	// free the memory
	ase_free (mem);
}

static int __add_prim (ase_lsp_mem_t* mem, 
	const ase_char_t* name, ase_size_t len, ase_lsp_prim_t prim)
{
	ase_lsp_obj_t* n, * p;
	
	n = ase_lsp_make_symbolx (mem, name, len);
	if (n == ASE_NULL) return -1;

	ase_lsp_lock (n);

	p = ase_lsp_make_prim (mem, prim);
	if (p == ASE_NULL) return -1;

	ase_lsp_unlock (n);

	if (ase_lsp_set_func(mem, n, p) == ASE_NULL) return -1;

	return 0;
}


int ase_lsp_add_builtin_prims (ase_lsp_mem_t* mem)
{

#define ADD_PRIM(mem,name,len,prim) \
	if (__add_prim(mem,name,len,prim) == -1) return -1;

	ADD_PRIM (mem, ASE_T("abort"), 5, ase_lsp_prim_abort);
	ADD_PRIM (mem, ASE_T("eval"),  4, ase_lsp_prim_eval);
	ADD_PRIM (mem, ASE_T("prog1"), 5, ase_lsp_prim_prog1);
	ADD_PRIM (mem, ASE_T("progn"), 5, ase_lsp_prim_progn);
	ADD_PRIM (mem, ASE_T("gc"),    2, ase_lsp_prim_gc);

	ADD_PRIM (mem, ASE_T("cond"),  4, ase_lsp_prim_cond);
	ADD_PRIM (mem, ASE_T("if"),    2, ase_lsp_prim_if);
	ADD_PRIM (mem, ASE_T("while"), 5, ase_lsp_prim_while);

	ADD_PRIM (mem, ASE_T("car"),   3, ase_lsp_prim_car);
	ADD_PRIM (mem, ASE_T("cdr"),   3, ase_lsp_prim_cdr);
	ADD_PRIM (mem, ASE_T("cons"),  4, ase_lsp_prim_cons);
	ADD_PRIM (mem, ASE_T("set"),   3, ase_lsp_prim_set);
	ADD_PRIM (mem, ASE_T("setq"),  4, ase_lsp_prim_setq);
	ADD_PRIM (mem, ASE_T("quote"), 5, ase_lsp_prim_quote);
	ADD_PRIM (mem, ASE_T("defun"), 5, ase_lsp_prim_defun);
	ADD_PRIM (mem, ASE_T("demac"), 5, ase_lsp_prim_demac);
	ADD_PRIM (mem, ASE_T("let"),   3, ase_lsp_prim_let);
	ADD_PRIM (mem, ASE_T("let*"),  4, ase_lsp_prim_letx);

	ADD_PRIM (mem, ASE_T(">"),     1, ase_lsp_prim_gt);
	ADD_PRIM (mem, ASE_T("<"),     1, ase_lsp_prim_lt);

	ADD_PRIM (mem, ASE_T("+"),     1, ase_lsp_prim_plus);
	ADD_PRIM (mem, ASE_T("-"),     1, ase_lsp_prim_minus);

	return 0;
}


ase_lsp_obj_t* ase_lsp_alloc (ase_lsp_mem_t* mem, int type, ase_size_t size)
{
	ase_lsp_obj_t* obj;
	
	if (mem->count >= mem->ubound) ase_lsp_garbage_collect (mem);
	if (mem->count >= mem->ubound) {
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return ASE_NULL;
	}

	obj = (ase_lsp_obj_t*) ase_malloc (size);
	if (obj == ASE_NULL) {
		ase_lsp_garbage_collect (mem);

		obj = (ase_lsp_obj_t*) ase_malloc (size);
		if (obj == ASE_NULL) return ASE_NULL;
	}

	ASE_LSP_TYPE(obj) = type;
	ASE_LSP_SIZE(obj) = size;
	ASE_LSP_MARK(obj) = 0;
	ASE_LSP_LOCK(obj) = 0;

	// insert the object at the head of the used list
	ASE_LSP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;

#if 0
	ase_dprint1 (ASE_T("mem->count: %u\n"), mem->count);
#endif

	return obj;
}

void ase_lsp_dispose (ase_lsp_mem_t* mem, ase_lsp_obj_t* prev, ase_lsp_obj_t* obj)
{
	ase_assert (mem != ASE_NULL);
	ase_assert (obj != ASE_NULL);
	ase_assert (mem->count > 0);

	// TODO: push the object to the free list for more 
	//       efficient memory management

	if (prev == ASE_NULL) 
		mem->used[ASE_LSP_TYPE(obj)] = ASE_LSP_LINK(obj);
	else ASE_LSP_LINK(prev) = ASE_LSP_LINK(obj);

	mem->count--;
#if 0
	ase_dprint1 (ASE_T("mem->count: %u\n"), mem->count);
#endif

	ase_free (obj);	
}

void ase_lsp_dispose_all (ase_lsp_mem_t* mem)
{
	ase_lsp_obj_t* obj, * next;
	ase_size_t i;

	for (i = 0; i < ASE_LSP_TYPE_COUNT; i++) {
		obj = mem->used[i];

		while (obj != ASE_NULL) {
			next = ASE_LSP_LINK(obj);
			ase_lsp_dispose (mem, ASE_NULL, obj);
			obj = next;
		}
	}
}

static void ase_lsp_mark_obj (ase_lsp_obj_t* obj)
{
	ase_assert (obj != ASE_NULL);

	// TODO:....
	// can it be recursive?
	if (ASE_LSP_MARK(obj) != 0) return;

	ASE_LSP_MARK(obj) = 1;

	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) {
		ase_lsp_mark_obj (ASE_LSP_CAR(obj));
		ase_lsp_mark_obj (ASE_LSP_CDR(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_FUNC) {
		ase_lsp_mark_obj (ASE_LSP_FFORMAL(obj));
		ase_lsp_mark_obj (ASE_LSP_FBODY(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_MACRO) {
		ase_lsp_mark_obj (ASE_LSP_MFORMAL(obj));
		ase_lsp_mark_obj (ASE_LSP_MBODY(obj));
	}
}

/*
 * ase_lsp_lock and ase_lsp_unlock_all are just called by ase_lsp_read.
 */
void ase_lsp_lock (ase_lsp_obj_t* obj)
{
	ase_assert (obj != ASE_NULL);
	ASE_LSP_LOCK(obj) = 1;
	//ASE_LSP_MARK(obj) = 1;
}

void ase_lsp_unlock (ase_lsp_obj_t* obj)
{
	ase_assert (obj != ASE_NULL);
	ASE_LSP_LOCK(obj) = 0;
}

void ase_lsp_unlock_all (ase_lsp_obj_t* obj)
{
	ase_assert (obj != ASE_NULL);

	ASE_LSP_LOCK(obj) = 0;

	if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) {
		ase_lsp_unlock_all (ASE_LSP_CAR(obj));
		ase_lsp_unlock_all (ASE_LSP_CDR(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_FUNC) {
		ase_lsp_unlock_all (ASE_LSP_FFORMAL(obj));
		ase_lsp_unlock_all (ASE_LSP_FBODY(obj));
	}
	else if (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_MACRO) {
		ase_lsp_unlock_all (ASE_LSP_MFORMAL(obj));
		ase_lsp_unlock_all (ASE_LSP_MBODY(obj));
	}
}

static void ase_lsp_mark (ase_lsp_mem_t* mem)
{
	ase_lsp_frame_t* frame;
	ase_lsp_assoc_t* assoc;
	ase_lsp_array_t* array;
	ase_size_t       i;

#if 0
	ase_dprint0 (ASE_T("marking environment frames\n"));
#endif
	// mark objects in the environment frames
	frame = mem->frame;
	while (frame != ASE_NULL) {
		assoc = frame->assoc;
		while (assoc != ASE_NULL) {
			ase_lsp_mark_obj (assoc->name);

			if (assoc->value != ASE_NULL) 
				ase_lsp_mark_obj (assoc->value);
			if (assoc->func != ASE_NULL) 
				ase_lsp_mark_obj (assoc->func);

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
			ase_lsp_mark_obj (assoc->name);

			if (assoc->value != ASE_NULL) 
				ase_lsp_mark_obj (assoc->value);
			if (assoc->func != ASE_NULL) 
				ase_lsp_mark_obj (assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*
	ase_dprint0 (ASE_T("marking the locked object\n"));
	if (mem->locked != ASE_NULL) ase_lsp_mark_obj (mem->locked);
	*/

#if 0
	ase_dprint0 (ASE_T("marking termporary objects\n"));
#endif
	array = mem->temp_array;
	for (i = 0; i < array->size; i++) {
		ase_lsp_mark_obj (array->buffer[i]);
	}

#if 0
	ase_dprint0 (ASE_T("marking builtin objects\n"));
#endif
	// mark common objects
	if (mem->t      != ASE_NULL) ase_lsp_mark_obj (mem->t);
	if (mem->nil    != ASE_NULL) ase_lsp_mark_obj (mem->nil);
	if (mem->quote  != ASE_NULL) ase_lsp_mark_obj (mem->quote);
	if (mem->lambda != ASE_NULL) ase_lsp_mark_obj (mem->lambda);
	if (mem->macro  != ASE_NULL) ase_lsp_mark_obj (mem->macro);
}

static void ase_lsp_sweep (ase_lsp_mem_t* mem)
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

void ase_lsp_garbage_collect (ase_lsp_mem_t* mem)
{
	ase_lsp_mark (mem);
	ase_lsp_sweep (mem);
}

ase_lsp_obj_t* ase_lsp_make_nil (ase_lsp_mem_t* mem)
{
	if (mem->nil != ASE_NULL) return mem->nil;
	mem->nil = ase_lsp_alloc (mem, ASE_LSP_OBJ_NIL, ase_sizeof(ase_lsp_obj_nil_t));
	return mem->nil;
}

ase_lsp_obj_t* ase_lsp_make_true (ase_lsp_mem_t* mem)
{
	if (mem->t != ASE_NULL) return mem->t;
	mem->t = ase_lsp_alloc (mem, ASE_LSP_OBJ_TRUE, ase_sizeof(ase_lsp_obj_true_t));
	return mem->t;
}

ase_lsp_obj_t* ase_lsp_make_int (ase_lsp_mem_t* mem, ase_lsp_int_t value)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, 
		ASE_LSP_OBJ_INT, ase_sizeof(ase_lsp_obj_int_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_IVALUE(obj) = value;

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_real (ase_lsp_mem_t* mem, ase_lsp_real_t value)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, 
		ASE_LSP_OBJ_REAL, ase_sizeof(ase_lsp_obj_real_t));
	if (obj == ASE_NULL) return ASE_NULL;
	
	ASE_LSP_RVALUE(obj) = value;

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_symbol (ase_lsp_mem_t* mem, const ase_char_t* str)
{
	return ase_lsp_make_symbolx (mem, str, ase_strlen(str));
}

ase_lsp_obj_t* ase_lsp_make_symbolx (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len)
{
	ase_lsp_obj_t* obj;

	// look for a sysmbol with the given name
	obj = mem->used[ASE_LSP_OBJ_SYMBOL];
	while (obj != ASE_NULL) {
		// if there is a symbol with the same name, it is just used.
		if (ase_lsp_comp_symbol2 (obj, str, len) == 0) return obj;
		obj = ASE_LSP_LINK(obj);
	}

	// no such symbol found. create a new one 
	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_SYMBOL,
		ase_sizeof(ase_lsp_obj_symbol_t) + (len + 1) * ase_sizeof(ase_char_t));
	if (obj == ASE_NULL) return ASE_NULL;

	// fill in the symbol buffer
	ase_lsp_copy_string2 (ASE_LSP_SYMVALUE(obj), str, len);

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_string (ase_lsp_mem_t* mem, const ase_char_t* str)
{
	return ase_lsp_make_stringx (mem, str, ase_strlen(str));
}

ase_lsp_obj_t* ase_lsp_make_stringx (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len)
{
	ase_lsp_obj_t* obj;

	// allocate memory for the string
	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_STRING,
		ase_sizeof(ase_lsp_obj_string_t) + (len + 1) * ase_sizeof(ase_char_t));
	if (obj == ASE_NULL) return ASE_NULL;

	// fill in the string buffer
	ase_lsp_copy_string2 (ASE_LSP_STRVALUE(obj), str, len);

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_cons (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* car, ase_lsp_obj_t* cdr)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_CONS, ase_sizeof(ase_lsp_obj_cons_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_CAR(obj) = car;
	ASE_LSP_CDR(obj) = cdr;

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_func (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_FUNC, ase_sizeof(ase_lsp_obj_func_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_FFORMAL(obj) = formal;
	ASE_LSP_FBODY(obj)   = body;

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_macro (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_MACRO, ase_sizeof(ase_lsp_obj_macro_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_MFORMAL(obj) = formal;
	ASE_LSP_MBODY(obj)   = body;

	return obj;
}

ase_lsp_obj_t* ase_lsp_make_prim (ase_lsp_mem_t* mem, void* impl)
{
	ase_lsp_obj_t* obj;

	obj = ase_lsp_alloc (mem, ASE_LSP_OBJ_PRIM, ase_sizeof(ase_lsp_obj_prim_t));
	if (obj == ASE_NULL) return ASE_NULL;

	ASE_LSP_PRIM(obj) = impl;

	return obj;
}

ase_lsp_assoc_t* ase_lsp_lookup (ase_lsp_mem_t* mem, ase_lsp_obj_t* name)
{
	ase_lsp_frame_t* frame;
	ase_lsp_assoc_t* assoc;

	ase_assert (ASE_LSP_TYPE(name) == ASE_LSP_OBJ_SYMBOL);

	frame = mem->frame;

	while (frame != ASE_NULL) {
		assoc = ase_lsp_frame_lookup (frame, name);
		if (assoc != ASE_NULL) return assoc;
		frame = frame->link;
	}

	return ASE_NULL;
}

ase_lsp_assoc_t* ase_lsp_set_value (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* value)
{
	ase_lsp_assoc_t* assoc;

	assoc = ase_lsp_lookup (mem, name);
	if (assoc == ASE_NULL)  {
		assoc = ase_lsp_frame_insert_value (
			mem->root_frame, name, value);
		if (assoc == ASE_NULL) return ASE_NULL;
	}
	else assoc->value = value;

	return assoc;
}

ase_lsp_assoc_t* ase_lsp_set_func (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* func)
{
	ase_lsp_assoc_t* assoc;

	assoc = ase_lsp_lookup (mem, name);
	if (assoc == ASE_NULL)  {
		assoc = ase_lsp_frame_insert_func (mem->root_frame, name, func);
		if (assoc == ASE_NULL) return ASE_NULL;
	}
	else assoc->func = func;

	return assoc;
}

ase_size_t ase_lsp_cons_len (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj)
{
	ase_size_t count;

	ase_assert (obj == mem->nil || ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS);

	count = 0;
	//while (obj != mem->nil) {
	while (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) {
		count++;
		obj = ASE_LSP_CDR(obj);
	}

	return count;
}

int ase_lsp_probe_args (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj, ase_size_t* len)
{
	ase_size_t count = 0;

	while (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_CONS) {
		count++;
		obj = ASE_LSP_CDR(obj);
	}	

	if (obj != mem->nil) return -1;

	*len = count;
	return 0;
}

int ase_lsp_comp_symbol (ase_lsp_obj_t* obj, const ase_char_t* str)
{
	ase_char_t* p;
	ase_size_t index, length;

	ase_assert (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = ASE_LSP_SYMLEN(obj);

	p = ASE_LSP_SYMVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == ASE_T('\0'))? 0: -1;
}

int ase_lsp_comp_symbol2 (ase_lsp_obj_t* obj, const ase_char_t* str, ase_size_t len)
{
	ase_char_t* p;
	ase_size_t index, length;

	ase_assert (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = ASE_LSP_SYMLEN(obj);
	p = ASE_LSP_SYMVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

int ase_lsp_comp_string (ase_lsp_obj_t* obj, const ase_char_t* str)
{
	ase_char_t* p;
	ase_size_t index, length;

	ase_assert (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_STRING);
	
	index = 0;
	length = ASE_LSP_STRLEN(obj);

	p = ASE_LSP_STRVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == ASE_T('\0'))? 0: -1;
}

int ase_lsp_comp_string2 (ase_lsp_obj_t* obj, const ase_char_t* str, ase_size_t len)
{
	ase_char_t* p;
	ase_size_t index, length;

	ase_assert (ASE_LSP_TYPE(obj) == ASE_LSP_OBJ_STRING);
	
	index = 0;
	length = ASE_LSP_STRLEN(obj);
	p = ASE_LSP_STRVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

void ase_lsp_copy_string (ase_char_t* dst, const ase_char_t* str)
{
	// the buffer pointed by dst should be big enough to hold str
	while (*str != ASE_T('\0')) *dst++ = *str++;
	*dst = ASE_T('\0');
}

void ase_lsp_copy_string2 (ase_char_t* dst, const ase_char_t* str, ase_size_t len)
{
	// the buffer pointed by dst should be big enough to hold str
	while (len > 0) {
		*dst++ = *str++;
		len--;
	}
	*dst = ASE_T('\0');
}

