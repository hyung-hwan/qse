/*
 * $Id: object.h,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#ifndef _XP_LISP_OBJECT_H_
#define _XP_LISP_OBJECT_H_

#include <xp/lisp/types.h>

// object types
enum 
{
	XP_LISP_OBJ_NIL = 0,
	XP_LISP_OBJ_TRUE,
	XP_LISP_OBJ_INT,
	XP_LISP_OBJ_FLOAT,
	XP_LISP_OBJ_SYMBOL,
	XP_LISP_OBJ_STRING,
	XP_LISP_OBJ_CONS,
	XP_LISP_OBJ_FUNC,
	XP_LISP_OBJ_MACRO,
	XP_LISP_OBJ_PRIM,

	XP_LISP_TYPE_COUNT // the number of lisp object types
};

#define XP_LISP_OBJ_HEADER  \
	xp_uint32_t type: 24; \
	xp_uint32_t mark:  4; \
	xp_uint32_t lock:  4; \
	xp_size_t    size;     \
	struct    xp_lisp_obj_t* link

struct xp_lisp_obj_t
{
	XP_LISP_OBJ_HEADER;
};

struct xp_lisp_obj_nil_t
{
	XP_LISP_OBJ_HEADER;
};

struct xp_lisp_obj_true_t
{
	XP_LISP_OBJ_HEADER;
};

struct xp_lisp_obj_int_t
{
	XP_LISP_OBJ_HEADER;
	xp_lisp_int value;
};

struct xp_lisp_obj_float_t
{
	XP_LISP_OBJ_HEADER;
	xp_lisp_float value;
};

struct xp_lisp_obj_symbol_t
{
	XP_LISP_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_lisp_char buffer[0];
#endif
};

struct xp_lisp_obj_string_t
{
	XP_LISP_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_lisp_char buffer[0];
#endif
};

struct xp_lisp_obj_cons_t
{
	XP_LISP_OBJ_HEADER;
	struct xp_lisp_obj_t* car;
	struct xp_lisp_obj_t* cdr;
};

struct xp_lisp_obj_func_t
{
	XP_LISP_OBJ_HEADER;
	struct xp_lisp_obj_t* formal;
	struct xp_lisp_obj_t* body;
};

struct xp_lisp_obj_macro_t
{
	XP_LISP_OBJ_HEADER;
	struct xp_lisp_obj_t* formal;
	struct xp_lisp_obj_t* body;
};

struct xp_lisp_obj_prim_t
{
	XP_LISP_OBJ_HEADER;
	void* impl; // xp_lisp_prim_t
};

typedef struct xp_lisp_obj_t         xp_lisp_obj_t;
typedef struct xp_lisp_obj_nil_t     xp_lisp_obj_nil_t;
typedef struct xp_lisp_obj_true_t    xp_lisp_obj_true_t;
typedef struct xp_lisp_obj_int_t     xp_lisp_obj_int_t;
typedef struct xp_lisp_obj_float_t   xp_lisp_obj_float_t;
typedef struct xp_lisp_obj_symbol_t  xp_lisp_obj_symbol_t;
typedef struct xp_lisp_obj_string_t  xp_lisp_obj_string_t;
typedef struct xp_lisp_obj_cons_t    xp_lisp_obj_cons_t;
typedef struct xp_lisp_obj_func_t    xp_lisp_obj_func_t;
typedef struct xp_lisp_obj_macro_t   xp_lisp_obj_macro_t;
typedef struct xp_lisp_obj_prim_t    xp_lisp_obj_prim_t;

// header access
#define XP_LISP_TYPE(x)     (((xp_lisp_obj_t*)x)->type)
#define XP_LISP_SIZE(x)     (((xp_lisp_obj_t*)x)->size)
#define XP_LISP_MARK(x)     (((xp_lisp_obj_t*)x)->mark)
#define XP_LISP_LOCK(x)     (((xp_lisp_obj_t*)x)->lock)
#define XP_LISP_LINK(x)     (((xp_lisp_obj_t*)x)->link)

// value access
#define XP_LISP_IVALUE(x)   (((xp_lisp_obj_int_t*)x)->value)
#define XP_LISP_FVALUE(x)   (((xp_lisp_obj_float_t*)x)->value)

#ifdef __BORLANDC__
#define XP_LISP_SYMVALUE(x) ((xp_lisp_char*)(((xp_lisp_obj_symbol_t*)x) + 1))
#else
#define XP_LISP_SYMVALUE(x) (((xp_lisp_obj_symbol_t*)x)->buffer)
#endif
#define XP_LISP_SYMLEN(x)   ((((xp_lisp_obj_symbol_t*)x)->size - sizeof(xp_lisp_obj_t)) / sizeof(xp_lisp_char) - 1)

#ifdef __BORLANDC__
#define XP_LISP_STRVALUE(x) ((xp_lisp_char*)(((xp_lisp_obj_string_t*)x) + 1))
#else
#define XP_LISP_STRVALUE(x) (((xp_lisp_obj_string_t*)x)->buffer)
#endif
#define XP_LISP_STRLEN(x)   ((((xp_lisp_obj_string_t*)x)->size - sizeof(xp_lisp_obj_t)) / sizeof(xp_lisp_char) - 1)

#define XP_LISP_CAR(x)      (((xp_lisp_obj_cons_t*)x)->car)
#define XP_LISP_CDR(x)      (((xp_lisp_obj_cons_t*)x)->cdr)
#define XP_LISP_FFORMAL(x)  (((xp_lisp_obj_func_t*)x)->formal)
#define XP_LISP_FBODY(x)    (((xp_lisp_obj_func_t*)x)->body)
#define XP_LISP_MFORMAL(x)  (((xp_lisp_obj_macro_t*)x)->formal)
#define XP_LISP_MBODY(x)    (((xp_lisp_obj_macro_t*)x)->body)
#define XP_LISP_PIMPL(x)    ((xp_lisp_pimpl_t)(((xp_lisp_obj_prim_t*)x)->impl))

#endif
