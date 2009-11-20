/*
 * $Id: rex.h 304 2009-11-20 05:12:27Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_CMN_REX_H_
#define _QSE_CMN_REX_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 *
 * Regular Esseression Syntax
 *   A regular expression is zero or more branches, separated by '|'.
 *   ......
 *   ......
 *
 * Compiled form of a regular expression:
 *
 * | expression                                                                      |
 * | header  | branch                          | branch              | branch        |
 * | nb | el | na | bl | cmd | arg | cmd | arg | na | bl | cmd | arg | na | bl | cmd |
 *
 * - nb: the number of branches
 * -  el: the length of a expression including the length of nb and el
 * -  na: the number of atoms
 * -  bl: the length of a branch including the length of na and bl
 * -  cmd: The command and repetition info encoded together. 
 *
 * Some commands require an argument to follow them but some other don't.
 * It is encoded as follows:
 * .................
 * 
 * Subexpressions can be nested by having the command "GROUP" 
 * and a subexpression as its argument.
 *
 * Examples:
 * a.c -> |1|6|5|ORD_CHAR(no bound)|a|ANY_CHAR(no bound)|ORD_CHAR(no bound)|c|
 * ab|xy -> |2|10|4|ORD_CHAR(no bound)|a|ORD_CHAR(no bound)|b|4|ORD_CHAR(no bound)|x|ORD_CHAR(no bound)|y|
 *
 * @todo
 * - support \\n to refer to the nth matching substring
 */

#define QSE_REX_NA(code) (*(qse_size_t*)(code))

#define QSE_REX_LEN(code) \
	(*(qse_size_t*)((qse_byte_t*)(code)+QSE_SIZEOF(qse_size_t)))

enum qse_rex_option_t
{
	QSE_REX_BUILD_NOBOUND    = (1 << 0),
	QSE_REX_MATCH_IGNORECASE = (1 << 8),


	/**< do not support the {n,m} style occurrence specifier */
	QSE_REX_NOBOUND = (1 << 0),

	/**< perform case-insensitive match */
	QSE_REX_IGNORECASE = (1 << 8)
};

enum qse_rex_errnum_t
{
	QSE_REX_ENOERR = 0,
	QSE_REX_ENOMEM,        /* no sufficient memory available */
        QSE_REX_ERECUR,        /* recursion too deep */
        QSE_REX_ERPAREN,       /* a right parenthesis is expected */
        QSE_REX_ERBRACKET,     /* a right bracket is expected */
        QSE_REX_ERBRACE,       /* a right brace is expected */
        QSE_REX_EUNBALPAREN,   /* unbalanced parenthesis */
        QSE_REX_EINVALBRACE,   /* invalid brace */
        QSE_REX_ECOLON,        /* a colon is expected */
        QSE_REX_ECRANGE,       /* invalid character range */
        QSE_REX_ECCLASS,       /* invalid character class */
        QSE_REX_EBRANGE,       /* invalid boundary range */
        QSE_REX_EEND,          /* unexpected end of the pattern */
        QSE_REX_EGARBAGE       /* garbage after the pattern */
};
typedef enum qse_rex_errnum_t qse_rex_errnum_t;

enum qse_rex_node_id_t
{
	QSE_REX_NODE_START,
	QSE_REX_NODE_END,
	QSE_REX_NODE_NOP,
	QSE_REX_NODE_BOL,     /* beginning of line */
	QSE_REX_NODE_EOL,     /* end of line */
	QSE_REX_NODE_ANYCHAR, /* dot */
	QSE_REX_NODE_CHAR,    /* single character */
	QSE_REX_NODE_CHARSET, /* character set */
	QSE_REX_NODE_BRANCH,
	QSE_REX_NODE_GROUP,
	QSE_REX_NODE_GROUPEND
};
typedef enum qse_rex_node_id_t qse_rex_node_id_t;

typedef struct qse_rex_node_t qse_rex_node_t;
struct qse_rex_node_t
{
	/* for internal management. not used for startnode */
	qse_rex_node_t* link; 

	/* connect to the next node in the graph */
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
		qse_char_t* cs; /* charset */

		struct
		{
			qse_rex_node_t* left;
			qse_rex_node_t* right;
		} b;

		struct
		{
			qse_rex_node_t* head;
		} g;

		struct
		{
			qse_rex_node_t* group;
		} ge;
	} u;

	struct
	{
		qse_size_t min;
		qse_size_t max;
	} occ;
};

/**
 * The qse_rex_t type defines a regular expression processor.
 * You can compile a regular expression and match it againt a string.
 */
typedef struct qse_rex_t qse_rex_t;
struct qse_rex_t
{
	QSE_DEFINE_COMMON_FIELDS (rex)
	qse_rex_errnum_t errnum;
	int option;
	qse_rex_node_t*  code;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (rex)

qse_rex_t* qse_rex_open (
	qse_mmgr_t*     mmgr,
	qse_size_t      xtn,
	qse_rex_node_t* code
);

void qse_rex_close (
	qse_rex_t* rex
);


/**
 * The qse_rex_getoption() function returns the current options.
 */
int qse_rex_getoption (
	qse_rex_t* rex /**< regular expression processor */
);

/**
 * The qse_rex_setoption() function overrides the current options with options.
 */
void qse_rex_setoption (
	qse_rex_t* rex, /**< regular expression processor */
	int        opts /**< 0 or number XORed of ::qse_rex_option_t enumerators */
);

qse_rex_node_t* qse_rex_comp (
	qse_rex_t*        rex,
	const qse_char_t* ptn,
	qse_size_t        len
);

int qse_rex_exec (
	qse_rex_t*        rex,
	const qse_char_t* str,
	qse_size_t        len,
	const qse_char_t* substr,
	qse_size_t        sublen
);

void* qse_buildrex (
	qse_mmgr_t*       mmgr,
	qse_size_t        depth,
	int               option,
	const qse_char_t* ptn,
	qse_size_t        len,
	qse_rex_errnum_t* errnum
);

int qse_matchrex (
	qse_mmgr_t*        mmgr,
	qse_size_t         depth,
	void*              code, 
	int                option,
	const qse_char_t*  str, 
	qse_size_t         len, 
	const qse_char_t*  substr, 
	qse_size_t         sublen, 
	qse_cstr_t*        match,	
	qse_rex_errnum_t*  errnum
);

void qse_freerex (
	qse_mmgr_t* mmgr,
	void*       code
);

qse_bool_t qse_isemptyrex (
	void* code
);

#if 0
void qse_dprintrex (qse_rex_t* rex, void* rex);
#endif

#ifdef __cplusplus
}
#endif

#endif
