/*
 * $Id: mem.h,v 1.3 2007/04/30 06:09:46 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_LSP_MEM_H_
#define _ASE_LSP_MEM_H_

#ifndef _ASE_LSP_LSP_H_
#error Never include this file directly. Include <ase/lsp/lsp.h> instead
#endif

typedef struct ase_lsp_mem_t ase_lsp_mem_t;

struct ase_lsp_mem_t
{
	ase_lsp_t* lsp;

	/* object allocation list */
	ase_size_t ubound; /* upper bounds of the maximum number of objects */
	ase_size_t ubound_inc; /* increment of the upper bounds */
	ase_size_t count;  /* the number of objects currently allocated */
	ase_lsp_obj_t* used[ASE_LSP_TYPE_COUNT];
	ase_lsp_obj_t* free[ASE_LSP_TYPE_COUNT];
	ase_lsp_obj_t* read;

	/* commonly accessed objects */
	ase_lsp_obj_t* nil;     /* ase_lsp_obj_nil_t */
	ase_lsp_obj_t* t;       /* ase_lsp_obj_true_t */
	ase_lsp_obj_t* quote;   /* ase_lsp_obj_sym_t */
	ase_lsp_obj_t* lambda;  /* ase_lsp_obj_sym_t */
	ase_lsp_obj_t* macro;   /* ase_lsp_obj_sym_t */

	/* run-time environment frame */
	ase_lsp_frame_t* frame;
	/* pointer to a global-level frame */
	ase_lsp_frame_t* root_frame;
	/* pointer to an interim frame not yet added to "frame" */
	ase_lsp_frame_t* brooding_frame; 

	/* links for temporary objects */
	ase_lsp_tlink_t* tlink;
	ase_size_t tlink_count;
};

#ifdef __cplusplus
extern "C" {
#endif
	
ase_lsp_mem_t* ase_lsp_openmem (
	ase_lsp_t* lsp, ase_size_t ubound, ase_size_t ubound_inc);
void ase_lsp_closemem (ase_lsp_mem_t* mem);

ase_lsp_obj_t* ase_lsp_alloc (ase_lsp_mem_t* mem, int type, ase_size_t size);
void ase_lsp_dispose  (ase_lsp_mem_t* mem, ase_lsp_obj_t* prev, ase_lsp_obj_t* obj);
void ase_lsp_dispose_all (ase_lsp_mem_t* mem);
void ase_lsp_gc (ase_lsp_mem_t* mem);

void ase_lsp_lockobj (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
void ase_lsp_unlockobj (ase_lsp_t* lsp, ase_lsp_obj_t* obj);
void ase_lsp_deepunlockobj (ase_lsp_t* lsp, ase_lsp_obj_t* obj);

/* object creation of standard types */
ase_lsp_obj_t* ase_lsp_makenil     (ase_lsp_mem_t* mem);
ase_lsp_obj_t* ase_lsp_maketrue    (ase_lsp_mem_t* mem);
ase_lsp_obj_t* ase_lsp_makeintobj  (ase_lsp_mem_t* mem, ase_long_t value);
ase_lsp_obj_t* ase_lsp_makerealobj (ase_lsp_mem_t* mem, ase_real_t value);

ase_lsp_obj_t* ase_lsp_makesym (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len);
ase_lsp_obj_t* ase_lsp_makestr (
	ase_lsp_mem_t* mem, const ase_char_t* str, ase_size_t len);

ase_lsp_obj_t* ase_lsp_makecons (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* car, ase_lsp_obj_t* cdr);
ase_lsp_obj_t* ase_lsp_makefunc (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body);
ase_lsp_obj_t* ase_lsp_makemacro (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* formal, ase_lsp_obj_t* body);

ase_lsp_obj_t* ase_lsp_makeprim (ase_lsp_mem_t* mem, 
	ase_lsp_prim_t impl, ase_size_t min_args, ase_size_t max_args);

/* frame lookup */
ase_lsp_assoc_t* ase_lsp_lookup (ase_lsp_mem_t* mem, ase_lsp_obj_t* name);
ase_lsp_assoc_t* ase_lsp_setvalue (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* value);
ase_lsp_assoc_t* ase_lsp_setfunc (
	ase_lsp_mem_t* mem, ase_lsp_obj_t* name, ase_lsp_obj_t* func);

/* cons operations */
ase_size_t ase_lsp_conslen (ase_lsp_mem_t* mem, ase_lsp_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif
