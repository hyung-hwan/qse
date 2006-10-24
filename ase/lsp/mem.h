/*
 * $Id: mem.h,v 1.10 2006-10-24 15:31:35 bacon Exp $
 */

#ifndef _ASE_LSP_MEM_H_
#define _ASE_LSP_MEM_H_

#include <ase/lsp/obj.h>
#include <ase/lsp/env.h>
#include <ase/lsp/array.h>

typedef struct ase_lsp_mem_t ase_lsp_mem_t;

struct ase_lsp_mem_t
{
	ase_lsp_t* lsp;

	/* 
	 * object allocation list
	 */
	ase_size_t     ubound;     // upper bounds of the maximum number of objects
	ase_size_t     ubound_inc; // increment of the upper bounds
	ase_size_t     count;      // the number of objects currently allocated
	ase_lsp_obj_t* used[ASE_LSP_TYPE_COUNT];
	ase_lsp_obj_t* free[ASE_LSP_TYPE_COUNT];
	ase_lsp_obj_t* locked;

	/*
	 * commonly accessed objects 
	 */
	ase_lsp_obj_t* nil;	    // ase_lsp_obj_nil_t
	ase_lsp_obj_t* t;       // ase_lsp_obj_true_t
	ase_lsp_obj_t* quote;   // ase_lsp_obj_symbol_t
	ase_lsp_obj_t* lambda;  // ase_lsp_obj_symbol_t
	ase_lsp_obj_t* macro;   // ase_lsp_obj_symbol_t

	/*
	 * run-time environment frame
	 */
	ase_lsp_frame_t* frame;
	// pointer to a global-level frame
	ase_lsp_frame_t* root_frame;
	// pointer to an interim frame not yet added to "frame"
	ase_lsp_frame_t* brooding_frame; 

	/* 
	 * temporary objects
	 */
	ase_lsp_array_t* temp_array;
};


#ifdef __cplusplus
extern "C" {
#endif
	
ase_lsp_mem_t* ase_lsp_openmem (
	ase_lsp_t* lsp, ase_size_t ubound, ase_size_t ubound_inc);
void ase_lsp_closemem (ase_lsp_mem_t* mem);

int ase_lsp_add_builtin_prims (ase_lsp_mem_t* mem);

ase_lsp_obj_t* ase_lsp_alloc (ase_lsp_mem_t* mem, int type, ase_size_t size);
void ase_lsp_dispose  (ase_lsp_mem_t* mem, ase_lsp_obj_t* prev, ase_lsp_obj_t* obj);
void ase_lsp_dispose_all (ase_lsp_mem_t* mem);
void ase_lsp_collectgarbage (ase_lsp_mem_t* mem);

void ase_lsp_lockobj (ase_lsp_obj_t* obj);
void ase_lsp_unlockobj (ase_lsp_obj_t* obj);
void ase_lsp_unlockallobjs (ase_lsp_obj_t* obj);

// object creation of standard types
ase_lsp_obj_t* ase_lsp_make_nil    (ase_lsp_mem_t* mem);
ase_lsp_obj_t* ase_lsp_make_true   (ase_lsp_mem_t* mem);
ase_lsp_obj_t* ase_lsp_make_int    (ase_lsp_mem_t* mem, ase_lsp_int_t value);
ase_lsp_obj_t* ase_lsp_make_real  (ase_lsp_mem_t* mem, ase_lsp_real_t value);

ase_lsp_obj_t* ase_lsp_make_symbol (
	ase_lsp_mem_t* mem, const ase_char_t* str);
ase_lsp_obj_t* ase_lsp_make_symbolx (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len);
ase_lsp_obj_t* ase_lsp_make_string (
	ase_lsp_mem_t* mem, const ase_char_t* str);
ase_lsp_obj_t* ase_lsp_make_stringx (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len);
ase_lsp_obj_t* ase_lsp_make_cons (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* car, ase_lsp_obj_t* cdr);
ase_lsp_obj_t* ase_lsp_make_func (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body);
ase_lsp_obj_t* ase_lsp_make_macro (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body);

ase_lsp_obj_t* ase_lsp_make_prim (ase_lsp_mem_t* mem, void* impl);

// frame lookup 
ase_lsp_assoc_t* ase_lsp_lookup (ase_lsp_mem_t* mem, ase_lsp_obj_t* name);
ase_lsp_assoc_t* ase_lsp_set_value (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* value);
ase_lsp_assoc_t* ase_lsp_set_func (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* func);

// cons operations
ase_size_t ase_lsp_cons_len (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj);
int ase_lsp_probe_args (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj, ase_size_t* len);

// symbol and string operations
int  ase_lsp_comp_symbol  (ase_lsp_obj_t* obj, const ase_char_t* str);
int  ase_lsp_comp_symbol2 (ase_lsp_obj_t* obj, const ase_char_t* str, ase_size_t len);
int  ase_lsp_comp_string  (ase_lsp_obj_t* obj, const ase_char_t* str);
int  ase_lsp_comp_string2 (ase_lsp_obj_t* obj, const ase_char_t* str, ase_size_t len);
void ase_lsp_copy_string  (ase_char_t* dst,  const ase_char_t* str);
void ase_lsp_copy_string2 (ase_char_t* dst,  const ase_char_t* str, ase_size_t len);

#ifdef __cplusplus
}
#endif

#endif
