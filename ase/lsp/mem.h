/*
 * $Id: mem.h,v 1.3 2005-09-19 03:05:37 bacon Exp $
 */

#ifndef _XP_LSP_MEM_H_
#define _XP_LSP_MEM_H_

#include <xp/lsp/obj.h>
#include <xp/lsp/env.h>
#include <xp/lsp/array.h>

struct xp_lsp_mem_t
{
	/* 
	 * object allocation list
	 */
	xp_size_t     ubound;     // upper bounds of the maximum number of objects
	xp_size_t     ubound_inc; // increment of the upper bounds
	xp_size_t     count;      // the number of objects currently allocated
	xp_lsp_obj_t* used[XP_LSP_TYPE_COUNT];
	xp_lsp_obj_t* free[XP_LSP_TYPE_COUNT];
	xp_lsp_obj_t* locked;

	/*
	 * commonly accessed objects 
	 */
	xp_lsp_obj_t* nil;	    // xp_lsp_obj_nil_t
	xp_lsp_obj_t* t;       // xp_lsp_obj_true_t
	xp_lsp_obj_t* quote;   // xp_lsp_obj_symbol_t
	xp_lsp_obj_t* lambda;  // xp_lsp_obj_symbol_t
	xp_lsp_obj_t* macro;   // xp_lsp_obj_symbol_t

	/*
	 * run-time environment frame
	 */
	xp_lsp_frame_t* frame;
	// pointer to a global-level frame
	xp_lsp_frame_t* root_frame;
	// pointer to an interim frame not yet added to "frame"
	xp_lsp_frame_t* brooding_frame; 

	/* 
	 * temporary objects
	 */
	xp_lsp_array_t* temp_array;
};

typedef struct xp_lsp_mem_t xp_lsp_mem_t;

#ifdef __cplusplus
extern "C" {
#endif
	
xp_lsp_mem_t* xp_lsp_mem_new   (xp_size_t ubound, xp_size_t ubound_inc);
void       xp_lsp_mem_free  (xp_lsp_mem_t* mem);

int xp_lsp_add_builtin_prims (xp_lsp_mem_t* mem);

xp_lsp_obj_t* xp_lsp_allocate (xp_lsp_mem_t* mem, int type, xp_size_t size);
void       xp_lsp_dispose  (xp_lsp_mem_t* mem, xp_lsp_obj_t* prev, xp_lsp_obj_t* obj);
void       xp_lsp_dispose_all     (xp_lsp_mem_t* mem);
void       xp_lsp_garbage_collect (xp_lsp_mem_t* mem);

void       xp_lsp_lock       (xp_lsp_obj_t* obj);
void       xp_lsp_unlock     (xp_lsp_obj_t* obj);
void       xp_lsp_unlock_all (xp_lsp_obj_t* obj);

// object creation of standard types
xp_lsp_obj_t* xp_lsp_make_nil    (xp_lsp_mem_t* mem);
xp_lsp_obj_t* xp_lsp_make_true   (xp_lsp_mem_t* mem);
xp_lsp_obj_t* xp_lsp_make_int    (xp_lsp_mem_t* mem, xp_lsp_int_t value);
xp_lsp_obj_t* xp_lsp_make_float  (xp_lsp_mem_t* mem, xp_lsp_real_t value);
xp_lsp_obj_t* xp_lsp_make_symbol (xp_lsp_mem_t* mem, const xp_char_t* str, xp_size_t len);
xp_lsp_obj_t* xp_lsp_make_string (xp_lsp_mem_t* mem, const xp_char_t* str, xp_size_t len);
xp_lsp_obj_t* xp_lsp_make_cons   (xp_lsp_mem_t* mem, xp_lsp_obj_t* car, xp_lsp_obj_t* cdr);
xp_lsp_obj_t* xp_lsp_make_func   (xp_lsp_mem_t* mem, xp_lsp_obj_t* formal, xp_lsp_obj_t* body);
xp_lsp_obj_t* xp_lsp_make_macro  (xp_lsp_mem_t* mem, xp_lsp_obj_t* formal, xp_lsp_obj_t* body);
xp_lsp_obj_t* xp_lsp_make_prim   (xp_lsp_mem_t* mem, void* impl);

// frame lookup 
xp_lsp_assoc_t* xp_lsp_lookup (xp_lsp_mem_t* mem, xp_lsp_obj_t* name);
xp_lsp_assoc_t* xp_lsp_set    (xp_lsp_mem_t* mem, xp_lsp_obj_t* name, xp_lsp_obj_t* value);

// cons operations
xp_size_t xp_lsp_cons_len   (xp_lsp_mem_t* mem, xp_lsp_obj_t* obj);
int    xp_lsp_probe_args (xp_lsp_mem_t* mem, xp_lsp_obj_t* obj, xp_size_t* len);

// symbol and string operations
int  xp_lsp_comp_symbol  (xp_lsp_obj_t* obj, const xp_char_t* str);
int  xp_lsp_comp_symbol2 (xp_lsp_obj_t* obj, const xp_char_t* str, xp_size_t len);
int  xp_lsp_comp_string  (xp_lsp_obj_t* obj, const xp_char_t* str);
int  xp_lsp_comp_string2 (xp_lsp_obj_t* obj, const xp_char_t* str, xp_size_t len);
void xp_lsp_copy_string  (xp_char_t* dst,  const xp_char_t* str);
void xp_lsp_copy_string2 (xp_char_t* dst,  const xp_char_t* str, xp_size_t len);

#ifdef __cplusplus
}
#endif

#endif
