/*
 * $Id: memory.c,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#include <xp/lisp/memory.h> 
#include <xp/lisp/primitive.h>
#include <xp/c/stdlib.h>

xp_lisp_mem_t* xp_lisp_mem_new (xp_size_t ubound, xp_size_t ubound_inc)
{
	xp_lisp_mem_t* mem;
	xp_size_t i;

	// allocate memory
	mem = (xp_lisp_mem_t*)xp_malloc (sizeof(xp_lisp_mem_t));	
	if (mem == XP_NULL) return XP_NULL;

	// create a new root environment frame
	mem->frame = xp_lisp_frame_new ();
	if (mem->frame == XP_NULL) {
		xp_free (mem);
		return XP_NULL;
	}
	mem->root_frame     = mem->frame;
	mem->brooding_frame = XP_NULL;

	// create an array to hold temporary objects
	mem->temp_array = xp_lisp_array_new (512);
	if (mem->temp_array == XP_NULL) {
		xp_lisp_frame_free (mem->frame);
		xp_free (mem);
		return XP_NULL;
	}

	// initialize object allocation list
	mem->ubound     = ubound;
	mem->ubound_inc = ubound_inc;
	mem->count      = 0;
	for (i = 0; i < XP_LISP_TYPE_COUNT; i++) {
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
	mem->nil     = xp_lisp_make_nil    (mem);
	mem->t       = xp_lisp_make_true   (mem);
	mem->quote   = xp_lisp_make_symbol (mem, XP_LISP_TEXT("quote"),  5);
	mem->lambda  = xp_lisp_make_symbol (mem, XP_LISP_TEXT("lambda"), 6);
	mem->macro   = xp_lisp_make_symbol (mem, XP_LISP_TEXT("macro"),  5);

	if (mem->nil    == XP_NULL ||
	    mem->t      == XP_NULL ||
	    mem->quote  == XP_NULL ||
	    mem->lambda == XP_NULL ||
	    mem->macro  == XP_NULL) {
		xp_lisp_dispose_all (mem);
		xp_lisp_array_free (mem->temp_array);
		xp_lisp_frame_free (mem->frame);
		xp_free (mem);
		return XP_NULL;
	}

	return mem;
}

void xp_lisp_mem_free (xp_lisp_mem_t* mem)
{
	xp_lisp_assert (mem != XP_NULL);

	// dispose of the allocated objects
	xp_lisp_dispose_all (mem);

	// dispose of the temporary object arrays
	xp_lisp_array_free (mem->temp_array);

	// dispose of environment frames
	xp_lisp_frame_free (mem->frame);

	// free the memory
	xp_free (mem);
}

static int xp_lisp_add_prim (
	xp_lisp_mem_t* mem, const xp_lisp_char* name, xp_size_t len, xp_lisp_pimpl_t prim)
{
	xp_lisp_obj_t* n, * p;
	
	n = xp_lisp_make_symbol (mem, name, len);
	if (n == XP_NULL) return -1;

	xp_lisp_lock (n);

	p = xp_lisp_make_prim (mem, prim);
	if (p == XP_NULL) return -1;

	xp_lisp_unlock (n);

	if (xp_lisp_set (mem, n, p) == XP_NULL) return -1;

	return 0;
}


int xp_lisp_add_prims (xp_lisp_mem_t* mem)
{

#define ADD_PRIM(mem,name,len,prim) \
	if (xp_lisp_add_prim(mem,name,len,prim) == -1) return -1;

	ADD_PRIM (mem, XP_TEXT("abort"), 5, xp_lisp_prim_abort);
	ADD_PRIM (mem, XP_TEXT("eval"),  4, xp_lisp_prim_eval);
	ADD_PRIM (mem, XP_TEXT("prog1"), 5, xp_lisp_prim_prog1);
	ADD_PRIM (mem, XP_TEXT("progn"), 5, xp_lisp_prim_progn);
	ADD_PRIM (mem, XP_TEXT("gc"),    2, xp_lisp_prim_gc);

	ADD_PRIM (mem, XP_TEXT("cond"),  4, xp_lisp_prim_cond);
	ADD_PRIM (mem, XP_TEXT("if"),    2, xp_lisp_prim_if);
	ADD_PRIM (mem, XP_TEXT("while"), 5, xp_lisp_prim_while);

	ADD_PRIM (mem, XP_TEXT("car"),   3, xp_lisp_prim_car);
	ADD_PRIM (mem, XP_TEXT("cdr"),   3, xp_lisp_prim_cdr);
	ADD_PRIM (mem, XP_TEXT("cons"),  4, xp_lisp_prim_cons);
	ADD_PRIM (mem, XP_TEXT("set"),   3, xp_lisp_prim_set);
	ADD_PRIM (mem, XP_TEXT("setq"),  4, xp_lisp_prim_setq);
	ADD_PRIM (mem, XP_TEXT("quote"), 5, xp_lisp_prim_quote);
	ADD_PRIM (mem, XP_TEXT("defun"), 5, xp_lisp_prim_defun);
	ADD_PRIM (mem, XP_TEXT("demac"), 5, xp_lisp_prim_demac);
	ADD_PRIM (mem, XP_TEXT("let"),   3, xp_lisp_prim_let);
	ADD_PRIM (mem, XP_TEXT("let*"),  4, xp_lisp_prim_letx);

	ADD_PRIM (mem, XP_TEXT("+"),     1, xp_lisp_prim_plus);
	ADD_PRIM (mem, XP_TEXT(">"),     1, xp_lisp_prim_gt);
	ADD_PRIM (mem, XP_TEXT("<"),     1, xp_lisp_prim_lt);

	return 0;
}


xp_lisp_obj_t* xp_lisp_allocate (xp_lisp_mem_t* mem, int type, xp_size_t size)
{
	xp_lisp_obj_t* obj;
	
	if (mem->count >= mem->ubound) xp_lisp_garbage_collect (mem);
	if (mem->count >= mem->ubound) {
		mem->ubound += mem->ubound_inc;
		if (mem->count >= mem->ubound) return XP_NULL;
	}

	obj = (xp_lisp_obj_t*)xp_malloc (size);
	if (obj == XP_NULL) {
		xp_lisp_garbage_collect (mem);

		obj = (xp_lisp_obj_t*)xp_malloc (size);
		if (obj == XP_NULL) return XP_NULL;
	}

	XP_LISP_TYPE(obj) = type;
	XP_LISP_SIZE(obj) = size;
	XP_LISP_MARK(obj) = 0;
	XP_LISP_LOCK(obj) = 0;

	// insert the object at the head of the used list
	XP_LISP_LINK(obj) = mem->used[type];
	mem->used[type] = obj;
	mem->count++;
	XP_DEBUG1 (XP_TEXT("mem->count: %u\n"), mem->count);

	return obj;
}

void xp_lisp_dispose (xp_lisp_mem_t* mem, xp_lisp_obj_t* prev, xp_lisp_obj_t* obj)
{
	xp_lisp_assert (mem != XP_NULL);
	xp_lisp_assert (obj != XP_NULL);
	xp_lisp_assert (mem->count > 0);

	// TODO: push the object to the free list for more 
	//       efficient memory management

	if (prev == XP_NULL) 
		mem->used[XP_LISP_TYPE(obj)] = XP_LISP_LINK(obj);
	else XP_LISP_LINK(prev) = XP_LISP_LINK(obj);

	mem->count--;
	XP_DEBUG1 (XP_TEXT("mem->count: %u\n"), mem->count);

	xp_free (obj);	
}

void xp_lisp_dispose_all (xp_lisp_mem_t* mem)
{
	xp_lisp_obj_t* obj, * next;
	xp_size_t i;

	for (i = 0; i < XP_LISP_TYPE_COUNT; i++) {
		obj = mem->used[i];

		while (obj != XP_NULL) {
			next = XP_LISP_LINK(obj);
			xp_lisp_dispose (mem, XP_NULL, obj);
			obj = next;
		}
	}
}

static void xp_lisp_mark_obj (xp_lisp_obj_t* obj)
{
	xp_lisp_assert (obj != XP_NULL);

	// TODO:....
	// can it be recursive?
	if (XP_LISP_MARK(obj) != 0) return;

	XP_LISP_MARK(obj) = 1;

	if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS) {
		xp_lisp_mark_obj (XP_LISP_CAR(obj));
		xp_lisp_mark_obj (XP_LISP_CDR(obj));
	}
	else if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_FUNC) {
		xp_lisp_mark_obj (XP_LISP_FFORMAL(obj));
		xp_lisp_mark_obj (XP_LISP_FBODY(obj));
	}
	else if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_MACRO) {
		xp_lisp_mark_obj (XP_LISP_MFORMAL(obj));
		xp_lisp_mark_obj (XP_LISP_MBODY(obj));
	}
}

/*
 * xp_lisp_lock and xp_lisp_unlock_all are just called by xp_lisp_read.
 */
