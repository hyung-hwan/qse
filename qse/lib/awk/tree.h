/*
 * $Id: tree.h 381 2008-09-24 11:07:24Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_AWK_TREE_H_
#define _QSE_LIB_AWK_TREE_H_

enum qse_awk_nde_type_t
{
	QSE_AWK_NDE_NULL,

	/* statement */
	QSE_AWK_NDE_BLK,
	QSE_AWK_NDE_IF,
	QSE_AWK_NDE_WHILE,
	QSE_AWK_NDE_DOWHILE,
	QSE_AWK_NDE_FOR,
	QSE_AWK_NDE_FOREACH,
	QSE_AWK_NDE_BREAK,
	QSE_AWK_NDE_CONTINUE,
	QSE_AWK_NDE_RETURN,
	QSE_AWK_NDE_EXIT,
	QSE_AWK_NDE_NEXT,
	QSE_AWK_NDE_NEXTFILE,
	QSE_AWK_NDE_DELETE,
	QSE_AWK_NDE_RESET,
	QSE_AWK_NDE_PRINT,
	QSE_AWK_NDE_PRINTF,

	/* expression */
	/* if you change the following values including their order,
	 * you should change __eval_func of __eval_expression 
	 * in run.c accordingly */
	QSE_AWK_NDE_GRP, 
	QSE_AWK_NDE_ASS,
	QSE_AWK_NDE_EXP_BIN,
	QSE_AWK_NDE_EXP_UNR,
	QSE_AWK_NDE_EXP_INCPRE,
	QSE_AWK_NDE_EXP_INCPST,
	QSE_AWK_NDE_CND,
	QSE_AWK_NDE_BFN,
	QSE_AWK_NDE_AFN,
	QSE_AWK_NDE_INT,
	QSE_AWK_NDE_REAL,
	QSE_AWK_NDE_STR,
	QSE_AWK_NDE_REX,

	/* keep this order for the following items otherwise, you may have 
	 * to change eval_incpre and eval_incpst in run.c as well as
	 * QSE_AWK_VAL_REF_XXX in awk.h */
	QSE_AWK_NDE_NAMED,
	QSE_AWK_NDE_GLOBAL,
	QSE_AWK_NDE_LOCAL,
	QSE_AWK_NDE_ARG,
	QSE_AWK_NDE_NAMEDIDX,
	QSE_AWK_NDE_GLOBALIDX,
	QSE_AWK_NDE_LOCALIDX,
	QSE_AWK_NDE_ARGIDX,
	QSE_AWK_NDE_POS,
	/* ---------------------------------- */

	QSE_AWK_NDE_GETLINE
};

enum qse_awk_in_type_t
{
	/* the order of these values match 
	 * __in_type_map and __in_opt_map in extio.c */

	QSE_AWK_IN_PIPE,
	QSE_AWK_IN_COPROC,
	QSE_AWK_IN_FILE,
	QSE_AWK_IN_CONSOLE
};

enum qse_awk_out_type_t
{
	/* the order of these values match 
	 * __out_type_map and __out_opt_map in extio.c */

	QSE_AWK_OUT_PIPE,
	QSE_AWK_OUT_COPROC,
	QSE_AWK_OUT_FILE,
	QSE_AWK_OUT_FILE_APPEND,
	QSE_AWK_OUT_CONSOLE
};

/* afn (awk function defined with the keyword function) */
typedef struct qse_awk_afn_t           qse_awk_afn_t;
typedef struct qse_awk_nde_t           qse_awk_nde_t;
typedef struct qse_awk_nde_blk_t       qse_awk_nde_blk_t;
typedef struct qse_awk_nde_grp_t       qse_awk_nde_grp_t;
typedef struct qse_awk_nde_ass_t       qse_awk_nde_ass_t;
typedef struct qse_awk_nde_exp_t       qse_awk_nde_exp_t;
typedef struct qse_awk_nde_cnd_t       qse_awk_nde_cnd_t;
typedef struct qse_awk_nde_pos_t       qse_awk_nde_pos_t;

