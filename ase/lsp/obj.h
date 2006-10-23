/*
 * $Id: obj.h,v 1.6 2006-10-23 14:42:38 bacon Exp $
 */

#ifndef _SSE_LSP_OBJ_H_
#define _SSE_LSP_OBJ_H_

#include <sse/lsp/types.h>

/* object types */
enum 
{
	SSE_LSP_OBJ_NIL = 0,
	SSE_LSP_OBJ_TRUE,
	SSE_LSP_OBJ_INT,
	SSE_LSP_OBJ_REAL,
	SSE_LSP_OBJ_SYMBOL,
	SSE_LSP_OBJ_STRING,
	SSE_LSP_OBJ_CONS,
	SSE_LSP_OBJ_FUNC,
	SSE_LSP_OBJ_MACRO,
	SSE_LSP_OBJ_PRIM,

	SSE_LSP_TYPE_COUNT // the number of lsp object types
};

typedef struct sse_lsp_objhdr_t      sse_lsp_objhdr_t;
typedef struct sse_lsp_obj_t         sse_lsp_obj_t;
typedef struct sse_lsp_obj_nil_t     sse_lsp_obj_nil_t;
typedef struct sse_lsp_obj_true_t    sse_lsp_obj_true_t;
typedef struct sse_lsp_obj_int_t     sse_lsp_obj_int_t;
typedef struct sse_lsp_obj_real_t    sse_lsp_obj_real_t;
typedef struct sse_lsp_obj_symbol_t  sse_lsp_obj_symbol_t;
typedef struct sse_lsp_obj_string_t  sse_lsp_obj_string_t;
typedef struct sse_lsp_obj_cons_t    sse_lsp_obj_cons_t;
typedef struct sse_lsp_obj_func_t    sse_lsp_obj_func_t;
typedef struct sse_lsp_obj_macro_t   sse_lsp_obj_macro_t;
typedef struct sse_lsp_obj_prim_t    sse_lsp_obj_prim_t;

struct sse_lsp_objhdr_t
{
	sse_uint32_t   type: 24;
	sse_uint32_t   mark:  4;
	sse_uint32_t   lock:  4;
	sse_size_t     size;
	sse_lsp_obj_t* link;
};

struct sse_lsp_obj_t
{
	sse_lsp_objhdr_t hdr;
};

struct sse_lsp_obj_nil_t
{
	sse_lsp_objhdr_t hdr;
};

struct sse_lsp_obj_true_t
{
	sse_lsp_objhdr_t hdr;
};

struct sse_lsp_obj_int_t
{
	sse_lsp_objhdr_t hdr;
	sse_lsp_int_t value;
};

struct sse_lsp_obj_real_t
{
	sse_lsp_objhdr_t hdr;
	sse_lsp_real_t value;
};

struct sse_lsp_obj_symbol_t
{
	sse_lsp_objhdr_t hdr;
#if defined(__BORLANDC__) || defined(_MSC_VER)
#else
	sse_char_t buffer[0];
#endif
};

struct sse_lsp_obj_string_t
{
	sse_lsp_objhdr_t hdr;
#if defined(__BORLANDC__) || defined(_MSC_VER)
#else
	sse_char_t buffer[0];
#endif
};

struct sse_lsp_obj_cons_t
{
	sse_lsp_objhdr_t hdr;
	struct sse_lsp_obj_t* car;
	struct sse_lsp_obj_t* cdr;
};

struct sse_lsp_obj_func_t
{
	sse_lsp_objhdr_t hdr;
	struct sse_lsp_obj_t* formal;
	struct sse_lsp_obj_t* body;
};

struct sse_lsp_obj_macro_t
{
	sse_lsp_objhdr_t hdr;
	struct sse_lsp_obj_t* formal;
	struct sse_lsp_obj_t* body;
};

struct sse_lsp_obj_prim_t
{
	sse_lsp_objhdr_t hdr;
	void* impl;  /* sse_lsp_prim_t */
};

/* header access */
#define SSE_LSP_TYPE(x)     (((sse_lsp_obj_t*)x)->hdr.type)
#define SSE_LSP_SIZE(x)     (((sse_lsp_obj_t*)x)->hdr.size)
#define SSE_LSP_MARK(x)     (((sse_lsp_obj_t*)x)->hdr.mark)
#define SSE_LSP_LOCK(x)     (((sse_lsp_obj_t*)x)->hdr.lock)
#define SSE_LSP_LINK(x)     (((sse_lsp_obj_t*)x)->hdr.link)

/* value access */
#define SSE_LSP_IVALUE(x)   (((sse_lsp_obj_int_t*)x)->value)
#define SSE_LSP_RVALUE(x)   (((sse_lsp_obj_real_t*)x)->value)

#ifdef __BORLANDC__
#define SSE_LSP_SYMVALUE(x) ((sse_char_t*)(((sse_lsp_obj_symbol_t*)x) + 1))
#else
#define SSE_LSP_SYMVALUE(x) (((sse_lsp_obj_symbol_t*)x)->buffer)
#endif
#define SSE_LSP_SYMLEN(x)   ((((sse_lsp_obj_symbol_t*)x)->hdr.size - sizeof(sse_lsp_obj_t)) / sizeof(sse_char_t) - 1)

#ifdef __BORLANDC__
#define SSE_LSP_STRVALUE(x) ((sse_char_t*)(((sse_lsp_obj_string_t*)x) + 1))
#else
#define SSE_LSP_STRVALUE(x) (((sse_lsp_obj_string_t*)x)->buffer)
#endif
#define SSE_LSP_STRLEN(x)   ((((sse_lsp_obj_string_t*)x)->hdr.size - sizeof(sse_lsp_obj_t)) / sizeof(sse_char_t) - 1)

#define SSE_LSP_CAR(x)      (((sse_lsp_obj_cons_t*)x)->car)
#define SSE_LSP_CDR(x)      (((sse_lsp_obj_cons_t*)x)->cdr)
#define SSE_LSP_FFORMAL(x)  (((sse_lsp_obj_func_t*)x)->formal)
#define SSE_LSP_FBODY(x)    (((sse_lsp_obj_func_t*)x)->body)
#define SSE_LSP_MFORMAL(x)  (((sse_lsp_obj_macro_t*)x)->formal)
#define SSE_LSP_MBODY(x)    (((sse_lsp_obj_macro_t*)x)->body)
#define SSE_LSP_PRIM(x)     ((sse_lsp_prim_t)(((sse_lsp_obj_prim_t*)x)->impl))

#endif
