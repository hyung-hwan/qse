/*
 * $Id: rex.h,v 1.19 2006-10-22 11:34:53 bacon Exp $
 **/

#ifndef _SSE_AWK_REX_H_
#define _SSE_AWK_REX_H_

#include <sse/types.h>
#include <sse/macros.h>


/*
 * Regular Esseression Syntax
 *   A regular esseression is zero or more branches, separated by '|'.
 *   ......
 *   ......
 *
 * Compiled form of a regular esseression:
 *
 *   | esseression                                                                      |
 *   | header  | branch                          | branch              | branch        |
 *   | nb | el | na | bl | cmd | arg | cmd | arg | na | bl | cmd | arg | na | bl | cmd |
 *
 *   nb: the number of branches
 *   el: the length of a esseression including the length of nb and el
 *   na: the number of atoms
 *   bl: the length of a branch including the length of na and bl
 *   cmd: The command and repetition info encoded together. 
 *      Some commands require an argument to follow them but some other don't.
 *      It is encoded as follows:
 *
 *   Subesseressions can be nested by having the command "GROUP" 
 *   and a subesseression as its argument.
 *
 * Examples:
 *   a.c -> |1|6|5|ORD_CHAR(no bound)|a|ANY_CHAR(no bound)|ORD_CHAR(no bound)|c|
 *   ab|xy -> |2|10|4|ORD_CHAR(no bound)|a|ORD_CHAR(no bound)|b|4|ORD_CHAR(no bound)|x|ORD_CHAR(no bound)|y|
 */

#define SSE_AWK_REX_NA(code) (*(sse_size_t*)(code))

#define SSE_AWK_REX_LEN(code) \
	(*(sse_size_t*)((sse_byte_t*)(code)+sse_sizeof(sse_size_t)))

enum sse_awk_rex_option_t
{
	SSE_AWK_REX_IGNORECASE = (1 << 0)
};

#ifdef __cplusplus
extern "C" {
#endif

void* sse_awk_buildrex (
	sse_awk_t* awk, const sse_char_t* ptn, 
	sse_size_t len, int* errnum);

int sse_awk_matchrex (
	sse_awk_t* awk, void* code, int option,
	const sse_char_t* str, sse_size_t len, 
	const sse_char_t** match_ptr, sse_size_t* match_len, int* errnum);

void sse_awk_freerex (sse_awk_t* awk, void* code);

sse_bool_t sse_awk_isemptyrex (sse_awk_t* awk, void* code);

void sse_awk_printrex (void* code);

#ifdef __cplusplus
}
#endif

#endif
