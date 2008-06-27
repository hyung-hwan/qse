/*
 * $Id: rex.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_REX_H_
#define _ASE_CMN_REX_H_

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

#define ASE_REX_NA(code) (*(ase_size_t*)(code))

#define ASE_REX_LEN(code) \
	(*(ase_size_t*)((ase_byte_t*)(code)+ASE_SIZEOF(ase_size_t)))

enum ase_rex_option_t
{
	ASE_REX_IGNORECASE = (1 << 0)
};

enum ase_rex_errnum_t
{
	ASE_REX_ENOERR = 0,
	ASE_REX_ENOMEM,
        ASE_REX_ERECUR,        /* recursion too deep */
        ASE_REX_ERPAREN,       /* a right parenthesis is expected */
        ASE_REX_ERBRACKET,     /* a right bracket is expected */
        ASE_REX_ERBRACE,       /* a right brace is expected */
        ASE_REX_EUNBALPAR,     /* unbalanced parenthesis */
        ASE_REX_ECOLON,        /* a colon is expected */
        ASE_REX_ECRANGE,       /* invalid character range */
        ASE_REX_ECCLASS,       /* invalid character class */
        ASE_REX_EBRANGE,       /* invalid boundary range */
        ASE_REX_EEND,          /* unexpected end of the pattern */
        ASE_REX_EGARBAGE       /* garbage after the pattern */
};

#ifdef __cplusplus
extern "C" {
#endif

void* ase_buildrex (
	ase_mmgr_t* mmgr, ase_size_t depth,
	const ase_char_t* ptn, ase_size_t len, int* errnum);

int ase_matchrex (
	ase_mmgr_t* mmgr, ase_ccls_t* ccls, ase_size_t depth,
	void* code, int option,
	const ase_char_t* str, ase_size_t len, 
	const ase_char_t** match_ptr, ase_size_t* match_len, int* errnum);

void ase_freerex (ase_mmgr_t* mmgr, void* code);

ase_bool_t ase_isemptyrex (void* code);

#if 0
void ase_dprintrex (ase_rex_t* rex, void* rex);
#endif

#ifdef __cplusplus
}
#endif

#endif
