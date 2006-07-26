/*
 * $Id: rex.h,v 1.10 2006-07-26 16:43:35 bacon Exp $
 **/

#ifndef _XP_AWK_REX_H_
#define _XP_AWK_REX_H_

#include <xp/types.h>
#include <xp/macros.h>


/*
 * Regular Expression Syntax
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

enum
{
	XP_AWK_REX_ENOERR,    /* no error */
	XP_AWK_REX_ENOMEM,    /* ran out of memory */
	XP_AWK_REX_ERPAREN,   /* a right parenthesis is expected */
	XP_AWK_REX_ERBRACKET, /* a right bracket is expected */
	XP_AWK_REX_ERBRACE,   /* a right brace is expected */
	XP_AWK_REX_ECOLON,    /* a colon is expected */
	XP_AWK_REX_ECRANGE,   /* invalid character range */
	XP_AWK_REX_ECCLASS,   /* invalid character class */
	XP_AWK_REX_EBRANGE,   /* invalid boundary range */
	XP_AWK_REX_EEND,      /* unexpected end of the pattern */
	XP_AWK_REX_EGARBAGE   /* garbage after the pattern */
};

#define XP_AWK_REX_NA(code) (*(xp_size_t*)(code))

#define XP_AWK_REX_LEN(code) \
	(*(xp_size_t*)((xp_byte_t*)(code)+xp_sizeof(xp_size_t)))

#ifdef __cplusplus
extern "C" {
#endif

const xp_char_t* xp_awk_getrexerrstr (int errnum);

void* xp_awk_buildrex (const xp_char_t* ptn, xp_size_t len, int* errnum);

int xp_awk_matchrex (void* code,
	const xp_char_t* str, xp_size_t len, 
	const xp_char_t** match_ptr, xp_size_t* match_len, int* errnum);

void xp_awk_printrex (void* code);

#ifdef __cplusplus
}
#endif

#endif
