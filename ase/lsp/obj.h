/*
 * $Id: obj.h,v 1.14 2006-11-02 06:46:31 bacon Exp $
 */

#ifndef _ASE_LSP_OBJ_H_
#define _ASE_LSP_OBJ_H_

#ifndef _ASE_LSP_LSP_H_
#error Never include this file directly. Include <ase/lsp/lsp.h> instead
#endif

/* object types */
enum 
{
	ASE_LSP_OBJ_NIL = 0,
	ASE_LSP_OBJ_TRUE,
	ASE_LSP_OBJ_INT,
	ASE_LSP_OBJ_REAL,
	ASE_LSP_OBJ_SYM,
	ASE_LSP_OBJ_STR,
	ASE_LSP_OBJ_CONS,
	ASE_LSP_OBJ_FUNC,
	ASE_LSP_OBJ_MACRO,
	ASE_LSP_OBJ_PRIM,

	ASE_LSP_TYPE_COUNT // the number of lsp object types
};

typedef struct ase_lsp_objhdr_t     ase_lsp_objhdr_t;
typedef struct ase_lsp_obj_nil_t    ase_lsp_obj_nil_t;
typedef struct ase_lsp_obj_true_t   ase_lsp_obj_true_t;
typedef struct ase_lsp_obj_int_t    ase_lsp_obj_int_t;
typedef struct ase_lsp_obj_real_t   ase_lsp_obj_real_t;
typedef struct ase_lsp_obj_sym_t    ase_lsp_obj_sym_t;
typedef struct ase_lsp_obj_str_t    ase_lsp_obj_str_t;
typedef struct ase_lsp_obj_cons_t   ase_lsp_obj_cons_t;
typedef struct ase_lsp_obj_func_t   ase_lsp_obj_func_t;
typedef struct ase_lsp_obj_macro_t  ase_lsp_obj_macro_t;
typedef struct ase_lsp_obj_prim_t   ase_lsp_obj_prim_t;

struct ase_lsp_objhdr_t
{
	ase_uint32_t   type:  8;
	ase_uint32_t   mark:  8;
	ase_uint32_t   lock: 16;
	ase_size_t     size;
	ase_lsp_obj_t* link;
};

struct ase_lsp_obj_t
{
	ase_lsp_objhdr_t hdr;
};

struct ase_lsp_obj_nil_t
{
	ase_lsp_objhdr_t hdr;
};

struct ase_lsp_obj_true_t
{
	ase_lsp_objhdr_t hdr;
};

struct ase_lsp_obj_int_t
{
	ase_lsp_objhdr_t hdr;
	ase_long_t value;
};

struct ase_lsp_obj_real_t
{
	ase_lsp_objhdr_t hdr;
	ase_real_t value;
};

struct ase_lsp_obj_sym_t
{
	ase_lsp_objhdr_t hdr;
#if !defined(__BORLANDC__) && !defined(_MSC_VER)
	ase_char_t buffer[0];
#endif
};

struct ase_lsp_obj_str_t
{
	ase_lsp_objhdr_t hdr;
#if !defined(__BORLANDC__) && !defined(_MSC_VER)
	ase_char_t buffer[0];
#endif
};

struct ase_lsp_obj_cons_t
{
	ase_lsp_objhdr_t hdr;
	ase_lsp_obj_t* car;
	ase_lsp_obj_t* cdr;
};

struct ase_lsp_obj_func_t
{
	ase_lsp_objhdr_t hdr;
	ase_lsp_obj_t* formal;
	ase_lsp_obj_t* body;
};

struct ase_lsp_obj_macro_t
{
	ase_lsp_objhdr_t hdr;
	ase_lsp_obj_t* formal;
	ase_lsp_obj_t* body;
};

struct ase_lsp_obj_prim_t
{
	ase_lsp_objhdr_t hdr;
	ase_lsp_prim_t impl;
	ase_size_t min_args;
	ase_size_t max_args;
};

/* header access */
#define ASE_LSP_TYPE(x) (((ase_lsp_obj_t*)x)->hdr.type)
#define ASE_LSP_SIZE(x) (((ase_lsp_obj_t*)x)->hdr.size)
#define ASE_LSP_MARK(x) (((ase_lsp_obj_t*)x)->hdr.mark)
#define ASE_LSP_LOCK(x) (((ase_lsp_obj_t*)x)->hdr.lock)
#define ASE_LSP_LINK(x) (((ase_lsp_obj_t*)x)->hdr.link)

/* value access */
#define ASE_LSP_IVAL(x) (((ase_lsp_obj_int_t*)x)->value)
#define ASE_LSP_RVAL(x) (((ase_lsp_obj_real_t*)x)->value)

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define ASE_LSP_SYMPTR(x) ((ase_char_t*)(((ase_lsp_obj_sym_t*)x) + 1))
#else
#define ASE_LSP_SYMPTR(x) (((ase_lsp_obj_sym_t*)x)->buffer)
#endif
#define ASE_LSP_SYMLEN(x) ((((ase_lsp_obj_sym_t*)x)->hdr.size - sizeof(ase_lsp_obj_t)) / sizeof(ase_char_t) - 1)

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define ASE_LSP_STRPTR(x) ((ase_char_t*)(((ase_lsp_obj_str_t*)x) + 1))
#else
#define ASE_LSP_STRPTR(x) (((ase_lsp_obj_str_t*)x)->buffer)
#endif
#define ASE_LSP_STRLEN(x) ((((ase_lsp_obj_str_t*)x)->hdr.size - sizeof(ase_lsp_obj_t)) / sizeof(ase_char_t) - 1)

#define ASE_LSP_CAR(x)      (((ase_lsp_obj_cons_t*)x)->car)
#define ASE_LSP_CDR(x)      (((ase_lsp_obj_cons_t*)x)->cdr)
#define ASE_LSP_FFORMAL(x)  (((ase_lsp_obj_func_t*)x)->formal)
#define ASE_LSP_FBODY(x)    (((ase_lsp_obj_func_t*)x)->body)
#define ASE_LSP_MFORMAL(x)  (((ase_lsp_obj_macro_t*)x)->formal)
#define ASE_LSP_MBODY(x)    (((ase_lsp_obj_macro_t*)x)->body)
#define ASE_LSP_PIMPL(x)    (((ase_lsp_obj_prim_t*)x)->impl)
#define ASE_LSP_PMINARGS(x) (((ase_lsp_obj_prim_t*)x)->min_args)
#define ASE_LSP_PMAXARGS(x) (((ase_lsp_obj_prim_t*)x)->max_args)

#endif
