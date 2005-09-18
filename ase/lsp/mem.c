/*
 * $Id: mem.c,v 1.1 2005-09-18 11:34:35 bacon Exp $
 */

#include <xp/lsp/mem.h> 
#include <xp/lsp/prim.h>

#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#include <xp/bas/dprint.h>

xp_lsp_mem_t* xp_lsp_mem_new (xp_size_t ubound, xp_size_t ubound_inc)
{
	xp_lsp_mem_t* mem;
	xp_size_t i;

	// allocate memory
	mem = (xp_lsp_mem_t*)xp_malloc (sizeof(xp_lsp_mem_t));	
	if (mem == XP_NULL) return XP_NULL;

	// create a new root environment frame
	mem->frame = xp_lsp_frame_new ();
	if (mem->frame == XP_NULL) {
		xp_free (mem);
		return XP_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = XP_NULL;

	// create an array to hold temporary objects
	mem->temp_array = xp_lsp_array_new (512);
	if (mem->temp_array == XP_NULL) {
		xp_lsp_frame_free (mem->frame);
		xp_free (mem);
		return XP_NULL;
	}

	// initialize object allocation list
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < XP_LSP_TYPE_COUNT; i++) {
		mem->used[i] = XP_NULL;
		mem->free[i] = XP_NULL;
	}
	mem->locked = XP_NULL;

	// when "ubound" is too small, the garbage collection can
	// be performed while making the common objects.
	mem->nil    = XP_NULL;
	mem->t      = XP_NULL;
	mem->quote  = XP_NULL;
	mem->lambda = XP_NULL;
	mem->macro  = XP_NULL;

	// initialize common object pointers
	mem->nil     = xp_lsp_make_nil    (mem);
	mem->t       = xp_lsp_make_true   (mem);
	mem->quote   = xp_lsp_make_symbol (mem, XP_TEXT("quote"),  5);
	mem->lambda  = xp_lsp_make_symbol (mem, XP_TEXT("lambda"), 6);
	mem->macro   = xp_lsp_make_symbol (mem, XP_TEXT("macro"),  5);

	if (mem->nil    == XP_NULL ||
	    mem->t      == XP_NULL ||
	    mem->quote  == XP_NULL ||
	    mem->lambda == XP_NULL ||
	    mem->macro  == XP_NULL) {
		xp_lsp_dispose_all (mem);
		xp_lsp_array_free (mem->temp_array);
		xp_lsp_frame_free (mem->frame);
		xp_free (mem);
		return XP_NULL;
	}

	return mem;
}

void xp_lsp_mem_free (xp_lsp_mem_t* mem)
{
	xp_assert (mem != XP_NULL);

	// dispose of the allocated objects
	xp_lsp_dispose_all (mem);

	// dispose of the temporary object arrays
	xp_lsp_array_free (mem->temp_array);

	// dispose of environment frames
	xp_lsp_frame_free (mem->frame);

	// free the memory
	xp_free (mem);
}

static int xp_lsp_add_prim (
	xp_lsp_mem_t* mem, const xp_char_t* name, xp_size_t len, xp_lsp_pimpl_t prim)
{
	xp_lsp_obj_t* n, * p;
	
	n = xp_lsp_make_symbol (mem, name, len);
	if (n == XP_NULL) return -1;

	xp_lsp_lock (n);

	p = xp_lsp_make_prim (mem, prim);
	if (p == XP_NULL) return -1;

	xp_lsp_unlock (n);

	if (xp_lsp_set (mem, n, p) == XP_NULL) return -1;

	return 0;
}