#ifndef QSE_AWK_NDE_INT_DEFINED
#define QSE_AWK_NDE_INT_DEFINED
typedef struct qse_awk_nde_int_t       qse_awk_nde_int_t;
#endif

#ifndef QSE_AWK_NDE_REAL_DEFINED
#define QSE_AWK_NDE_REAL_DEFINED
typedef struct qse_awk_nde_real_t      qse_awk_nde_real_t;
#endif

typedef struct qse_awk_nde_str_t       qse_awk_nde_str_t;
typedef struct qse_awk_nde_rex_t       qse_awk_nde_rex_t;
typedef struct qse_awk_nde_var_t       qse_awk_nde_var_t;
typedef struct qse_awk_nde_call_t      qse_awk_nde_call_t;
typedef struct qse_awk_nde_getline_t   qse_awk_nde_getline_t;

typedef struct qse_awk_nde_if_t        qse_awk_nde_if_t;
typedef struct qse_awk_nde_while_t     qse_awk_nde_while_t;
typedef struct qse_awk_nde_for_t       qse_awk_nde_for_t;
typedef struct qse_awk_nde_foreach_t   qse_awk_nde_foreach_t;
typedef struct qse_awk_nde_break_t     qse_awk_nde_break_t;
typedef struct qse_awk_nde_continue_t  qse_awk_nde_continue_t;
typedef struct qse_awk_nde_return_t    qse_awk_nde_return_t;
typedef struct qse_awk_nde_exit_t      qse_awk_nde_exit_t;
typedef struct qse_awk_nde_next_t      qse_awk_nde_next_t;
typedef struct qse_awk_nde_nextfile_t  qse_awk_nde_nextfile_t;
typedef struct qse_awk_nde_delete_t    qse_awk_nde_delete_t;
typedef struct qse_awk_nde_reset_t     qse_awk_nde_reset_t;
typedef struct qse_awk_nde_print_t     qse_awk_nde_print_t;

struct qse_awk_afn_t
{
	qse_xstr_t name;
	qse_size_t nargs;
	qse_awk_nde_t* body;
};

#define QSE_AWK_NDE_HDR \
	int type; \
	qse_size_t line; \
	qse_awk_nde_t* next

struct qse_awk_nde_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_BLK - block statement including top-level blocks */
struct qse_awk_nde_blk_t
{
	QSE_AWK_NDE_HDR;
	qse_size_t nlocals;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_GRP - expression group */
struct qse_awk_nde_grp_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_ASS - assignment */
struct qse_awk_nde_ass_t
{
	QSE_AWK_NDE_HDR;
	int opcode;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right;
};

/* QSE_AWK_NDE_EXP_BIN, QSE_AWK_NDE_EXP_UNR, 
 * QSE_AWK_NDE_EXP_INCPRE, QSE_AW_NDE_EXP_INCPST */
struct qse_awk_nde_exp_t
{
	QSE_AWK_NDE_HDR;
	int opcode;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right; /* QSE_NULL for UNR, INCPRE, INCPST */
};

/* QSE_AWK_NDE_CND */
struct qse_awk_nde_cnd_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right;
};

/* QSE_AWK_NDE_POS - positional - $1, $2, $x, etc */
struct qse_awk_nde_pos_t  
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val;	
};

/* QSE_AWK_NDE_INT */
struct qse_awk_nde_int_t
{
	QSE_AWK_NDE_HDR;
	qse_long_t val;
	qse_char_t* str; 
	qse_size_t  len;
};

/* QSE_AWK_NDE_REAL */
struct qse_awk_nde_real_t
{
	QSE_AWK_NDE_HDR;
	qse_real_t val;
	qse_char_t* str;
	qse_size_t  len;
};

/* QSE_AWK_NDE_STR */
struct qse_awk_nde_str_t
{
	QSE_AWK_NDE_HDR;
	qse_char_t* buf;
	qse_size_t  len;
};

