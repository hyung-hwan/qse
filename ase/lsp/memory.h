/*
 * $Id: memory.h,v 1.3 2005-02-07 15:10:41 bacon Exp $
 */

#ifndef _XP_LISP_MEM_H_
#define _XP_LISP_MEM_H_

#include <xp/lisp/object.h>
#include <xp/lisp/env.h>
#include <xp/lisp/array.h>

struct xp_lisp_mem_t
{
	/* 
	 * object allocation list
	 */
	xp_size_t     ubound;     // upper bounds of the maximum number of objects
	xp_size_t     ubound_inc; // increment of the upper bounds
	xp_size_t     count;      // the number of objects currently allocated
	xp_lisp_obj_t* used[XP_LISP_TYPE_COUNT];
	xp_lisp_obj_t* free[XP_LISP_TYPE_COUNT];
	xp_lisp_obj_t* locked;

	/*
	 * commonly accessed objects 
	 */
	xp_lisp_obj_t* nil;	    // xp_lisp_obj_nil_t
	xp_lisp_obj_t* t;       // xp_lisp_obj_true_t
	xp_lisp_obj_t* quote;   // xp_lisp_obj_symbol_t
	xp_lisp_obj_t* lambda;  // xp_lisp_obj_symbol_t
	xp_lisp_obj_t* macro;   // xp_lisp_obj_symbol_t

	/*
	 * run-time environment frame
	 */
	xp_lisp_frame_t* frame;
	// pointer to a global-level frame
	xp_lisp_frame_t* root_frame;
	// pointer to an interim frame not yet added to "frame"
	xp_lisp_frame_t* brooding_frame; 

	/* 
	 * temporary objects
	 */
	xp_lisp_array_t* temp_array;
};

typedef struct xp_lisp_mem_t xp_lisp_mem_t;

#ifdef __cplusplus
extern "C" {
#endif
	
xp_lisp_mem_t* xp_lisp_mem_new   (xp_size_t ubound, xp_size_t ubound_inc);
void       xp_lisp_mem_free  (xp_lisp_mem_t* mem);

int xp_lisp_add_prims (xp_lisp_mem_t* mem);

xp_lisp_obj_t* xp_lisp_allocate (xp_lisp_mem_t* mem, int type, xp_size_t size);
void       xp_lisp_dispose  (xp_lisp_mem_t* mem, xp_lisp_obj_t* prev, xp_lisp_obj_t* obj);
void       xp_lisp_dispose_all     (xp_lisp_mem_t* mem);
void       xp_lisp_garbage_collect (xp_lisp_mem_t* mem);

void       xp_lisp_lock       (xp_lisp_obj_t* obj);
void       xp_lisp_unlock     (xp_lisp_obj_t* obj);
void       xp_lisp_unlock_all (xp_lisp_obj_t* obj);

// object creation of standard types
xp_lisp_obj_t* xp_lisp_make_nil    (xp_lisp_mem_t* mem);
xp_lisp_obj_t* xp_lisp_make_true   (xp_lisp_mem_t* mem);
xp_lisp_obj_t* xp_lisp_make_int    (xp_lisp_mem_t* mem, xp_lisp_int value);
xp_lisp_obj_t* xp_lisp_make_float  (xp_lisp_mem_t* mem, xp_lisp_float value);
xp_lisp_obj_t* xp_lisp_make_symbol (xp_lisp_mem_t* mem, const xp_char_t* str, xp_size_t len);
xp_lisp_obj_t* xp_lisp_make_string (xp_lisp_mem_t* mem, const xp_char_t* str, xp_size_t len);
xp_lisp_obj_t* xp_lisp_make_cons   (xp_lisp_mem_t* mem, xp_lisp_obj_t* car, xp_lisp_obj_t* cdr);
xp_lisp_obj_t* xp_lisp_make_func   (xp_lisp_mem_t* mem, xp_lisp_obj_t* formal, xp_lisp_obj_t* body);
xp_lisp_obj_t* xp_lisp_make_macro  (xp_lisp_mem_t* mem, xp_lisp_obj_t* formal, xp_lisp_obj_t* body);
xp_lisp_obj_t* xp_lisp_make_prim   (xp_lisp_mem_t* mem, void* impl);

// frame lookup 
xp_lisp_assoc_t* xp_lisp_lookup (xp_lisp_mem_t* mem, xp_lisp_obj_t* name);
xp_lisp_assoc_t* xp_lisp_set    (xp_lisp_mem_t* mem, xp_lisp_obj_t* name, xp_lisp_obj_t* value);

// cons operations
xp_size_t xp_lisp_cons_len   (xp_lisp_mem_t* mem, xp_lisp_obj_t* obj);
int    xp_lisp_probe_args (xp_lisp_mem_t* mem, xp_lisp_obj_t* obj, xp_size_t* len);

// symbol and string operations
int  xp_lisp_comp_symbol  (xp_lisp_obj_t* obj, const xp_char_t* str);
int  xp_lisp_comp_symbol2 (xp_lisp_obj_t* obj, const xp_char_t* str, xp_size_t len);
int  xp_lisp_comp_string  (xp_lisp_obj_t* obj, const xp_char_t* str);
int  xp_lisp_comp_string2 (xp_lisp_obj_t* obj, const xp_char_t* str, xp_size_t len);
void xp_lisp_copy_string  (xp_char_t* dst,  const xp_char_t* str);
void xp_lisp_copy_string2 (xp_char_t* dst,  const xp_char_t* str, xp_size_t len);

#ifdef __cplusplus
}
#endif

#endif