int xp_lsp_add_prims (xp_lsp_mem_t* mem)
{

#define ADD_PRIM(mem,name,len,prim) \
	if (xp_lsp_add_prim(mem,name,len,prim) == -1) return -1;

	ADD_PRIM (mem, XP_TEXT("abort"), 5, xp_lsp_prim_abort);
	ADD_PRIM (mem, XP_TEXT("eval"),  4, xp_lsp_prim_eval);
	ADD_PRIM (mem, XP_TEXT("prog1"), 5, xp_lsp_prim_prog1);
	ADD_PRIM (mem, XP_TEXT("progn"), 5, xp_lsp_prim_progn);
	ADD_PRIM (mem, XP_TEXT("gc"),    2, xp_lsp_prim_gc);

	ADD_PRIM (mem, XP_TEXT("cond"),  4, xp_lsp_prim_cond);
	ADD_PRIM (mem, XP_TEXT("if"),    2, xp_lsp_prim_if);
	ADD_PRIM (mem, XP_TEXT("while"), 5, xp_lsp_prim_while);

	ADD_PRIM (mem, XP_TEXT("car"),   3, xp_lsp_prim_car);
	ADD_PRIM (mem, XP_TEXT("cdr"),   3, xp_lsp_prim_cdr);
	ADD_PRIM (mem, XP_TEXT("cons"),  4, xp_lsp_prim_cons);
	ADD_PRIM (mem, XP_TEXT("set"),   3, xp_lsp_prim_set);
	ADD_PRIM (mem, XP_TEXT("setq"),  4, xp_lsp_prim_setq);
	ADD_PRIM (mem, XP_TEXT("quote"), 5, xp_lsp_prim_quote);
	ADD_PRIM (mem, XP_TEXT("defun"), 5, xp_lsp_prim_defun);
	ADD_PRIM (mem, XP_TEXT("demac"), 5, xp_lsp_prim_demac);
	ADD_PRIM (mem, XP_TEXT("let"),   3, xp_lsp_prim_let);
	ADD_PRIM (mem, XP_TEXT("let*"),  4, xp_lsp_prim_letx);

	ADD_PRIM (mem, XP_TEXT("+"),     1, xp_lsp_prim_plus);
	ADD_PRIM (mem, XP_TEXT(">"),     1, xp_lsp_prim_gt);
	ADD_PRIM (mem, XP_TEXT("<"),     1, xp_lsp_prim_lt);

	return 0;
}


xp_lsp_obj_t* xp_lsp_allocate (xp_lsp_mem_t* mem, int type, xp_size_t size)
{
	xp_lsp_obj_t* obj;
	
	if (mem->count >= mem->ubound) xp_lsp_garbage_collect (mem);
	if (mem->count >= mem->ubound) {
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return XP_NULL;
	}

	obj = (xp_lsp_obj_t*)xp_malloc (size);
	if (obj == XP_NULL) {
		xp_lsp_garbage_collect (mem);

		obj = (xp_lsp_obj_t*)xp_malloc (size);
		if (obj == XP_NULL) return XP_NULL;
	}

	XP_LSP_TYPE(obj) = type;
	XP_LSP_SIZE(obj) = size;
	XP_LSP_MARK(obj) = 0;
	XP_LSP_LOCK(obj) = 0;

	// insert the object at the head of the used list
	XP_LSP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;
	xp_dprint1 (XP_TEXT("mem->count: %u\n"), mem->count);

	return obj;
}

void xp_lsp_dispose (xp_lsp_mem_t* mem, xp_lsp_obj_t* prev, xp_lsp_obj_t* obj)
{
	xp_assert (mem != XP_NULL);
	xp_assert (obj != XP_NULL);
	xp_assert (mem->count > 0);

	// TODO: push the object to the free list for more 
	//       efficient memory management

	if (prev == XP_NULL) 
		mem->used[XP_LSP_TYPE(obj)] = XP_LSP_LINK(obj);
	else XP_LSP_LINK(prev) = XP_LSP_LINK(obj);

	mem->count--;
	xp_dprint1 (XP_TEXT("mem->count: %u\n"), mem->count);

	xp_free (obj);	
}

void xp_lsp_dispose_all (xp_lsp_mem_t* mem)
{
	xp_lsp_obj_t* obj, * next;
	xp_size_t i;

	for (i = 0; i < XP_LSP_TYPE_COUNT; i++) {
		obj = mem->used[i];

		while (obj != XP_NULL) {
			next = XP_LSP_LINK(obj);
			xp_lsp_dispose (mem, XP_NULL, obj);
			obj = next;
		}
	}
}