void xp_lisp_lock (xp_lisp_obj_t* obj)
{
	xp_lisp_assert (obj != XP_NULL);
	XP_LISP_LOCK(obj) = 1;
	//XP_LISP_MARK(obj) = 1;
}

void xp_lisp_unlock (xp_lisp_obj_t* obj)
{
	xp_lisp_assert (obj != XP_NULL);
	XP_LISP_LOCK(obj) = 0;
}

void xp_lisp_unlock_all (xp_lisp_obj_t* obj)
{
	xp_lisp_assert (obj != XP_NULL);

	XP_LISP_LOCK(obj) = 0;

	if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS) {
		xp_lisp_unlock_all (XP_LISP_CAR(obj));
		xp_lisp_unlock_all (XP_LISP_CDR(obj));
	}
	else if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_FUNC) {
		xp_lisp_unlock_all (XP_LISP_FFORMAL(obj));
		xp_lisp_unlock_all (XP_LISP_FBODY(obj));
	}
	else if (XP_LISP_TYPE(obj) == XP_LISP_OBJ_MACRO) {
		xp_lisp_unlock_all (XP_LISP_MFORMAL(obj));
		xp_lisp_unlock_all (XP_LISP_MBODY(obj));
	}
}

static void xp_lisp_mark (xp_lisp_mem_t* mem)
{
	xp_lisp_frame_t* frame;
	xp_lisp_assoc_t* assoc;
	xp_lisp_array_t* array;
	xp_size_t       i;

	XP_DEBUG0 (XP_TEXT("marking environment frames\n"));
	// mark objects in the environment frames
	frame = mem->frame;
	while (frame != XP_NULL) {
		assoc = frame->assoc;
		while (assoc != XP_NULL) {
			xp_lisp_mark_obj (assoc->name);
			xp_lisp_mark_obj (assoc->value);
			assoc = assoc->link;
		}

		frame = frame->link;
	}

	XP_DEBUG0 (XP_TEXT("marking interim frames\n"));

	// mark objects in the interim frames
	frame = mem->brooding_frame;
	while (frame != XP_NULL) {

		assoc = frame->assoc;
		while (assoc != XP_NULL) {
			xp_lisp_mark_obj (assoc->name);
			xp_lisp_mark_obj (assoc->value);
			assoc = assoc->link;
		}

		frame = frame->link;
	}

	/*
	XP_DEBUG0 (XP_TEXT("marking the locked object\n"));
	if (mem->locked != XP_NULL) xp_lisp_mark_obj (mem->locked);
	*/

	XP_DEBUG0 (XP_TEXT("marking termporary objects\n"));
	array = mem->temp_array;
	for (i = 0; i < array->size; i++) {
		xp_lisp_mark_obj (array->buffer[i]);
	}

	XP_DEBUG0 (XP_TEXT("marking builtin objects\n"));
	// mark common objects
	if (mem->t      != XP_NULL) xp_lisp_mark_obj (mem->t);
	if (mem->nil    != XP_NULL) xp_lisp_mark_obj (mem->nil);
	if (mem->quote  != XP_NULL) xp_lisp_mark_obj (mem->quote);
	if (mem->lambda != XP_NULL) xp_lisp_mark_obj (mem->lambda);
	if (mem->macro  != XP_NULL) xp_lisp_mark_obj (mem->macro);
}

