/*
 * $Id: mem.c,v 1.10 2006-10-23 14:44:43 bacon Exp $
 */

#include <sse/lsp/mem.h> 
#include <sse/lsp/prim.h>

#include <sse/bas/memory.h>
#include <sse/bas/string.h>
#include <sse/bas/assert.h>

sse_lsp_mem_t* sse_lsp_mem_new (sse_size_t ubound, sse_size_t ubound_inc)
{
	sse_lsp_mem_t* mem;
	sse_size_t i;

	// allocate memory
	mem = (sse_lsp_mem_t*) sse_malloc (sse_sizeof(sse_lsp_mem_t));	
	if (mem == SSE_NULL) return SSE_NULL;

	// create a new root environment frame
	mem->frame = sse_lsp_frame_new ();
	if (mem->frame == SSE_NULL) {
		sse_free (mem);
		return SSE_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = SSE_NULL;

	// create an array to hold temporary objects
	mem->temp_array = sse_lsp_array_new (512);
	if (mem->temp_array == SSE_NULL) {
		sse_lsp_frame_free (mem->frame);
		sse_free (mem);
		return SSE_NULL;
	}

	// initialize object allocation list
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < SSE_LSP_TYPE_COUNT; i++) {
		mem->used[i] = SSE_NULL;
		mem->free[i] = SSE_NULL;
	}
	mem->locked = SSE_NULL;

	// when "ubound" is too small, the garbage collection can
	// be performed while making the common objects.
	mem->nil    = SSE_NULL;
	mem->t      = SSE_NULL;
	mem->quote  = SSE_NULL;
	mem->lambda = SSE_NULL;
	mem->macro  = SSE_NULL;

	// initialize common object pointers
	mem->nil     = sse_lsp_make_nil    (mem);
	mem->t       = sse_lsp_make_true   (mem);
	mem->quote   = sse_lsp_make_symbol (mem, SSE_T("quote"));
	mem->lambda  = sse_lsp_make_symbol (mem, SSE_T("lambda"));
	mem->macro   = sse_lsp_make_symbol (mem, SSE_T("macro"));

	if (mem->nil    == SSE_NULL ||
	    mem->t      == SSE_NULL ||
	    mem->quote  == SSE_NULL ||
	    mem->lambda == SSE_NULL ||
	    mem->macro  == SSE_NULL) {
		sse_lsp_dispose_all (mem);
		sse_lsp_array_free (mem->temp_array);
		sse_lsp_frame_free (mem->frame);
		sse_free (mem);
		return SSE_NULL;
	}

	return mem;
}

void sse_lsp_mem_free (sse_lsp_mem_t* mem)
{
	sse_assert (mem != SSE_NULL);

	// dispose of the allocated objects
	sse_lsp_dispose_all (mem);

	// dispose of the temporary object arrays
	sse_lsp_array_free (mem->temp_array);

	// dispose of environment frames
	sse_lsp_frame_free (mem->frame);

	// free the memory
	sse_free (mem);
}

static int __add_prim (sse_lsp_mem_t* mem, 
	const sse_char_t* name, sse_size_t len, sse_lsp_prim_t prim)
{
	sse_lsp_obj_t* n, * p;
	
	n = sse_lsp_make_symbolx (mem, name, len);
	if (n == SSE_NULL) return -1;

	sse_lsp_lock (n);

	p = sse_lsp_make_prim (mem, prim);
	if (p == SSE_NULL) return -1;

	sse_lsp_unlock (n);

	if (sse_lsp_set_func(mem, n, p) == SSE_NULL) return -1;

	return 0;
}


