/*
 * $Id: mem.h,v 1.7 2006-10-22 13:10:46 bacon Exp $
 */

#ifndef _SSE_LSP_MEM_H_
#define _SSE_LSP_MEM_H_

#include <sse/lsp/obj.h>
#include <sse/lsp/env.h>
#include <sse/lsp/array.h>

struct sse_lsp_mem_t
{
	/* 
	 * object allocation list
	 */
	sse_size_t     ubound;     // upper bounds of the maximum number of objects
	sse_size_t     ubound_inc; // increment of the upper bounds
	sse_size_t     count;      // the number of objects currently allocated
	sse_lsp_obj_t* used[SSE_LSP_TYPE_COUNT];
	sse_lsp_obj_t* free[SSE_LSP_TYPE_COUNT];
	sse_lsp_obj_t* locked;

	/*
	 * commonly accessed objects 
	 */
	sse_lsp_obj_t* nil;	    // sse_lsp_obj_nil_t
	sse_lsp_obj_t* t;       // sse_lsp_obj_true_t
	sse_lsp_obj_t* quote;   // sse_lsp_obj_symbol_t
	sse_lsp_obj_t* lambda;  // sse_lsp_obj_symbol_t
	sse_lsp_obj_t* macro;   // sse_lsp_obj_symbol_t

	/*
	 * run-time environment frame
	 */
	sse_lsp_frame_t* frame;
	// pointer to a global-level frame
	sse_lsp_frame_t* root_frame;
	// pointer to an interim frame not yet added to "frame"
	sse_lsp_frame_t* brooding_frame; 

	/* 
	 * temporary objects
	 */
	sse_lsp_array_t* temp_array;
};

typedef struct sse_lsp_mem_t sse_lsp_mem_t;

#ifdef __cplusplus
extern "C" {
#endif
	
sse_lsp_mem_t* sse_lsp_mem_new   (sse_size_t ubound, sse_size_t ubound_inc);
void       sse_lsp_mem_free  (sse_lsp_mem_t* mem);

int sse_lsp_add_builtin_prims (sse_lsp_mem_t* mem);

sse_lsp_obj_t* sse_lsp_alloc (sse_lsp_mem_t* mem, int type, sse_size_t size);
void       sse_lsp_dispose  (sse_lsp_mem_t* mem, sse_lsp_obj_t* prev, sse_lsp_obj_t* obj);
void       sse_lsp_dispose_all     (sse_lsp_mem_t* mem);
void       sse_lsp_garbage_collect (sse_lsp_mem_t* mem);

void       sse_lsp_lock       (sse_lsp_obj_t* obj);
void       sse_lsp_unlock     (sse_lsp_obj_t* obj);
void       sse_lsp_unlock_all (sse_lsp_obj_t* obj);

// object creation of standard types
sse_lsp_obj_t* sse_lsp_make_nil    (sse_lsp_mem_t* mem);
sse_lsp_obj_t* sse_lsp_make_true   (sse_lsp_mem_t* mem);
sse_lsp_obj_t* sse_lsp_make_int    (sse_lsp_mem_t* mem, sse_lsp_int_t value);
sse_lsp_obj_t* sse_lsp_make_real  (sse_lsp_mem_t* mem, sse_lsp_real_t value);

sse_lsp_obj_t* sse_lsp_make_symbol (
	sse_lsp_mem_t* mem, const sse_char_t* str);
sse_lsp_obj_t* sse_lsp_make_symbolx (
	sse_lsp_mem_t* mem, const sse_char_t* str, sse_size_t len);
sse_lsp_obj_t* sse_lsp_make_string (
	sse_lsp_mem_t* mem, const sse_char_t* str);
sse_lsp_obj_t* sse_lsp_make_stringx (
	sse_lsp_mem_t* mem, const sse_char_t* str, sse_size_t len);
sse_lsp_obj_t* sse_lsp_make_cons (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* car, sse_lsp_obj_t* cdr);
sse_lsp_obj_t* sse_lsp_make_func (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* formal, sse_lsp_obj_t* body);
sse_lsp_obj_t* sse_lsp_make_macro (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* formal, sse_lsp_obj_t* body);

sse_lsp_obj_t* sse_lsp_make_prim (sse_lsp_mem_t* mem, void* impl);

// frame lookup 
sse_lsp_assoc_t* sse_lsp_lookup (sse_lsp_mem_t* mem, sse_lsp_obj_t* name);
sse_lsp_assoc_t* sse_lsp_set_value (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* name, sse_lsp_obj_t* value);
sse_lsp_assoc_t* sse_lsp_set_func (
	sse_lsp_mem_t* mem, sse_lsp_obj_t* name, sse_lsp_obj_t* func);

// cons operations
sse_size_t sse_lsp_cons_len (sse_lsp_mem_t* mem, sse_lsp_obj_t* obj);
int sse_lsp_probe_args (sse_lsp_mem_t* mem, sse_lsp_obj_t* obj, sse_size_t* len);

// symbol and string operations
int  sse_lsp_comp_symbol  (sse_lsp_obj_t* obj, const sse_char_t* str);
int  sse_lsp_comp_symbol2 (sse_lsp_obj_t* obj, const sse_char_t* str, sse_size_t len);
int  sse_lsp_comp_string  (sse_lsp_obj_t* obj, const sse_char_t* str);
int  sse_lsp_comp_string2 (sse_lsp_obj_t* obj, const sse_char_t* str, sse_size_t len);
void sse_lsp_copy_string  (sse_char_t* dst,  const sse_char_t* str);
void sse_lsp_copy_string2 (sse_char_t* dst,  const sse_char_t* str, sse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
