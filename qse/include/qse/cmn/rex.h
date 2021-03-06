/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_REX_H_
#define _QSE_CMN_REX_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>

/** @file
 *
 * Regular Expression Syntax
 *
 *  regular expression := branch_set
 *  branch_set := branch [ '|' branch ]*
 *  atom := char | '.' | '^' | '$' | subgroup
 *  subgroup = '(' branch_set ')'
 *  bound := '?' | '*' | '+' | {n,m}
 *  bounded_atom := atom bound*
 *  branch := bounded_atom bounded_atom*
 * 
 * Special escaping sequence includes:
 *   \uXXXX, \XX, \000, \t, \n, \r, \v, ...
 *
 * A special character preceded by a backslash loses its special role and
 * matches the character itself literally.
 *
 * Some examples of compiled regular expressions are shown below.
 * 
 * @code
 * ab
 * START --@ NOP --@  CHAR(a) --@ CHAR(b) --@ END
 *
 * a(bc)d
 * START --@ NOP --@ CHAR(A) --@ GROUP --@ CHAR(d) --@ END
 *                   u.g.head => | @ 
 *                               | | <= u.g.end
 *                               | +---------------------------+
 *                               |                             | <= u.ge.group
 *                               @                             @ 
 *                              NOP -@ CHAR(b) -@ CHAR(c) -@ GROUPEND
 *
 * ab|cd
 * START --@ NOP --@  BRANCH --+--@ CHAR(a) --@ CHAR(b) --+--@ END
 *                             |                          |
 *                             | <= u.b.alter             |
 *                             +--@ CHAR(c) --@ CHAR(d) --+ 
 * @endcode
 *
 * @todo
 * - support \\n to refer to the nth matching substring
 */

enum qse_rex_option_t
{
	/**< do not allow a special character at normal character position. */
	QSE_REX_STRICT  = (1 << 0),

	/**< do not support the {n,m} style occurrence specifier. */
	QSE_REX_NOBOUND = (1 << 1),

	/**< perform case-insensitive match */
	QSE_REX_IGNORECASE = (1 << 2)
};

enum qse_rex_errnum_t
{
	QSE_REX_ENOERR = 0,
	QSE_REX_EOTHER,
	QSE_REX_ENOIMPL,
	QSE_REX_ESYSERR,
	QSE_REX_EINTERN,

	QSE_REX_ENOMEM,        /**< no sufficient memory available */
	QSE_REX_ENOCOMP,       /**< no expression compiled */
	QSE_REX_ERECUR,        /**< recursion too deep */
	QSE_REX_ERPAREN,       /**< right parenthesis expected */
	QSE_REX_ERBRACK,       /**< right bracket expected */
	QSE_REX_ERBRACE,       /**< right brace expected */
	QSE_REX_ECOLON,        /**< colon expected */
	QSE_REX_ECRANGE,       /**< invalid character range */
	QSE_REX_ECCLASS,       /**< invalid character class */
	QSE_REX_EBOUND,        /**< invalid occurrence bound */
	QSE_REX_ESPCAWP,       /**< special character at wrong position */
	QSE_REX_EPREEND        /**< premature expression end */
};
typedef enum qse_rex_errnum_t qse_rex_errnum_t;

enum qse_rex_node_id_t
{
	QSE_REX_NODE_START,
	QSE_REX_NODE_END,
	QSE_REX_NODE_NOP,
	QSE_REX_NODE_BOL,  /* beginning of line */
	QSE_REX_NODE_EOL,  /* end of line */
	QSE_REX_NODE_ANY,  /* dot */
	QSE_REX_NODE_CHAR, /* single character */
	QSE_REX_NODE_CSET, /* character set */
	QSE_REX_NODE_BRANCH,
	QSE_REX_NODE_GROUP,
	QSE_REX_NODE_GROUPEND,
	QSE_REX_NODE_BACKREF /* back reference */
};
typedef enum qse_rex_node_id_t qse_rex_node_id_t;

typedef struct qse_rex_node_t qse_rex_node_t;
struct qse_rex_node_t
{
	/* for internal management. not used for startnode */
	qse_rex_node_t* link; 

	/* connect to the next node in the graph.
	 * it is always NULL for a branch node. */
	qse_rex_node_t* next;