int sse_lsp_add_builtin_prims (sse_lsp_mem_t* mem)
{

#define ADD_PRIM(mem,name,len,prim) \
	if (__add_prim(mem,name,len,prim) == -1) return -1;

	ADD_PRIM (mem, SSE_T("abort"), 5, sse_lsp_prim_abort);
	ADD_PRIM (mem, SSE_T("eval"),  4, sse_lsp_prim_eval);
	ADD_PRIM (mem, SSE_T("prog1"), 5, sse_lsp_prim_prog1);
	ADD_PRIM (mem, SSE_T("progn"), 5, sse_lsp_prim_progn);
	ADD_PRIM (mem, SSE_T("gc"),    2, sse_lsp_prim_gc);

	ADD_PRIM (mem, SSE_T("cond"),  4, sse_lsp_prim_cond);
	ADD_PRIM (mem, SSE_T("if"),    2, sse_lsp_prim_if);
	ADD_PRIM (mem, SSE_T("while"), 5, sse_lsp_prim_while);

	ADD_PRIM (mem, SSE_T("car"),   3, sse_lsp_prim_car);
	ADD_PRIM (mem, SSE_T("cdr"),   3, sse_lsp_prim_cdr);
	ADD_PRIM (mem, SSE_T("cons"),  4, sse_lsp_prim_cons);
	ADD_PRIM (mem, SSE_T("set"),   3, sse_lsp_prim_set);
	ADD_PRIM (mem, SSE_T("setq"),  4, sse_lsp_prim_setq);
	ADD_PRIM (mem, SSE_T("quote"), 5, sse_lsp_prim_quote);
	ADD_PRIM (mem, SSE_T("defun"), 5, sse_lsp_prim_defun);
	ADD_PRIM (mem, SSE_T("demac"), 5, sse_lsp_prim_demac);
	ADD_PRIM (mem, SSE_T("let"),   3, sse_lsp_prim_let);
	ADD_PRIM (mem, SSE_T("let*"),  4, sse_lsp_prim_letx);

	ADD_PRIM (mem, SSE_T(">"),     1, sse_lsp_prim_gt);
	ADD_PRIM (mem, SSE_T("<"),     1, sse_lsp_prim_lt);

	ADD_PRIM (mem, SSE_T("+"),     1, sse_lsp_prim_plus);
	ADD_PRIM (mem, SSE_T("-"),     1, sse_lsp_prim_minus);

	return 0;
}


sse_lsp_obj_t* sse_lsp_alloc (sse_lsp_mem_t* mem, int type, sse_size_t size)
{
	sse_lsp_obj_t* obj;
	
	if (mem->count >= mem->ubound) sse_lsp_garbage_collect (mem);
	if (mem->count >= mem->ubound) {
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return SSE_NULL;
	}

	obj = (sse_lsp_obj_t*) sse_malloc (size);
	if (obj == SSE_NULL) {
		sse_lsp_garbage_collect (mem);

		obj = (sse_lsp_obj_t*) sse_malloc (size);
		if (obj == SSE_NULL) return SSE_NULL;
	}

	SSE_LSP_TYPE(obj) = type;
	SSE_LSP_SIZE(obj) = size;
	SSE_LSP_MARK(obj) = 0;
	SSE_LSP_LOCK(obj) = 0;

	// insert the object at the head of the used list
	SSE_LSP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;

#if 0
	sse_dprint1 (SSE_T("mem->count: %u\n"), mem->count);
#endif

	return obj;
}

void sse_lsp_dispose (sse_lsp_mem_t* mem, sse_lsp_obj_t* prev, sse_lsp_obj_t* obj)
{
	sse_assert (mem != SSE_NULL);
	sse_assert (obj != SSE_NULL);
	sse_assert (mem->count > 0);

	// TODO: push the object to the free list for more 
	//       efficient memory management

	if (prev == SSE_NULL) 
		mem->used[SSE_LSP_TYPE(obj)] = SSE_LSP_LINK(obj);
	else SSE_LSP_LINK(prev) = SSE_LSP_LINK(obj);

	mem->count--;
#if 0
	sse_dprint1 (SSE_T("mem->count: %u\n"), mem->count);
#endif

	sse_free (obj);	
}

void sse_lsp_dispose_all (sse_lsp_mem_t* mem)
{
	sse_lsp_obj_t* obj, * next;
	sse_size_t i;

	for (i = 0; i < SSE_LSP_TYPE_COUNT; i++) {
		obj = mem->used[i];

		while (obj != SSE_NULL) {
			next = SSE_LSP_LINK(obj);
			sse_lsp_dispose (mem, SSE_NULL, obj);
			obj = next;
		}
	}
}

static void sse_lsp_mark_obj (sse_lsp_obj_t* obj)
{
	sse_assert (obj != SSE_NULL);

	// TODO:....
	// can it be recursive?
	if (SSE_LSP_MARK(obj) != 0) return;

	SSE_LSP_MARK(obj) = 1;

	if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS) {
		sse_lsp_mark_obj (SSE_LSP_CAR(obj));
		sse_lsp_mark_obj (SSE_LSP_CDR(obj));
	}
	else if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_FUNC) {
		sse_lsp_mark_obj (SSE_LSP_FFORMAL(obj));
		sse_lsp_mark_obj (SSE_LSP_FBODY(obj));
	}
	else if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_MACRO) {
		sse_lsp_mark_obj (SSE_LSP_MFORMAL(obj));
		sse_lsp_mark_obj (SSE_LSP_MBODY(obj));
	}
}

