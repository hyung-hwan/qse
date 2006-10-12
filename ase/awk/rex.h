/*
 * $Id: rex.h,v 1.18 2006-10-12 04:17:31 bacon Exp $
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

#define XP_AWK_REX_NA(code) (*(xp_size_t*)(code))

#define XP_AWK_REX_LEN(code) \
	(*(xp_size_t*)((xp_byte_t*)(code)+xp_sizeof(xp_size_t)))

enum xp_awk_rex_option_t
{
	XP_AWK_REX_IGNORECASE = (1 << 0)
};

#ifdef __cplusplus
extern "C" {
#endif

void* xp_awk_buildrex (
	xp_awk_t* awk, const xp_char_t* ptn, 
	xp_size_t len, int* errnum);

int xp_awk_matchrex (
	xp_awk_t* awk, void* code, int option,
	const xp_char_t* str, xp_size_t len, 
	const xp_char_t** match_ptr, xp_size_t* match_len, int* errnum);

void xp_awk_freerex (xp_awk_t* awk, void* code);

xp_bool_t xp_awk_isemptyrex (xp_awk_t* awk, void* code);

void xp_awk_printrex (void* code);

#ifdef __cplusplus
}
#endif

#endif