static void xp_lisp_sweep (xp_lisp_mem_t* mem)
{
	xp_lisp_obj_t* obj, * prev, * next;
	xp_size_t i;

	// scan all the allocated objects and get rid of unused objects
	for (i = 0; i < XP_LISP_TYPE_COUNT; i++) {
	//for (i = XP_LISP_TYPE_COUNT; i > 0; /*i--*/) {
		prev = XP_NULL;
		obj = mem->used[i];
		//obj = mem->used[--i];

		XP_DEBUG1 (XP_TEXT("sweeping objects of type: %u\n"), i);

		while (obj != XP_NULL) {
			next = XP_LISP_LINK(obj);

			if (XP_LISP_LOCK(obj) == 0 && XP_LISP_MARK(obj) == 0) {
				// dispose of unused objects
				xp_lisp_dispose (mem, prev, obj);
			}
			else {
				// unmark the object in use
				XP_LISP_MARK(obj) = 0; 
				prev = obj;
			}

			obj = next;
		}
	}
}

void xp_lisp_garbage_collect (xp_lisp_mem_t* mem)
{
	xp_lisp_mark (mem);
	xp_lisp_sweep (mem);
}

xp_lisp_obj_t* xp_lisp_make_nil (xp_lisp_mem_t* mem)
{
	if (mem->nil != XP_NULL) return mem->nil;
	mem->nil = xp_lisp_allocate (mem, XP_LISP_OBJ_NIL, sizeof(xp_lisp_obj_nil_t));
	return mem->nil;
}