/*
 * sse_lsp_lock and sse_lsp_unlock_all are just called by sse_lsp_read.
 */
void sse_lsp_lock (sse_lsp_obj_t* obj)
{
	sse_assert (obj != SSE_NULL);
	SSE_LSP_LOCK(obj) = 1;
	//SSE_LSP_MARK(obj) = 1;
}

void sse_lsp_unlock (sse_lsp_obj_t* obj)
{
	sse_assert (obj != SSE_NULL);
	SSE_LSP_LOCK(obj) = 0;
}

void sse_lsp_unlock_all (sse_lsp_obj_t* obj)
{
	sse_assert (obj != SSE_NULL);

	SSE_LSP_LOCK(obj) = 0;

	if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS) {
		sse_lsp_unlock_all (SSE_LSP_CAR(obj));
		sse_lsp_unlock_all (SSE_LSP_CDR(obj));
	}
	else if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_FUNC) {
		sse_lsp_unlock_all (SSE_LSP_FFORMAL(obj));
		sse_lsp_unlock_all (SSE_LSP_FBODY(obj));
	}
	else if (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_MACRO) {
		sse_lsp_unlock_all (SSE_LSP_MFORMAL(obj));
		sse_lsp_unlock_all (SSE_LSP_MBODY(obj));
	}
}