static void xp_lsp_mark_obj (xp_lsp_obj_t* obj)
{
	xp_assert (obj != XP_NULL);

	// TODO:....
	// can it be recursive?
	if (XP_LSP_MARK(obj) != 0) return;

	XP_LSP_MARK(obj) = 1;

	if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS) {
		xp_lsp_mark_obj (XP_LSP_CAR(obj));
		xp_lsp_mark_obj (XP_LSP_CDR(obj));
	}
	else if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_FUNC) {
		xp_lsp_mark_obj (XP_LSP_FFORMAL(obj));
		xp_lsp_mark_obj (XP_LSP_FBODY(obj));
	}
	else if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_MACRO) {
		xp_lsp_mark_obj (XP_LSP_MFORMAL(obj));
		xp_lsp_mark_obj (XP_LSP_MBODY(obj));
	}
}

/*
 * xp_lsp_lock and xp_lsp_unlock_all are just called by xp_lsp_read.
 */
void xp_lsp_lock (xp_lsp_obj_t* obj)
{
	xp_assert (obj != XP_NULL);
	XP_LSP_LOCK(obj) = 1;
	//XP_LSP_MARK(obj) = 1;
}

void xp_lsp_unlock (xp_lsp_obj_t* obj)
{
	xp_assert (obj != XP_NULL);
	XP_LSP_LOCK(obj) = 0;
}

void xp_lsp_unlock_all (xp_lsp_obj_t* obj)
{
	xp_assert (obj != XP_NULL);

	XP_LSP_LOCK(obj) = 0;

	if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS) {
		xp_lsp_unlock_all (XP_LSP_CAR(obj));
		xp_lsp_unlock_all (XP_LSP_CDR(obj));
	}
	else if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_FUNC) {
		xp_lsp_unlock_all (XP_LSP_FFORMAL(obj));
		xp_lsp_unlock_all (XP_LSP_FBODY(obj));
	}
	else if (XP_LSP_TYPE(obj) == XP_LSP_OBJ_MACRO) {
		xp_lsp_unlock_all (XP_LSP_MFORMAL(obj));
		xp_lsp_unlock_all (XP_LSP_MBODY(obj));
	}
}

