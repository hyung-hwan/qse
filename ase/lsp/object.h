/*
 * $Id: object.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_OBJECT_H_
#define _RBL_OBJECT_H_

#include "types.h"

// object types
enum 
{
	RBL_OBJ_NIL = 0,
	RBL_OBJ_TRUE,
	RBL_OBJ_INT,
	RBL_OBJ_FLOAT,
	RBL_OBJ_SYMBOL,
	RBL_OBJ_STRING,
	RBL_OBJ_CONS,
	RBL_OBJ_FUNC,
	RBL_OBJ_MACRO,
	RBL_OBJ_PRIM,

	RBL_TYPE_COUNT // the number of lisp object types
};

#define RBL_OBJ_HEADER  \
	rb_uint32 type: 24; \
	rb_uint32 mark:  4; \
	rb_uint32 lock:  4; \
	xp_size_t    size;     \
	struct    xp_lisp_obj_t* link

struct xp_lisp_obj_t
{
	RBL_OBJ_HEADER;
};

struct xp_lisp_obj_nil_t
{
	RBL_OBJ_HEADER;
};

struct xp_lisp_obj_true_t
{
	RBL_OBJ_HEADER;
};

struct xp_lisp_obj_int_t
{
	RBL_OBJ_HEADER;
	xp_lisp_int value;
};

struct xp_lisp_obj_float_t
{
	RBL_OBJ_HEADER;
	xp_lisp_float value;
};

struct xp_lisp_obj_symbol_t
{
	RBL_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_lisp_char buffer[0];
#endif
};

struct xp_lisp_obj_string_t
{
	RBL_OBJ_HEADER;
#ifdef __BORLANDC__
#else
	xp_lisp_char buffer[0];
#endif
};

struct xp_lisp_obj_cons_t
{
	RBL_OBJ_HEADER;
	struct xp_lisp_obj_t* car;
	struct xp_lisp_obj_t* cdr;
};

struct xp_lisp_obj_func_t
{
	RBL_OBJ_HEADER;
	struct xp_lisp_obj_t* formal;
	struct xp_lisp_obj_t* body;
};

struct xp_lisp_obj_macro_t
{
	RBL_OBJ_HEADER;
	struct xp_lisp_obj_t* formal;
	struct xp_lisp_obj_t* body;
};

struct xp_lisp_obj_prim_t
{
	RBL_OBJ_HEADER;
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
#define RBL_TYPE(x)     (((xp_lisp_obj_t*)x)->type)
#define RBL_SIZE(x)     (((xp_lisp_obj_t*)x)->size)
#define RBL_MARK(x)     (((xp_lisp_obj_t*)x)->mark)
#define RBL_LOCK(x)     (((xp_lisp_obj_t*)x)->lock)
#define RBL_LINK(x)     (((xp_lisp_obj_t*)x)->link)

// value access
#define RBL_IVALUE(x)   (((xp_lisp_obj_int_t*)x)->value)
#define RBL_FVALUE(x)   (((xp_lisp_obj_float_t*)x)->value)

#ifdef __BORLANDC__
#define RBL_SYMVALUE(x) ((xp_lisp_char*)(((xp_lisp_obj_symbol_t*)x) + 1))
#else
#define RBL_SYMVALUE(x) (((xp_lisp_obj_symbol_t*)x)->buffer)
#endif
#define RBL_SYMLEN(x)   ((((xp_lisp_obj_symbol_t*)x)->size - sizeof(xp_lisp_obj_t)) / sizeof(xp_lisp_char) - 1)

#ifdef __BORLANDC__
#define RBL_STRVALUE(x) ((xp_lisp_char*)(((xp_lisp_obj_string_t*)x) + 1))
#else
#define RBL_STRVALUE(x) (((xp_lisp_obj_string_t*)x)->buffer)
#endif
#define RBL_STRLEN(x)   ((((xp_lisp_obj_string_t*)x)->size - sizeof(xp_lisp_obj_t)) / sizeof(xp_lisp_char) - 1)

#define RBL_CAR(x)      (((xp_lisp_obj_cons_t*)x)->car)
#define RBL_CDR(x)      (((xp_lisp_obj_cons_t*)x)->cdr)
#define RBL_FFORMAL(x)  (((xp_lisp_obj_func_t*)x)->formal)
#define RBL_FBODY(x)    (((xp_lisp_obj_func_t*)x)->body)
#define RBL_MFORMAL(x)  (((xp_lisp_obj_macro_t*)x)->formal)
#define RBL_MBODY(x)    (((xp_lisp_obj_macro_t*)x)->body)
#define RBL_PIMPL(x)    ((xp_lisp_pimpl_t)(((xp_lisp_obj_prim_t*)x)->impl))

#endif
