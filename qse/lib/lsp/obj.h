/*
 * $Id: obj.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LSP_OBJ_H_
#define _QSE_LSP_OBJ_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

/* object types */
enum 
{
	QSE_LSP_OBJ_NIL = 0,
	QSE_LSP_OBJ_TRUE,
	QSE_LSP_OBJ_INT,
	QSE_LSP_OBJ_REAL,
	QSE_LSP_OBJ_SYM,
	QSE_LSP_OBJ_STR,
	QSE_LSP_OBJ_CONS,
	QSE_LSP_OBJ_FUNC,
	QSE_LSP_OBJ_MACRO,
	QSE_LSP_OBJ_PRIM,

	QSE_LSP_TYPE_COUNT /* the number of lsp object types */
};

typedef struct qse_lsp_objhdr_t     qse_lsp_objhdr_t;
typedef struct qse_lsp_obj_nil_t    qse_lsp_obj_nil_t;
typedef struct qse_lsp_obj_true_t   qse_lsp_obj_true_t;
typedef struct qse_lsp_obj_int_t    qse_lsp_obj_int_t;
typedef struct qse_lsp_obj_real_t   qse_lsp_obj_real_t;
typedef struct qse_lsp_obj_sym_t    qse_lsp_obj_sym_t;
typedef struct qse_lsp_obj_str_t    qse_lsp_obj_str_t;
typedef struct qse_lsp_obj_cons_t   qse_lsp_obj_cons_t;
typedef struct qse_lsp_obj_func_t   qse_lsp_obj_func_t;
typedef struct qse_lsp_obj_macro_t  qse_lsp_obj_macro_t;
typedef struct qse_lsp_obj_prim_t   qse_lsp_obj_prim_t;

struct qse_lsp_objhdr_t
{
	qse_uint32_t   type:  8;
	qse_uint32_t   mark:  4;
	qse_uint32_t   perm:  4;
	qse_uint32_t   lock: 16;
	qse_size_t     size;
	qse_lsp_obj_t* link;
};

struct qse_lsp_obj_t
{
	qse_lsp_objhdr_t hdr;
};

struct qse_lsp_obj_nil_t
{
	qse_lsp_objhdr_t hdr;
};

struct qse_lsp_obj_true_t
{
	qse_lsp_objhdr_t hdr;
};

struct qse_lsp_obj_int_t
{
	qse_lsp_objhdr_t hdr;
	qse_long_t value;
};

struct qse_lsp_obj_real_t
{
	qse_lsp_objhdr_t hdr;
	qse_real_t value;
};

struct qse_lsp_obj_sym_t
{
	qse_lsp_objhdr_t hdr;
#if defined(__GNUC__)
	qse_char_t buffer[0];
#endif
};

struct qse_lsp_obj_str_t
{
	qse_lsp_objhdr_t hdr;
#if defined(__GNUC__)
	qse_char_t buffer[0];
#endif
};

struct qse_lsp_obj_cons_t
{
	qse_lsp_objhdr_t hdr;
	qse_lsp_obj_t* car;
	qse_lsp_obj_t* cdr;
};

struct qse_lsp_obj_func_t
{
	qse_lsp_objhdr_t hdr;
	qse_lsp_obj_t* formal;
	qse_lsp_obj_t* body;
};

struct qse_lsp_obj_macro_t
{
	qse_lsp_objhdr_t hdr;
	qse_lsp_obj_t* formal;
	qse_lsp_obj_t* body;
};

struct qse_lsp_obj_prim_t
{
	qse_lsp_objhdr_t hdr;
	qse_lsp_prim_t impl;
	qse_size_t min_args;
	qse_size_t max_args;
};

/* header access */
#define QSE_LSP_TYPE(x)  (((qse_lsp_obj_t*)x)->hdr.type)
#define QSE_LSP_SIZE(x)  (((qse_lsp_obj_t*)x)->hdr.size)
#define QSE_LSP_MARK(x)  (((qse_lsp_obj_t*)x)->hdr.mark)
#define QSE_LSP_PERM(x)  (((qse_lsp_obj_t*)x)->hdr.perm)
#define QSE_LSP_LOCK(x)  (((qse_lsp_obj_t*)x)->hdr.lock)
#define QSE_LSP_LINK(x)  (((qse_lsp_obj_t*)x)->hdr.link)

/* value access */
#define QSE_LSP_IVAL(x) (((qse_lsp_obj_int_t*)x)->value)
#define QSE_LSP_RVAL(x) (((qse_lsp_obj_real_t*)x)->value)

#if defined(__GNUC__)
	#define QSE_LSP_SYMPTR(x) (((qse_lsp_obj_sym_t*)x)->buffer)
#else
	#define QSE_LSP_SYMPTR(x) ((qse_char_t*)(((qse_lsp_obj_sym_t*)x) + 1))
#endif
#define QSE_LSP_SYMLEN(x) ((((qse_lsp_obj_sym_t*)x)->hdr.size - sizeof(qse_lsp_obj_t)) / sizeof(qse_char_t) - 1)

#if defined(__GNUC__)
	#define QSE_LSP_STRPTR(x) (((qse_lsp_obj_str_t*)x)->buffer)
#else
	#define QSE_LSP_STRPTR(x) ((qse_char_t*)(((qse_lsp_obj_str_t*)x) + 1))
#endif
#define QSE_LSP_STRLEN(x) ((((qse_lsp_obj_str_t*)x)->hdr.size - sizeof(qse_lsp_obj_t)) / sizeof(qse_char_t) - 1)

#define QSE_LSP_CAR(x)      (((qse_lsp_obj_cons_t*)x)->car)
#define QSE_LSP_CDR(x)      (((qse_lsp_obj_cons_t*)x)->cdr)
#define QSE_LSP_FFORMAL(x)  (((qse_lsp_obj_func_t*)x)->formal)
#define QSE_LSP_FBODY(x)    (((qse_lsp_obj_func_t*)x)->body)
#define QSE_LSP_MFORMAL(x)  (((qse_lsp_obj_macro_t*)x)->formal)
#define QSE_LSP_MBODY(x)    (((qse_lsp_obj_macro_t*)x)->body)
#define QSE_LSP_PIMPL(x)    (((qse_lsp_obj_prim_t*)x)->impl)
#define QSE_LSP_PMINARGS(x) (((qse_lsp_obj_prim_t*)x)->min_args)
#define QSE_LSP_PMAXARGS(x) (((qse_lsp_obj_prim_t*)x)->max_args)

#endif