static void xp_lsp_mark (xp_lsp_mem_t* mem)
{
	xp_lsp_frame_t* frame;
	xp_lsp_assoc_t* assoc;
	xp_lsp_array_t* array;
	xp_size_t       i;

	xp_dprint0 (XP_TEXT("marking environment frames\n"));
	// mark objects in the environment frames
	frame = mem->frame;
	while (frame != XP_NULL) {
		assoc = frame->assoc;
		while (assoc != XP_NULL) {
			xp_lsp_mark_obj (assoc->name);
			xp_lsp_mark_obj (assoc->value);
			assoc = assoc->link;
		}

		frame = frame->link;
	}

	xp_dprint0 (XP_TEXT("marking interim frames\n"));

	// mark objects in the interim frames
	frame = mem->brooding_frame;
	while (frame != XP_NULL) {

		assoc = frame->assoc;
		while (assoc != XP_NULL) {
			xp_lsp_mark_obj (assoc->name);
			xp_lsp_mark_obj (assoc->value);
			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*
	xp_dprint0 (XP_TEXT("marking the locked object\n"));
	if (mem->locked != XP_NULL) xp_lsp_mark_obj (mem->locked);
	*/

	xp_dprint0 (XP_TEXT("marking termporary objects\n"));
	array = mem->temp_array;
	for (i = 0; i < array->size; i++) {
		xp_lsp_mark_obj (array->buffer[i]);
	}

	xp_dprint0 (XP_TEXT("marking builtin objects\n"));
	// mark common objects
	if (mem->t      != XP_NULL) xp_lsp_mark_obj (mem->t);
	if (mem->nil    != XP_NULL) xp_lsp_mark_obj (mem->nil);
	if (mem->quote  != XP_NULL) xp_lsp_mark_obj (mem->quote);
	if (mem->lambda != XP_NULL) xp_lsp_mark_obj (mem->lambda);
	if (mem->macro  != XP_NULL) xp_lsp_mark_obj (mem->macro);
}

static void xp_lsp_sweep (xp_lsp_mem_t* mem)
{
	xp_lsp_obj_t* obj, * prev, * next;
	xp_size_t i;

	// scan all the allocated objects and get rid of unused objects
	for (i = 0; i < XP_LSP_TYPE_COUNT; i++) {
	//for (i = XP_LSP_TYPE_COUNT; i > 0; /*i--*/) {
		prev = XP_NULL;
		obj = mem->used[i];
		//obj = mem->used[--i];

		xp_dprint1 (XP_TEXT("sweeping objects of type: %u\n"), i);

		while (obj != XP_NULL) {
			next = XP_LSP_LINK(obj);

			if (XP_LSP_LOCK(obj) == 0 && XP_LSP_MARK(obj) == 0) {
				// dispose of unused objects
				xp_lsp_dispose (mem, prev, obj);
			}
			else {
				// unmark the object in use
				XP_LSP_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

void xp_lsp_garbage_collect (xp_lsp_mem_t* mem)
{
	xp_lsp_mark (mem);
	xp_lsp_sweep (mem);
}

xp_lsp_obj_t* xp_lsp_make_nil (xp_lsp_mem_t* mem)
{
	if (mem->nil != XP_NULL) return mem->nil;
	mem->nil = xp_lsp_allocate (mem, XP_LSP_OBJ_NIL, sizeof(xp_lsp_obj_nil_t));
	return mem->nil;
}

xp_lsp_obj_t* xp_lsp_make_true (xp_lsp_mem_t* mem)
{
	if (mem->t != XP_NULL) return mem->t;
	mem->t = xp_lsp_allocate (mem, XP_LSP_OBJ_TRUE, sizeof(xp_lsp_obj_true_t));
	return mem->t;
}

xp_lsp_obj_t* xp_lsp_make_int (xp_lsp_mem_t* mem, xp_lsp_int_t value)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_INT, sizeof(xp_lsp_obj_int_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LSP_IVALUE(obj) = value;

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_float (xp_lsp_mem_t* mem, xp_lsp_real_t value)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_FLOAT, sizeof(xp_lsp_obj_float_t));
	if (obj == XP_NULL) return XP_NULL;
	
	XP_LSP_FVALUE(obj) = value;

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_symbol (
	xp_lsp_mem_t* mem, const xp_char_t* str, xp_size_t len)
{
	xp_lsp_obj_t* obj;

	// look for a sysmbol with the given name
	obj = mem->used[XP_LSP_OBJ_SYMBOL];
	while (obj != XP_NULL) {
		// if there is a symbol with the same name, it is just used.
		if (xp_lsp_comp_symbol2 (obj, str, len) == 0) return obj;
		obj = XP_LSP_LINK(obj);
	}

	// no such symbol found. create a new one 
	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_SYMBOL,
		sizeof(xp_lsp_obj_symbol_t) + (len + 1) * sizeof(xp_char_t));
	if (obj == XP_NULL) return XP_NULL;

	// fill in the symbol buffer
	xp_lsp_copy_string2 (XP_LSP_SYMVALUE(obj), str, len);

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_string (xp_lsp_mem_t* mem, const xp_char_t* str, xp_size_t len)
{
	xp_lsp_obj_t* obj;

	// allocate memory for the string
	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_STRING,
		sizeof(xp_lsp_obj_string_t) + (len + 1) * sizeof(xp_char_t));
	if (obj == XP_NULL) return XP_NULL;

	// fill in the string buffer
	xp_lsp_copy_string2 (XP_LSP_STRVALUE(obj), str, len);

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_cons (xp_lsp_mem_t* mem, xp_lsp_obj_t* car, xp_lsp_obj_t* cdr)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_CONS, sizeof(xp_lsp_obj_cons_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LSP_CAR(obj) = car;
	XP_LSP_CDR(obj) = cdr;

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_func (xp_lsp_mem_t* mem, xp_lsp_obj_t* formal, xp_lsp_obj_t* body)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_FUNC, sizeof(xp_lsp_obj_func_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LSP_FFORMAL(obj) = formal;
	XP_LSP_FBODY(obj)   = body;

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_macro (xp_lsp_mem_t* mem, xp_lsp_obj_t* formal, xp_lsp_obj_t* body)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_MACRO, sizeof(xp_lsp_obj_macro_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LSP_MFORMAL(obj) = formal;
	XP_LSP_MBODY(obj)   = body;

	return obj;
}

xp_lsp_obj_t* xp_lsp_make_prim (xp_lsp_mem_t* mem, void* impl)
{
	xp_lsp_obj_t* obj;

	obj = xp_lsp_allocate (mem, XP_LSP_OBJ_PRIM, sizeof(xp_lsp_obj_prim_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LSP_PIMPL(obj) = impl;

	return obj;
}

xp_lsp_assoc_t* xp_lsp_lookup (xp_lsp_mem_t* mem, xp_lsp_obj_t* name)
{
	xp_lsp_frame_t* frame;
	xp_lsp_assoc_t* assoc;

	xp_assert (XP_LSP_TYPE(name) == XP_LSP_OBJ_SYMBOL);

	frame = mem->frame;

	while (frame != XP_NULL) {
		assoc = xp_lsp_frame_lookup (frame, name);
		if (assoc != XP_NULL) return assoc;
		frame = frame->link;
	}

	return XP_NULL;
}

xp_lsp_assoc_t* xp_lsp_set (xp_lsp_mem_t* mem, xp_lsp_obj_t* name, xp_lsp_obj_t* value)
{
	xp_lsp_assoc_t* assoc;

	assoc = xp_lsp_lookup (mem, name);
	if (assoc == XP_NULL)  {
		assoc = xp_lsp_frame_insert (mem->root_frame, name, value);
		if (assoc == XP_NULL) return XP_NULL;
	}
	else assoc->value = value;

	return assoc;
}

xp_size_t xp_lsp_cons_len (xp_lsp_mem_t* mem, xp_lsp_obj_t* obj)
{
	xp_size_t count;

	xp_assert (obj == mem->nil || XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS);

	count = 0;
	//while (obj != mem->nil) {
	while (XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS) {
		count++;
		obj = XP_LSP_CDR(obj);
	}

	return count;
}

int xp_lsp_probe_args (xp_lsp_mem_t* mem, xp_lsp_obj_t* obj, xp_size_t* len)
{
	xp_size_t count = 0;

	while (XP_LSP_TYPE(obj) == XP_LSP_OBJ_CONS) {
		count++;
		obj = XP_LSP_CDR(obj);
	}	

	if (obj != mem->nil) return -1;

	*len = count;
	return 0;
}

int xp_lsp_comp_symbol (xp_lsp_obj_t* obj, const xp_char_t* str)
{
	xp_char_t* p;
	xp_size_t index, length;

	xp_assert (XP_LSP_TYPE(obj) == XP_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = XP_LSP_SYMLEN(obj);

	p = XP_LSP_SYMVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}

int xp_lsp_comp_symbol2 (xp_lsp_obj_t* obj, const xp_char_t* str, xp_size_t len)
{
	xp_char_t* p;
	xp_size_t index, length;

	xp_assert (XP_LSP_TYPE(obj) == XP_LSP_OBJ_SYMBOL);
	
	index = 0;
	length = XP_LSP_SYMLEN(obj);
	p = XP_LSP_SYMVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

int xp_lsp_comp_string (xp_lsp_obj_t* obj, const xp_char_t* str)
{
	xp_char_t* p;
	xp_size_t index, length;

	xp_assert (XP_LSP_TYPE(obj) == XP_LSP_OBJ_STRING);
	
	index = 0;
	length = XP_LSP_STRLEN(obj);

	p = XP_LSP_STRVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}

int xp_lsp_comp_string2 (xp_lsp_obj_t* obj, const xp_char_t* str, xp_size_t len)
{
	xp_char_t* p;
	xp_size_t index, length;

	xp_assert (XP_LSP_TYPE(obj) == XP_LSP_OBJ_STRING);
	
	index = 0;
	length = XP_LSP_STRLEN(obj);
	p = XP_LSP_STRVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

void xp_lsp_copy_string (xp_char_t* dst, const xp_char_t* str)
{
	// the buffer pointed by dst should be big enough to hold str
	while (*str != XP_CHAR('\0')) *dst++ = *str++;
	*dst = XP_CHAR('\0');
}

void xp_lsp_copy_string2 (xp_char_t* dst, const xp_char_t* str, xp_size_t len)
{
	// the buffer pointed by dst should be big enough to hold str
	while (len > 0) {
		*dst++ = *str++;
		len--;
	}
	*dst = XP_CHAR('\0');
}