xp_lisp_obj_t* xp_lisp_make_true (xp_lisp_mem_t* mem)
{
	if (mem->t != XP_NULL) return mem->t;
	mem->t = xp_lisp_allocate (mem, XP_LISP_OBJ_TRUE, sizeof(xp_lisp_obj_true_t));
	return mem->t;
}

xp_lisp_obj_t* xp_lisp_make_int (xp_lisp_mem_t* mem, xp_lisp_int value)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_INT, sizeof(xp_lisp_obj_int_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LISP_IVALUE(obj) = value;

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_float (xp_lisp_mem_t* mem, xp_lisp_float value)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_FLOAT, sizeof(xp_lisp_obj_float_t));
	if (obj == XP_NULL) return XP_NULL;
	
	XP_LISP_FVALUE(obj) = value;

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_symbol (xp_lisp_mem_t* mem, const xp_lisp_char* str, xp_size_t len)
{
	xp_lisp_obj_t* obj;

	// look for a sysmbol with the given name
	obj = mem->used[XP_LISP_OBJ_SYMBOL];
	while (obj != XP_NULL) {
		// if there is a symbol with the same name, it is just used.
		if (xp_lisp_comp_symbol2 (obj, str, len) == 0) return obj;
		obj = XP_LISP_LINK(obj);
	}

	// no such symbol found. create a new one 
	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_SYMBOL,
		sizeof(xp_lisp_obj_symbol_t) + (len + 1) * sizeof(xp_lisp_char));
	if (obj == XP_NULL) return XP_NULL;

	// fill in the symbol buffer
	xp_lisp_copy_string2 (XP_LISP_SYMVALUE(obj), str, len);

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_string (xp_lisp_mem_t* mem, const xp_lisp_char* str, xp_size_t len)
{
	xp_lisp_obj_t* obj;

	// allocate memory for the string
	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_STRING,
		sizeof(xp_lisp_obj_string_t) + (len + 1) * sizeof(xp_lisp_char));
	if (obj == XP_NULL) return XP_NULL;

	// fill in the string buffer
	xp_lisp_copy_string2 (XP_LISP_STRVALUE(obj), str, len);

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_cons (xp_lisp_mem_t* mem, xp_lisp_obj_t* car, xp_lisp_obj_t* cdr)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_CONS, sizeof(xp_lisp_obj_cons_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LISP_CAR(obj) = car;
	XP_LISP_CDR(obj) = cdr;

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_func (xp_lisp_mem_t* mem, xp_lisp_obj_t* formal, xp_lisp_obj_t* body)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_FUNC, sizeof(xp_lisp_obj_func_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LISP_FFORMAL(obj) = formal;
	XP_LISP_FBODY(obj)   = body;

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_macro (xp_lisp_mem_t* mem, xp_lisp_obj_t* formal, xp_lisp_obj_t* body)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_MACRO, sizeof(xp_lisp_obj_macro_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LISP_MFORMAL(obj) = formal;
	XP_LISP_MBODY(obj)   = body;

	return obj;
}

xp_lisp_obj_t* xp_lisp_make_prim (xp_lisp_mem_t* mem, void* impl)
{
	xp_lisp_obj_t* obj;

	obj = xp_lisp_allocate (mem, XP_LISP_OBJ_PRIM, sizeof(xp_lisp_obj_prim_t));
	if (obj == XP_NULL) return XP_NULL;

	XP_LISP_PIMPL(obj) = impl;

	return obj;
}

xp_lisp_assoc_t* xp_lisp_lookup (xp_lisp_mem_t* mem, xp_lisp_obj_t* name)
{
	xp_lisp_frame_t* frame;
	xp_lisp_assoc_t* assoc;

	xp_lisp_assert (XP_LISP_TYPE(name) == XP_LISP_OBJ_SYMBOL);

	frame = mem->frame;

	while (frame != XP_NULL) {
		assoc = xp_lisp_frame_lookup (frame, name);
		if (assoc != XP_NULL) return assoc;
		frame = frame->link;
	}

	return XP_NULL;
}

xp_lisp_assoc_t* xp_lisp_set (xp_lisp_mem_t* mem, xp_lisp_obj_t* name, xp_lisp_obj_t* value)
{
	xp_lisp_assoc_t* assoc;

	assoc = xp_lisp_lookup (mem, name);
	if (assoc == XP_NULL)  {
		assoc = xp_lisp_frame_insert (mem->root_frame, name, value);
		if (assoc == XP_NULL) return XP_NULL;
	}
	else assoc->value = value;

	return assoc;
}

xp_size_t xp_lisp_cons_len (xp_lisp_mem_t* mem, xp_lisp_obj_t* obj)
{
	xp_size_t count;

	xp_lisp_assert (obj == mem->nil || XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS);

	count = 0;
	//while (obj != mem->nil) {
	while (XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS) {
		count++;
		obj = XP_LISP_CDR(obj);
	}

	return count;
}

int xp_lisp_probe_args (xp_lisp_mem_t* mem, xp_lisp_obj_t* obj, xp_size_t* len)
{
	xp_size_t count = 0;

	while (XP_LISP_TYPE(obj) == XP_LISP_OBJ_CONS) {
		count++;
		obj = XP_LISP_CDR(obj);
	}	

	if (obj != mem->nil) return -1;

	*len = count;
	return 0;
}

int xp_lisp_comp_symbol (xp_lisp_obj_t* obj, const xp_lisp_char* str)
{
	xp_lisp_char* p;
	xp_size_t index, length;

	xp_lisp_assert (XP_LISP_TYPE(obj) == XP_LISP_OBJ_SYMBOL);
	
	index = 0;
	length = XP_LISP_SYMLEN(obj);

	p = XP_LISP_SYMVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_LISP_CHAR('\0'))? 0: -1;
}

int xp_lisp_comp_symbol2 (xp_lisp_obj_t* obj, const xp_lisp_char* str, xp_size_t len)
{
	xp_lisp_char* p;
	xp_size_t index, length;

	xp_lisp_assert (XP_LISP_TYPE(obj) == XP_LISP_OBJ_SYMBOL);
	
	index = 0;
	length = XP_LISP_SYMLEN(obj);
	p = XP_LISP_SYMVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

int xp_lisp_comp_string (xp_lisp_obj_t* obj, const xp_lisp_char* str)
{
	xp_lisp_char* p;
	xp_size_t index, length;

	xp_lisp_assert (XP_LISP_TYPE(obj) == XP_LISP_OBJ_STRING);
	
	index = 0;
	length = XP_LISP_STRLEN(obj);

	p = XP_LISP_STRVALUE(obj);
	while (index < length) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_LISP_CHAR('\0'))? 0: -1;
}

int xp_lisp_comp_string2 (xp_lisp_obj_t* obj, const xp_lisp_char* str, xp_size_t len)
{
	xp_lisp_char* p;
	xp_size_t index, length;

	xp_lisp_assert (XP_LISP_TYPE(obj) == XP_LISP_OBJ_STRING);
	
	index = 0;
	length = XP_LISP_STRLEN(obj);
	p = XP_LISP_STRVALUE(obj);

	while (index < length && index < len) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (length < len)? -1: 
	       (length > len)?  1: 0;
}

void xp_lisp_copy_string (xp_lisp_char* dst, const xp_lisp_char* str)
{
	// the buffer pointed by dst should be big enough to hold str
	while (*str != XP_LISP_CHAR('\0')) *dst++ = *str++;
	*dst = XP_LISP_CHAR('\0');
}

void xp_lisp_copy_string2 (xp_lisp_char* dst, const xp_lisp_char* str, xp_size_t len)
{
	// the buffer pointed by dst should be big enough to hold str
	while (len > 0) {
		*dst++ = *str++;
		len--;
	}
	*dst = XP_LISP_CHAR('\0');
}