	qse_rex_node_id_t id;
	union
	{
		struct
		{
			qse_mmgr_t*     mmgr;
			qse_rex_node_t* link;
		} s;

		qse_char_t c;

		struct
		{
			int negated;
			qse_str_t* member; 
		} cset;

		struct
		{
			qse_rex_node_t* alter;
		} b;

		struct
		{
			qse_rex_node_t* head;
			qse_rex_node_t* end;
			int pseudo;
		} g;

		struct
		{
			qse_rex_node_t* group;
			int pseudo;
		} ge;

		struct
		{
			int index;		
		} bref;
	} u;

	struct
	{
		qse_size_t min;
		qse_size_t max;
	} occ;
};

enum qse_rex_cset_code_t
{
	QSE_REX_CSET_CHAR,
	QSE_REX_CSET_RANGE,
	QSE_REX_CSET_CLASS
};

/** @struct qse_rex_t
 * The qse_rex_t type defines a regular expression processor.
 * You can compile a regular expression and match it againt a string.
 */
typedef struct qse_rex_t qse_rex_t;
struct qse_rex_t
{
	qse_mmgr_t* mmgr;
	qse_rex_errnum_t errnum;
	int option;
	qse_rex_node_t* code;
};

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_rex_t* qse_rex_open (
	qse_mmgr_t*     mmgr, /**< memory manager */
	qse_size_t      xtn,  /**< extension size */
	qse_rex_node_t* code  /**< compiled regular expression code */
);

QSE_EXPORT void qse_rex_close (
	qse_rex_t* rex
);

QSE_EXPORT int qse_rex_init (
	qse_rex_t* rex, 
	qse_mmgr_t* mmgr,
	qse_rex_node_t* code
);

/** 
 * The qse_rex_fini() function finalizes a statically initialized 
 * regular expression object @a rex.
 */
QSE_EXPORT void qse_rex_fini (
	qse_rex_t* rex
);

QSE_EXPORT qse_mmgr_t* qse_rex_getmmgr (
	qse_rex_t* rex
);

QSE_EXPORT void* qse_rex_getxtn (
	qse_rex_t* rex
);

/**
 * The qse_rex_yield() function gives up the ownership of a compiled regular
 * expression. Once yielded, the compiled regular expression is not destroyed 
 * when @a rex is closed or finalized. 
 * @return start node of a compiled regular expression
 */
QSE_EXPORT qse_rex_node_t* qse_rex_yield (
	qse_rex_t* rex /**< regular expression processor */
);

/**
 * The qse_rex_getopt() function returns the current options.
 */
QSE_EXPORT int qse_rex_getopt (
	const qse_rex_t* rex /**< regular expression processor */
);

/**
 * The qse_rex_setopt() function overrides the current options with options.
 */
QSE_EXPORT void qse_rex_setopt (
	qse_rex_t* rex, /**< regular expression processor */
	int        opts /**< 0 or number XORed of ::qse_rex_option_t enumerators */
);

QSE_EXPORT qse_rex_errnum_t qse_rex_geterrnum (
	const qse_rex_t* rex
);

QSE_EXPORT const qse_char_t* qse_rex_geterrmsg (
	const qse_rex_t* rex
);

QSE_EXPORT qse_rex_node_t* qse_rex_comp (
	qse_rex_t*        rex,
	const qse_char_t* ptn,
	qse_size_t        len
);

QSE_EXPORT int qse_rex_exec (
	qse_rex_t*        rex,
	const qse_cstr_t* str, 
	const qse_cstr_t* substr,
	qse_cstr_t*       matstr
);


QSE_EXPORT void* qse_buildrex (
	qse_mmgr_t*       mmgr,
	qse_size_t        depth,
	int               option,
	const qse_char_t* ptn,
	qse_size_t        len,
	qse_rex_errnum_t* errnum
);

QSE_EXPORT int qse_matchrex (
	qse_mmgr_t*        mmgr,
	qse_size_t         depth,
	void*              code, 
	int                option,
	const qse_cstr_t*  str,
	const qse_cstr_t*  substr,
	qse_cstr_t*        match,	
	qse_rex_errnum_t*  errnum
);

QSE_EXPORT void qse_freerex (
	qse_mmgr_t* mmgr,
	void*       code
);

#if defined(__cplusplus)
}
#endif

#endif
