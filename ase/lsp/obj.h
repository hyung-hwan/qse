/*
 * $Id: obj.h,v 1.1 2005-09-18 11:54:23 bacon Exp $
 */

#ifndef _XP_LSP_OBJ_H_
#define _XP_LSP_OBJ_H_

#include <xp/lsp/types.h>

// object types
enum 
{
	XP_LSP_OBJ_NIL = 0,
	XP_LSP_OBJ_TRUE,
	XP_LSP_OBJ_INT,
	XP_LSP_OBJ_FLOAT,
	XP_LSP_OBJ_SYMBOL,
	XP_LSP_OBJ_STRING,
	XP_LSP_OBJ_CONS,
	XP_LSP_OBJ_FUNC,
	XP_LSP_OBJ_MACRO,
	XP_LSP_OBJ_PRIM,

	XP_LSP_TYPE_COUNT // the number of lsp object types
};

#define XP_LSP_OBJ_HEADER  \
	xp_uint32_t type: 24; \
	xp_uint32_t mark:  4; \
	xp_uint32_t lock:  4; \
	xp_size_t    size;     \
	struct    xp_lsp_obj_t* link

struct xp_lsp_obj_t
{
	XP_LSP_OBJ_HEADER;
};

struct xp_lsp_obj_nil_t
{
	XP_LSP_OBJ_HEADER;
};

struct xp_lsp_obj_true_t
{
	XP_LSP_OBJ_HEADER;
};

struct xp_lsp_obj_int_t
{
	XP_LSP_OBJ_HEADER;
	xp_lsp_int_t value;
};

struct xp_lsp_obj_float_t
{
	XP_LSP_OBJ_HEADER;
	xp_lsp_real_t value;
};

struct xp_lsp_obj_symbol_t
{
	XP_LSP_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_char_t buffer[0];
#endif
};

struct xp_lsp_obj_string_t
{
	XP_LSP_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_char_t buffer[0];
#endif
};

struct xp_lsp_obj_cons_t
{
	XP_LSP_OBJ_HEADER;
	struct xp_lsp_obj_t* car;
	struct xp_lsp_obj_t* cdr;
};

struct xp_lsp_obj_func_t
{
	XP_LSP_OBJ_HEADER;
	struct xp_lsp_obj_t* formal;
	struct xp_lsp_obj_t* body;
};

struct xp_lsp_obj_macro_t
{
	XP_LSP_OBJ_HEADER;
	struct xp_lsp_obj_t* formal;
	struct xp_lsp_obj_t* body;
};

struct xp_lsp_obj_prim_t
{
	XP_LSP_OBJ_HEADER;
	void* impl; // xp_lsp_prim_t
};

typedef struct xp_lsp_obj_t         xp_lsp_obj_t;
typedef struct xp_lsp_obj_nil_t     xp_lsp_obj_nil_t;
typedef struct xp_lsp_obj_true_t    xp_lsp_obj_true_t;
typedef struct xp_lsp_obj_int_t     xp_lsp_obj_int_t;
typedef struct xp_lsp_obj_float_t   xp_lsp_obj_float_t;
typedef struct xp_lsp_obj_symbol_t  xp_lsp_obj_symbol_t;
typedef struct xp_lsp_obj_string_t  xp_lsp_obj_string_t;
typedef struct xp_lsp_obj_cons_t    xp_lsp_obj_cons_t;
typedef struct xp_lsp_obj_func_t    xp_lsp_obj_func_t;
typedef struct xp_lsp_obj_macro_t   xp_lsp_obj_macro_t;
typedef struct xp_lsp_obj_prim_t    xp_lsp_obj_prim_t;

// header access
#define XP_LSP_TYPE(x)     (((xp_lsp_obj_t*)x)->type)
#define XP_LSP_SIZE(x)     (((xp_lsp_obj_t*)x)->size)
#define XP_LSP_MARK(x)     (((xp_lsp_obj_t*)x)->mark)
#define XP_LSP_LOCK(x)     (((xp_lsp_obj_t*)x)->lock)
#define XP_LSP_LINK(x)     (((xp_lsp_obj_t*)x)->link)

// value access
#define XP_LSP_IVALUE(x)   (((xp_lsp_obj_int_t*)x)->value)
#define XP_LSP_FVALUE(x)   (((xp_lsp_obj_float_t*)x)->value)

#ifdef __BORLANDC__
#define XP_LSP_SYMVALUE(x) ((xp_char_t*)(((xp_lsp_obj_symbol_t*)x) + 1))
#else
#define XP_LSP_SYMVALUE(x) (((xp_lsp_obj_symbol_t*)x)->buffer)
#endif
#define XP_LSP_SYMLEN(x)   ((((xp_lsp_obj_symbol_t*)x)->size - sizeof(xp_lsp_obj_t)) / sizeof(xp_char_t) - 1)

#ifdef __BORLANDC__
#define XP_LSP_STRVALUE(x) ((xp_char_t*)(((xp_lsp_obj_string_t*)x) + 1))
#else
#define XP_LSP_STRVALUE(x) (((xp_lsp_obj_string_t*)x)->buffer)
#endif
#define XP_LSP_STRLEN(x)   ((((xp_lsp_obj_string_t*)x)->size - sizeof(xp_lsp_obj_t)) / sizeof(xp_char_t) - 1)

#define XP_LSP_CAR(x)      (((xp_lsp_obj_cons_t*)x)->car)
#define XP_LSP_CDR(x)      (((xp_lsp_obj_cons_t*)x)->cdr)
#define XP_LSP_FFORMAL(x)  (((xp_lsp_obj_func_t*)x)->formal)
#define XP_LSP_FBODY(x)    (((xp_lsp_obj_func_t*)x)->body)
#define XP_LSP_MFORMAL(x)  (((xp_lsp_obj_macro_t*)x)->formal)
#define XP_LSP_MBODY(x)    (((xp_lsp_obj_macro_t*)x)->body)
#define XP_LSP_PIMPL(x)    ((xp_lsp_pimpl_t)(((xp_lsp_obj_prim_t*)x)->impl))

#endif
