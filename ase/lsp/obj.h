/*
 * $Id: obj.h,v 1.4 2005-09-21 15:53:55 bacon Exp $
 */

#ifndef _XP_LSP_OBJ_H_
#define _XP_LSP_OBJ_H_

#include <xp/lsp/types.h>

/* object types */
enum 
{
	XP_LSP_OBJ_NIL = 0,
	XP_LSP_OBJ_TRUE,
	XP_LSP_OBJ_INT,
	XP_LSP_OBJ_REAL,
	XP_LSP_OBJ_SYMBOL,
	XP_LSP_OBJ_STRING,
	XP_LSP_OBJ_CONS,
	XP_LSP_OBJ_FUNC,
	XP_LSP_OBJ_MACRO,
	XP_LSP_OBJ_PRIM,

	XP_LSP_TYPE_COUNT // the number of lsp object types
};

typedef struct xp_lsp_objhdr_t      xp_lsp_objhdr_t;
typedef struct xp_lsp_obj_t         xp_lsp_obj_t;
typedef struct xp_lsp_obj_nil_t     xp_lsp_obj_nil_t;
typedef struct xp_lsp_obj_true_t    xp_lsp_obj_true_t;
typedef struct xp_lsp_obj_int_t     xp_lsp_obj_int_t;
typedef struct xp_lsp_obj_real_t    xp_lsp_obj_real_t;
typedef struct xp_lsp_obj_symbol_t  xp_lsp_obj_symbol_t;
typedef struct xp_lsp_obj_string_t  xp_lsp_obj_string_t;
typedef struct xp_lsp_obj_cons_t    xp_lsp_obj_cons_t;
typedef struct xp_lsp_obj_func_t    xp_lsp_obj_func_t;
typedef struct xp_lsp_obj_macro_t   xp_lsp_obj_macro_t;
typedef struct xp_lsp_obj_prim_t    xp_lsp_obj_prim_t;

struct xp_lsp_objhdr_t
{
	xp_uint32_t   type: 24;
	xp_uint32_t   mark:  4;
	xp_uint32_t   lock:  4;
	xp_size_t     size;
	xp_lsp_obj_t* link;
};

struct xp_lsp_obj_t
{
	xp_lsp_objhdr_t hdr;
};

struct xp_lsp_obj_nil_t
{
	xp_lsp_objhdr_t hdr;
};

struct xp_lsp_obj_true_t
{
	xp_lsp_objhdr_t hdr;
};

struct xp_lsp_obj_int_t
{
	xp_lsp_objhdr_t hdr;
	xp_lsp_int_t value;
};

struct xp_lsp_obj_real_t
{
	xp_lsp_objhdr_t hdr;
	xp_lsp_real_t value;
};

struct xp_lsp_obj_symbol_t
{
	xp_lsp_objhdr_t hdr;
#ifdef __BORLANDC__
#else
	xp_char_t buffer[0];
#endif
};

struct xp_lsp_obj_string_t
{
	xp_lsp_objhdr_t hdr;
#ifdef __BORLANDC__
#else
	xp_char_t buffer[0];
#endif
};

struct xp_lsp_obj_cons_t
{
	xp_lsp_objhdr_t hdr;
	struct xp_lsp_obj_t* car;
	struct xp_lsp_obj_t* cdr;
};

struct xp_lsp_obj_func_t
{
	xp_lsp_objhdr_t hdr;
	struct xp_lsp_obj_t* formal;
	struct xp_lsp_obj_t* body;
};

struct xp_lsp_obj_macro_t
{
	xp_lsp_objhdr_t hdr;
	struct xp_lsp_obj_t* formal;
	struct xp_lsp_obj_t* body;
};

struct xp_lsp_obj_prim_t
{
	xp_lsp_objhdr_t hdr;
	void* impl;  /* xp_lsp_prim_t */
};

/* header access */
#define XP_LSP_TYPE(x)     (((xp_lsp_obj_t*)x)->hdr.type)
#define XP_LSP_SIZE(x)     (((xp_lsp_obj_t*)x)->hdr.size)
#define XP_LSP_MARK(x)     (((xp_lsp_obj_t*)x)->hdr.mark)
#define XP_LSP_LOCK(x)     (((xp_lsp_obj_t*)x)->hdr.lock)
#define XP_LSP_LINK(x)     (((xp_lsp_obj_t*)x)->hdr.link)

/* value access */
#define XP_LSP_IVALUE(x)   (((xp_lsp_obj_int_t*)x)->value)
#define XP_LSP_RVALUE(x)   (((xp_lsp_obj_real_t*)x)->value)

#ifdef __BORLANDC__
#define XP_LSP_SYMVALUE(x) ((xp_char_t*)(((xp_lsp_obj_symbol_t*)x) + 1))
#else
#define XP_LSP_SYMVALUE(x) (((xp_lsp_obj_symbol_t*)x)->buffer)
#endif
#define XP_LSP_SYMLEN(x)   ((((xp_lsp_obj_symbol_t*)x)->hdr.size - sizeof(xp_lsp_obj_t)) / sizeof(xp_char_t) - 1)

#ifdef __BORLANDC__
#define XP_LSP_STRVALUE(x) ((xp_char_t*)(((xp_lsp_obj_string_t*)x) + 1))
#else
#define XP_LSP_STRVALUE(x) (((xp_lsp_obj_string_t*)x)->buffer)
#endif
#define XP_LSP_STRLEN(x)   ((((xp_lsp_obj_string_t*)x)->hdr.size - sizeof(xp_lsp_obj_t)) / sizeof(xp_char_t) - 1)

#define XP_LSP_CAR(x)      (((xp_lsp_obj_cons_t*)x)->car)
#define XP_LSP_CDR(x)      (((xp_lsp_obj_cons_t*)x)->cdr)
#define XP_LSP_FFORMAL(x)  (((xp_lsp_obj_func_t*)x)->formal)
#define XP_LSP_FBODY(x)    (((xp_lsp_obj_func_t*)x)->body)
#define XP_LSP_MFORMAL(x)  (((xp_lsp_obj_macro_t*)x)->formal)
#define XP_LSP_MBODY(x)    (((xp_lsp_obj_macro_t*)x)->body)
#define XP_LSP_PRIM(x)     ((xp_lsp_prim_t)(((xp_lsp_obj_prim_t*)x)->impl))

#endif