static void sse_lsp_mark (sse_lsp_mem_t* mem)
{
	sse_lsp_frame_t* frame;
	sse_lsp_assoc_t* assoc;
	sse_lsp_array_t* array;
	sse_size_t       i;

#if 0
	sse_dprint0 (SSE_T("marking environment frames\n"));
#endif
	// mark objects in the environment frames
	frame = mem->frame;
	while (frame != SSE_NULL) {
		assoc = frame->assoc;
		while (assoc != SSE_NULL) {
			sse_lsp_mark_obj (assoc->name);

			if (assoc->value != SSE_NULL) 
				sse_lsp_mark_obj (assoc->value);
			if (assoc->func != SSE_NULL) 
				sse_lsp_mark_obj (assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

#if 0
	sse_dprint0 (SSE_T("marking interim frames\n"));
#endif

	// mark objects in the interim frames
	frame = mem->brooding_frame;
	while (frame != SSE_NULL) {

		assoc = frame->assoc;
		while (assoc != SSE_NULL) {
			sse_lsp_mark_obj (assoc->name);

			if (assoc->value != SSE_NULL) 
				sse_lsp_mark_obj (assoc->value);
			if (assoc->func != SSE_NULL) 
				sse_lsp_mark_obj (assoc->func);

			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*
	sse_dprint0 (SSE_T("marking the locked object\n"));
	if (mem->locked != SSE_NULL) sse_lsp_mark_obj (mem->locked);
	*/

#if 0
	sse_dprint0 (SSE_T("marking termporary objects\n"));
#endif
	array = mem->temp_array;
	for (i = 0; i < array->size; i++) {
		sse_lsp_mark_obj (array->buffer[i]);
	}

#if 0
	sse_dprint0 (SSE_T("marking builtin objects\n"));
#endif
	// mark common objects
	if (mem->t      != SSE_NULL) sse_lsp_mark_obj (mem->t);
	if (mem->nil    != SSE_NULL) sse_lsp_mark_obj (mem->nil);
	if (mem->quote  != SSE_NULL) sse_lsp_mark_obj (mem->quote);
	if (mem->lambda != SSE_NULL) sse_lsp_mark_obj (mem->lambda);
	if (mem->macro  != SSE_NULL) sse_lsp_mark_obj (mem->macro);
}

static void sse_lsp_sweep (sse_lsp_mem_t* mem)
{
	sse_lsp_obj_t* obj, * prev, * next;
	sse_size_t i;

	// scan all the allocated objects and get rid of unused objects
	for (i = 0; i < SSE_LSP_TYPE_COUNT; i++) {
	//for (i = SSE_LSP_TYPE_COUNT; i > 0; /*i--*/) {
		prev = SSE_NULL;
		obj = mem->used[i];
		//obj = mem->used[--i];

#if 0
		sse_dprint1 (SSE_T("sweeping objects of type: %u\n"), i);
#endif

		while (obj != SSE_NULL) {
			next = SSE_LSP_LINK(obj);

			if (SSE_LSP_LOCK(obj) == 0 && SSE_LSP_MARK(obj) == 0) {
				// dispose of unused objects
				sse_lsp_dispose (mem, prev, obj);
			}
			else {
				// unmark the object in use
				SSE_LSP_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

void sse_lsp_garbage_collect (sse_lsp_mem_t* mem)
{
	sse_lsp_mark (mem);
	sse_lsp_sweep (mem);
}

sse_lsp_obj_t* sse_lsp_make_nil (sse_lsp_mem_t* mem)
{
	if (mem->nil != SSE_NULL) return mem->nil;
	mem->nil = sse_lsp_alloc (mem, SSE_LSP_OBJ_NIL, sse_sizeof(sse_lsp_obj_nil_t));
	return mem->nil;
}

sse_lsp_obj_t* sse_lsp_make_true (sse_lsp_mem_t* mem)
{
	if (mem->t != SSE_NULL) return mem->t;
	mem->t = sse_lsp_alloc (mem, SSE_LSP_OBJ_TRUE, sse_sizeof(sse_lsp_obj_true_t));
	return mem->t;
}

sse_lsp_obj_t* sse_lsp_make_int (sse_lsp_mem_t* mem, sse_lsp_int_t value)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, 
		SSE_LSP_OBJ_INT, sse_sizeof(sse_lsp_obj_int_t));
	if (obj == SSE_NULL) return SSE_NULL;

	SSE_LSP_IVALUE(obj) = value;

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_real (sse_lsp_mem_t* mem, sse_lsp_real_t value)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, 
		SSE_LSP_OBJ_REAL, sse_sizeof(sse_lsp_obj_real_t));
	if (obj == SSE_NULL) return SSE_NULL;
	
	SSE_LSP_RVALUE(obj) = value;

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_symbol (sse_lsp_mem_t* mem, const sse_char_t* str)
{
	return sse_lsp_make_symbolx (mem, str, sse_strlen(str));
}

sse_lsp_obj_t* sse_lsp_make_symbolx (
	sse_lsp_mem_t* mem, const sse_char_t* str, sse_size_t len)
{
	sse_lsp_obj_t* obj;

	// look for a sysmbol with the given name
	obj = mem->used[SSE_LSP_OBJ_SYMBOL];
	while (obj != SSE_NULL) {
		// if there is a symbol with the same name, it is just used.
		if (sse_lsp_comp_symbol2 (obj, str, len) == 0) return obj;
		obj = SSE_LSP_LINK(obj);
	}

	// no such symbol found. create a new one 
	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_SYMBOL,
		sse_sizeof(sse_lsp_obj_symbol_t) + (len + 1) * sse_sizeof(sse_char_t));
	if (obj == SSE_NULL) return SSE_NULL;

	// fill in the symbol buffer
	sse_lsp_copy_string2 (SSE_LSP_SYMVALUE(obj), str, len);

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_string (sse_lsp_mem_t* mem, const sse_char_t* str)
{
	return sse_lsp_make_stringx (mem, str, sse_strlen(str));
}

sse_lsp_obj_t* sse_lsp_make_stringx (
	sse_lsp_mem_t* mem, const sse_char_t* str, sse_size_t len)
{
	sse_lsp_obj_t* obj;

	// allocate memory for the string
	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_STRING,
		sse_sizeof(sse_lsp_obj_string_t) + (len + 1) * sse_sizeof(sse_char_t));
	if (obj == SSE_NULL) return SSE_NULL;

	// fill in the string buffer
	sse_lsp_copy_string2 (SSE_LSP_STRVALUE(obj), str, len);

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_cons (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* car, sse_lsp_obj_t* cdr)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_CONS, sse_sizeof(sse_lsp_obj_cons_t));
	if (obj == SSE_NULL) return SSE_NULL;

	SSE_LSP_CAR(obj) = car;
	SSE_LSP_CDR(obj) = cdr;

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_func (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* formal, sse_lsp_obj_t* body)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_FUNC, sse_sizeof(sse_lsp_obj_func_t));
	if (obj == SSE_NULL) return SSE_NULL;

	SSE_LSP_FFORMAL(obj) = formal;
	SSE_LSP_FBODY(obj)   = body;

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_macro (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* formal, sse_lsp_obj_t* body)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_MACRO, sse_sizeof(sse_lsp_obj_macro_t));
	if (obj == SSE_NULL) return SSE_NULL;

	SSE_LSP_MFORMAL(obj) = formal;
	SSE_LSP_MBODY(obj)   = body;

	return obj;
}

sse_lsp_obj_t* sse_lsp_make_prim (sse_lsp_mem_t* mem, void* impl)
{
	sse_lsp_obj_t* obj;

	obj = sse_lsp_alloc (mem, SSE_LSP_OBJ_PRIM, sse_sizeof(sse_lsp_obj_prim_t));
	if (obj == SSE_NULL) return SSE_NULL;

	SSE_LSP_PRIM(obj) = impl;

	return obj;
}

sse_lsp_assoc_t* sse_lsp_lookup (sse_lsp_mem_t* mem, sse_lsp_obj_t* name)
{
	sse_lsp_frame_t* frame;
	sse_lsp_assoc_t* assoc;

	sse_assert (SSE_LSP_TYPE(name) == SSE_LSP_OBJ_SYMBOL);

	frame = mem->frame;

	while (frame != SSE_NULL) {
		assoc = sse_lsp_frame_lookup (frame, name);
		if (assoc != SSE_NULL) return assoc;
		frame = frame->link;
	}

	return SSE_NULL;
}

sse_lsp_assoc_t* sse_lsp_set_value (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* name, sse_lsp_obj_t* value)
{
	sse_lsp_assoc_t* assoc;

	assoc = sse_lsp_lookup (mem, name);
	if (assoc == SSE_NULL)  {
		assoc = sse_lsp_frame_insert_value (
			mem->root_frame, name, value);
		if (assoc == SSE_NULL) return SSE_NULL;
	}
	else assoc->value = value;

	return assoc;
}

sse_lsp_assoc_t* sse_lsp_set_func (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* name, sse_lsp_obj_t* func)
{
	sse_lsp_assoc_t* assoc;

	assoc = sse_lsp_lookup (mem, name);
	if (assoc == SSE_NULL)  {
		assoc = sse_lsp_frame_insert_func (mem->root_frame, name, func);
		if (assoc == SSE_NULL) return SSE_NULL;
	}
	else assoc->func = func;

	return assoc;
}

sse_size_t sse_lsp_cons_len (sse_lsp_mem_t* mem, sse_lsp_obj_t* obj)
{
	sse_size_t count;

	sse_assert (obj == mem->nil || SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS);

	count = 0;
	//while (obj != mem->nil) {
	while (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS) {
		count++;
		obj = SSE_LSP_CDR(obj);
	}

	return count;
}

int sse_lsp_probe_args (sse_lsp_mem_t* mem, sse_lsp_obj_t* obj, sse_size_t* len)
{
	sse_size_t count = 0;

	while (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_CONS) {
		count++;
		obj = SSE_LSP_CDR(obj);
	}	

	if (obj != mem->nil) return -1;

	*len = count;
	return 0;
}

int sse_lsp_comp_symbol (sse_lsp_obj_t* obj, const sse_char_t* str)
{
	sse_char_t* p;
	sse_size_t index, length;

	sse_assert (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = SSE_LSP_SYMLEN(obj);

	p = SSE_LSP_SYMVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == SSE_T('\0'))? 0: -1;
}

int sse_lsp_comp_symbol2 (sse_lsp_obj_t* obj, const sse_char_t* str, sse_size_t len)
{
	sse_char_t* p;
	sse_size_t index, length;

	sse_assert (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = SSE_LSP_SYMLEN(obj);
	p = SSE_LSP_SYMVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

int sse_lsp_comp_string (sse_lsp_obj_t* obj, const sse_char_t* str)
{
	sse_char_t* p;
	sse_size_t index, length;

	sse_assert (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_STRING);
	
	index = 0;
	length = SSE_LSP_STRLEN(obj);

	p = SSE_LSP_STRVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == SSE_T('\0'))? 0: -1;
}

int sse_lsp_comp_string2 (sse_lsp_obj_t* obj, const sse_char_t* str, sse_size_t len)
{
	sse_char_t* p;
	sse_size_t index, length;

	sse_assert (SSE_LSP_TYPE(obj) == SSE_LSP_OBJ_STRING);
	
	index = 0;
	length = SSE_LSP_STRLEN(obj);
	p = SSE_LSP_STRVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

void sse_lsp_copy_string (sse_char_t* dst, const sse_char_t* str)
{
	// the buffer pointed by dst should be big enough to hold str
	while (*str != SSE_T('\0')) *dst++ = *str++;
	*dst = SSE_T('\0');
}

void sse_lsp_copy_string2 (sse_char_t* dst, const sse_char_t* str, sse_size_t len)
{
	// the buffer pointed by dst should be big enough to hold str
	while (len > 0) {
		*dst++ = *str++;
		len--;
	}
	*dst = SSE_T('\0');
}