/* QSE_AWK_NDE_REX */
struct qse_awk_nde_rex_t
{
	QSE_AWK_NDE_HDR;
	qse_char_t* buf;
	qse_size_t  len;
	void*      code;
};

/* QSE_AWK_NDE_NAMED, QSE_AWK_NDE_GLOBAL, 
 * QSE_AWK_NDE_LOCAL, QSE_AWK_NDE_ARG 
 * QSE_AWK_NDE_NAMEDIDX, QSE_AWK_NDE_GLOBALIDX, 
 * QSE_AWK_NDE_LOCALIDX, QSE_AWK_NDE_ARGIDX */
struct qse_awk_nde_var_t
{
	QSE_AWK_NDE_HDR;
	struct 
	{
		qse_xstr_t name;
		qse_size_t idxa;
	} id;
	qse_awk_nde_t* idx; /* QSE_NULL for non-XXXXIDX */
};

/* QSE_AWK_NDE_BFN, QSE_AWK_NDE_AFN */
struct qse_awk_nde_call_t
{
	QSE_AWK_NDE_HDR;
	union
	{
		struct
		{
			qse_xstr_t name;
		} afn;

		/* minimum information of a intrinsic function 
		 * needed during run-time. */
		struct
		{
			qse_xstr_t name;

			/* original name. if qse_awk_setword has been 
			 * invoked, oname can be different from name */
			qse_xstr_t oname;

			struct
			{
				qse_size_t min;
				qse_size_t max;
				const qse_char_t* spec;
			} arg;

			int (*handler) (
				qse_awk_run_t*, const qse_char_t*, qse_size_t);
		} bfn;
	} what;
	qse_awk_nde_t* args;
	qse_size_t nargs;
};

/* QSE_AWK_NDE_GETLINE */
struct qse_awk_nde_getline_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
	int in_type; /* QSE_AWK_GETLINE_XXX */
	qse_awk_nde_t* in;
};

/* QSE_AWK_NDE_IF */
struct qse_awk_nde_if_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* then_part;
	qse_awk_nde_t* else_part; /* optional */
};

/* QSE_AWK_NDE_WHILE, QSE_AWK_NDE_DOWHILE */
struct qse_awk_nde_while_t
{
	QSE_AWK_NDE_HDR; 
	qse_awk_nde_t* test;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_FOR */
struct qse_awk_nde_for_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* init; /* optional */
	qse_awk_nde_t* test; /* optional */
	qse_awk_nde_t* incr; /* optional */
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_FOREACH */
struct qse_awk_nde_foreach_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_BREAK */
struct qse_awk_nde_break_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_CONTINUE */
struct qse_awk_nde_continue_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_RETURN */
struct qse_awk_nde_return_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val; /* optional (no return code if QSE_NULL) */	
};

/* QSE_AWK_NDE_EXIT */
struct qse_awk_nde_exit_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val; /* optional (no exit code if QSE_NULL) */
};

/* QSE_AWK_NDE_NEXT */
struct qse_awk_nde_next_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_NEXTFILE */
struct qse_awk_nde_nextfile_t
{
	QSE_AWK_NDE_HDR;
	int out;
};

/* QSE_AWK_NDE_DELETE */
struct qse_awk_nde_delete_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
};

/* QSE_AWK_NDE_RESET */
struct qse_awk_nde_reset_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
};

/* QSE_AWK_NDE_PRINT */
struct qse_awk_nde_print_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* args;
	int out_type; /* QSE_AWK_OUT_XXX */
	qse_awk_nde_t* out;
};

#ifdef __cplusplus
extern "C" {
#endif

/* print the entire tree */
int qse_awk_prnpt (qse_awk_t* awk, qse_awk_nde_t* tree);
/* print a single top-level node */
int qse_awk_prnnde (qse_awk_t* awk, qse_awk_nde_t* node); 
/* print the pattern part */
int qse_awk_prnptnpt (qse_awk_t* awk, qse_awk_nde_t* tree);

void qse_awk_clrpt (qse_awk_t* awk, qse_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
