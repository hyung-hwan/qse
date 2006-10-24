/*
 * $Id: rex.h,v 1.21 2006-10-24 04:10:12 bacon Exp $
 **/

#ifndef _ASE_AWK_REX_H_
#define _ASE_AWK_REX_H_

#include <ase/types.h>
#include <ase/macros.h>


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

#define ASE_AWK_REX_NA(code) (*(ase_size_t*)(code))

#define ASE_AWK_REX_LEN(code) \
	(*(ase_size_t*)((ase_byte_t*)(code)+ase_sizeof(ase_size_t)))

enum ase_awk_rex_option_t
{
	ASE_AWK_REX_IGNORECASE = (1 << 0)
};

#ifdef __cplusplus
extern "C" {
#endif

void* ase_awk_buildrex (
	ase_awk_t* awk, const ase_char_t* ptn, 
	ase_size_t len, int* errnum);

int ase_awk_matchrex (
	ase_awk_t* awk, void* code, int option,
	const ase_char_t* str, ase_size_t len, 
	const ase_char_t** match_ptr, ase_size_t* match_len, int* errnum);

void ase_awk_freerex (ase_awk_t* awk, void* code);

ase_bool_t ase_awk_isemptyrex (ase_awk_t* awk, void* code);

void ase_awk_printrex (void* code);

#ifdef __cplusplus
}
#endif

#endif
