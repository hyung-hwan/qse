/*
 * $Id: rex.h 223 2008-06-26 06:44:41Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_REX_H_
#define _QSE_CMN_REX_H_

#include <qse/types.h>
#include <qse/macros.h>

/*
 * Regular Esseression Syntax
 *   A regular expression is zero or more branches, separated by '|'.
 *   ......
 *   ......
 *
 * Compiled form of a regular expression:
 *
 *   | expression                                                                      |
 *   | header  | branch                          | branch              | branch        |
 *   | nb | el | na | bl | cmd | arg | cmd | arg | na | bl | cmd | arg | na | bl | cmd |
 *
 *   nb: the number of branches
 *   el: the length of a expression including the length of nb and el
 *   na: the number of atoms
 *   bl: the length of a branch including the length of na and bl
 *   cmd: The command and repetition info encoded together. 
 *      Some commands require an argument to follow them but some other don't.
 *      It is encoded as follows:
 *
 *   Subexpressions can be nested by having the command "GROUP" 
 *   and a subexpression as its argument.
 *
 * Examples:
 *   a.c -> |1|6|5|ORD_CHAR(no bound)|a|ANY_CHAR(no bound)|ORD_CHAR(no bound)|c|
 *   ab|xy -> |2|10|4|ORD_CHAR(no bound)|a|ORD_CHAR(no bound)|b|4|ORD_CHAR(no bound)|x|ORD_CHAR(no bound)|y|
 */

#define QSE_REX_NA(code) (*(qse_size_t*)(code))

#define QSE_REX_LEN(code) \
	(*(qse_size_t*)((qse_byte_t*)(code)+QSE_SIZEOF(qse_size_t)))

enum qse_rex_option_t
{
	QSE_REX_IGNORECASE = (1 << 0)
};

enum qse_rex_errnum_t
{
	QSE_REX_ENOERR = 0,
	QSE_REX_ENOMEM,
        QSE_REX_ERECUR,        /* recursion too deep */
        QSE_REX_ERPAREN,       /* a right parenthesis is expected */
        QSE_REX_ERBRACKET,     /* a right bracket is expected */
        QSE_REX_ERBRACE,       /* a right brace is expected */
        QSE_REX_EUNBALPAR,     /* unbalanced parenthesis */
        QSE_REX_ECOLON,        /* a colon is expected */
        QSE_REX_ECRANGE,       /* invalid character range */
        QSE_REX_ECCLASS,       /* invalid character class */
        QSE_REX_EBRANGE,       /* invalid boundary range */
        QSE_REX_EEND,          /* unexpected end of the pattern */
        QSE_REX_EGARBAGE       /* garbage after the pattern */
};

#ifdef __cplusplus
extern "C" {
#endif

void* qse_buildrex (
	qse_mmgr_t* mmgr, qse_size_t depth,
	const qse_char_t* ptn, qse_size_t len, int* errnum);

int qse_matchrex (
	qse_mmgr_t* mmgr, qse_ccls_t* ccls, qse_size_t depth,
	void* code, int option,
	const qse_char_t* str, qse_size_t len, 
	const qse_char_t** match_ptr, qse_size_t* match_len, int* errnum);

void qse_freerex (qse_mmgr_t* mmgr, void* code);

qse_bool_t qse_isemptyrex (void* code);

#if 0
void qse_dprintrex (qse_rex_t* rex, void* rex);
#endif

#ifdef __cplusplus
}
#endif

#endif
