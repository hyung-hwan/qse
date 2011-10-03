/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

/*
  tre-internal.h - TRE internal definitions

This is the license, copyright notice, and disclaimer for TRE, a regex
matching package (library and tools) with support for approximate
matching.

Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _QSE_LIB_CMN_TRE_H_
#define _QSE_LIB_CMN_TRE_H_

/* TODO: MAKE TRE WORK LIKE GNU

PATTERN: \(.\{0,1\}\)\(~[^,]*\)\([0-9]\)\(\.*\),\([^;]*\)\(;\([^;]*\(\3[^;]*\)\).*X*\1\(.*\)\)
INPUT: ~02.,3~3;0123456789;9876543210

------------------------------------------------------
samples/cmn/tre01 gives the following output. this does not seem wrong, though.

SUBMATCH[7],[8],[9].

SUBMATCH[0] = [~02.,3~3;0123456789;9876543210]
SUBMATCH[1] = []
SUBMATCH[2] = [~0]
SUBMATCH[3] = [2]
SUBMATCH[4] = [.]
SUBMATCH[5] = [3~3]
SUBMATCH[6] = [;0123456789;9876543210]
SUBMATCH[7] = [012]
SUBMATCH[8] = [2]
SUBMATCH[9] = [3456789;9876543210

------------------------------------------------------

Using the GNU regcomp(),regexec(), the following
is printed.

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
int main (int argc, char* argv[])
{
     regex_t tre;
     regmatch_t mat[10];
     int i;
     regcomp (&tre, argv[1], 0);
     regexec (&tre, argv[2], 10, mat, 0);
     for (i = 0; i < 10; i++)
     {
          if (mat[i].rm_so == -1) break;
          printf ("SUBMATCH[%u] = [%.*s]\n", i,
               (int)(mat[i].rm_eo - mat[i].rm_so), &argv[2][mat[i].rm_so]);
     }
     regfree (&tre);
     return 0;
}

SUBMATCH[0] = [~02.,3~3;0123456789;9876543210]
SUBMATCH[1] = []
SUBMATCH[2] = [~0]
SUBMATCH[3] = [2]
SUBMATCH[4] = [.]
SUBMATCH[5] = [3~3]
SUBMATCH[6] = [;0123456789;9876543210]
SUBMATCH[7] = [0123456789]
SUBMATCH[8] = [23456789]
SUBMATCH[9] = []


------------------------------------------------------
One more example here:
$ ./tre01 "\(x*\)ab\(\(c*\1\)\(.*\)\)" "abcdefg"
Match: YES
SUBMATCH[0] = [abcdefg]
SUBMATCH[1] = []
SUBMATCH[2] = [cdefg]
SUBMATCH[3] = []
SUBMATCH[4] = [cdefg]

$ ./reg "\(x*\)ab\(\(c*\1\)\(.*\)\)" "abcdefg"
SUBMATCH[0] = [abcdefg]
SUBMATCH[1] = []
SUBMATCH[2] = [cdefg]
SUBMATCH[3] = [c]
SUBMATCH[4] = [defg]
*/

#include <qse/cmn/tre.h>

#ifdef QSE_CHAR_IS_WCHAR
#	define TRE_WCHAR
/*
#	define TRE_MULTIBYTE
#	define TRE_MBSTATE
*/
#endif

#define TRE_REGEX_T_FIELD value
#define assert QSE_ASSERT
#define NULL QSE_NULL

#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/pma.h>
#include "mem.h"

#define tre_islower(c)  QSE_ISLOWER(c)
#define tre_isupper(c)  QSE_ISUPPER(c)
#define tre_isalpha(c)  QSE_ISALPHA(c)
#define tre_isdigit(c)  QSE_ISDIGIT(c)
#define tre_isxdigit(c) QSE_ISXDIGIT(c)
#define tre_isalnum(c)  QSE_ISALNUM(c)

#define tre_isspace(c)  QSE_ISSPACE(c)
#define tre_isprint(c)  QSE_ISPRINT(c)
#define tre_isgraph(c)  QSE_ISGRAPH(c)
#define tre_iscntrl(c)  QSE_ISCNTRL(c)
#define tre_ispunct(c)  QSE_ISPUNCT(c)
#define tre_isblank(c)  QSE_ISBLANK(c)

#define tre_tolower(c)  QSE_TOLOWER(c)
#define tre_toupper(c)  QSE_TOUPPER(c)

#if defined(QSE_CHAR_IS_MCHAR) && (QSE_SIZEOF_MCHAR_T == QSE_SIZEOF_CHAR)
	typedef unsigned char tre_char_t;
#else
	typedef qse_char_t tre_char_t;
#endif
typedef qse_cint_t tre_cint_t;

#define size_t qse_size_t
#define regex_t qse_tre_t
#define regmatch_t qse_tre_match_t
#define reg_errcode_t qse_tre_errnum_t
#define tre_str_source qse_tre_strsrc_t


#define REG_OK       QSE_TRE_ENOERR
#define REG_ESPACE   QSE_TRE_ENOMEM
#define REG_NOMATCH  QSE_TRE_ENOMATCH
#define REG_BADPAT   QSE_TRE_EBADPAT
#define REG_ECOLLATE QSE_TRE_ECOLLATE
#define REG_ECTYPE   QSE_TRE_ECTYPE
#define REG_EESCAPE  QSE_TRE_EESCAPE
#define REG_ESUBREG  QSE_TRE_ESUBREG
#define REG_EBRACK   QSE_TRE_EBRACK
#define REG_EPAREN   QSE_TRE_EPAREN
#define REG_EBRACE   QSE_TRE_EBRACE
#define REG_BADBR    QSE_TRE_EBADBR
#define REG_ERANGE   QSE_TRE_ERANGE
#define REG_BADRPT   QSE_TRE_EBADRPT

/* The maximum number of iterations in a bound expression. */
#undef RE_DUP_MAX
#define RE_DUP_MAX 255

/* POSIX tre_regcomp() flags. */
#define REG_EXTENDED    QSE_TRE_EXTENDED
#define REG_ICASE       QSE_TRE_IGNORECASE
#define REG_NEWLINE     QSE_TRE_NEWLINE
#define REG_NOSUB       QSE_TRE_NOSUBREG
/* Extra tre_regcomp() flags. */
#define REG_LITERAL     QSE_TRE_LITERAL 
#define REG_RIGHT_ASSOC QSE_TRE_RIGHTASSOC
#define REG_UNGREEDY    QSE_TRE_UNGREEDY 
#define REG_NONSTDEXT   QSE_TRE_NONSTDEXT

/* POSIX tre_regexec() flags. */
#define REG_NOTBOL QSE_TRE_NOTBOL
#define REG_NOTEOL QSE_TRE_NOTEOL
#define REG_BACKTRACKING_MATCHER QSE_TRE_BACKTRACKING


#define tre_strlen(c) qse_strlen(c)

typedef qse_pma_t* tre_mem_t;

#define tre_mem_new(mmgr) qse_pma_open(mmgr,0)
#define tre_mem_destroy(mem) qse_pma_close(mem)
#define tre_mem_alloc(mem,size) qse_pma_alloc(mem,size)
#define tre_mem_calloc(mem,size) qse_pma_calloc(mem,size)

#define xmalloc(mmgr,size) QSE_MMGR_ALLOC(mmgr,size)
#define xfree(mmgr,ptr) QSE_MMGR_FREE(mmgr,ptr)
#define xrealloc(mmgr,ptr,new_size) QSE_MMGR_REALLOC(mmgr, ptr, new_size)


/* tre-ast.h */
#define tre_ast_new_node qse_tre_astnewnode
#define tre_ast_new_literal qse_tre_astnewliteral
#define tre_ast_new_iter qse_tre_astnewiter
#define tre_ast_new_union qse_tre_astnewunion
#define tre_ast_new_catenation qse_tre_astnewcatenation

/* tre-parse.h */
#define tre_parse qse_tre_parse

/* tre-stack.h */
#define tre_stack_destroy qse_tre_stackfree
#define tre_stack_new qse_tre_stacknew
#define tre_stack_num_objects qse_tre_stacknumobjs
#define tre_stack_pop_int qse_tre_stackpopint
#define tre_stack_pop_voidptr qse_tre_stackpopvoidptr
#define tre_stack_push_int qse_tre_stackpushint
#define tre_stack_push_voidptr qse_tre_stackpushvoidptr

/* this tre.h */
#define tre_compile qse_tre_compile
#define tre_free qse_tre_free
#define tre_fill_pmatch qse_tre_fillpmatch
#define tre_tnfa_run_backtrack qse_tre_runbacktrack
#define tre_tnfa_run_parallel qse_tre_runparallel
#define tre_have_backrefs qse_tre_havebackrefs

/* Define the character types and functions. */
#ifdef TRE_WCHAR
#	define TRE_CHAR_MAX QSE_TYPE_MAX(qse_wchar_t)
#	ifdef TRE_MULTIBYTE
#		define TRE_MB_CUR_MAX (qse_getmbcurmax())
#	else /* !TRE_MULTIBYTE */
#		define TRE_MB_CUR_MAX 1
#	endif /* !TRE_MULTIBYTE */
#else /* !TRE_WCHAR */
#	define TRE_CHAR_MAX 255
#	define TRE_MB_CUR_MAX 1
#endif /* !TRE_WCHAR */

#define DPRINT(msg) 

typedef qse_ctype_t tre_ctype_t;
#define tre_isctype(c,t) QSE_ISCTYPE(c,t)

typedef enum { STR_WIDE, STR_BYTE, STR_MBS, STR_USER } tre_str_type_t;

/* Returns number of bytes to add to (char *)ptr to make it
   properly aligned for the type. */
#define ALIGN(ptr, type) \
  ((((qse_uintptr_t)ptr) % QSE_SIZEOF(type)) \
   ? (QSE_SIZEOF(type) - (((qse_uintptr_t)ptr) % QSE_SIZEOF(type))) \
   : 0)

#undef MAX
#undef MIN
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

/* Define STRF to the correct printf formatter for strings. */
#ifdef TRE_WCHAR
#define STRF "ls"
#else /* !TRE_WCHAR */
#define STRF "s"
#endif /* !TRE_WCHAR */

/* TNFA transition type. A TNFA state is an array of transitions,
   the terminator is a transition with NULL `state'. */
typedef struct tnfa_transition tre_tnfa_transition_t;

struct tnfa_transition
{
	/* Range of accepted characters. */
	tre_cint_t code_min;
	tre_cint_t code_max;
	/* Pointer to the destination state. */
	tre_tnfa_transition_t *state;
	/* ID number of the destination state. */
	int state_id;
	/* -1 terminated array of tags (or NULL). */
	int *tags;
	/* Matching parameters settings (or NULL). */
	int *params;
	/* Assertion bitmap. */
	int assertions;
	/* Assertion parameters. */
	union
	{
		/* Character class assertion. */
		tre_ctype_t class;
		/* Back reference assertion. */
		int backref;
	} u;
	/* Negative character class assertions. */
	tre_ctype_t *neg_classes;
};


/* Assertions. */
#define ASSERT_AT_BOL		  1   /* Beginning of line. */
#define ASSERT_AT_EOL		  2   /* End of line. */
#define ASSERT_CHAR_CLASS	  4   /* Character class in `class'. */
#define ASSERT_CHAR_CLASS_NEG	  8   /* Character classes in `neg_classes'. */
#define ASSERT_AT_BOW		 16   /* Beginning of word. */
#define ASSERT_AT_EOW		 32   /* End of word. */
#define ASSERT_AT_WB		 64   /* Word boundary. */
#define ASSERT_AT_WB_NEG	128   /* Not a word boundary. */
#define ASSERT_BACKREF		256   /* A back reference in `backref'. */
#define ASSERT_LAST		256

/* Tag directions. */
typedef enum
{
	TRE_TAG_MINIMIZE = 0,
	TRE_TAG_MAXIMIZE = 1
} tre_tag_direction_t;

/* Parameters that can be changed dynamically while matching. */
typedef enum
{
	TRE_PARAM_COST_INS	    = 0,
	TRE_PARAM_COST_DEL	    = 1,
	TRE_PARAM_COST_SUBST	    = 2,
	TRE_PARAM_COST_MAX	    = 3,
	TRE_PARAM_MAX_INS	    = 4,
	TRE_PARAM_MAX_DEL	    = 5,
	TRE_PARAM_MAX_SUBST	    = 6,
	TRE_PARAM_MAX_ERR	    = 7,
	TRE_PARAM_DEPTH	    = 8,
	TRE_PARAM_LAST	    = 9
} tre_param_t;

/* Unset matching parameter */
#define TRE_PARAM_UNSET -1

/* Signifies the default matching parameter value. */
#define TRE_PARAM_DEFAULT -2

/* Instructions to compute submatch register values from tag values
   after a successful match.  */
struct tre_submatch_data
{
	/* Tag that gives the value for rm_so (submatch start offset). */
	int so_tag;
	/* Tag that gives the value for rm_eo (submatch end offset). */
	int eo_tag;
	/* List of submatches this submatch is contained in. */
	int *parents;
};

typedef struct tre_submatch_data tre_submatch_data_t;


/* TNFA definition. */
typedef struct tnfa tre_tnfa_t;

struct tnfa
{
	tre_tnfa_transition_t *transitions;
	unsigned int num_transitions;
	tre_tnfa_transition_t *initial;
	tre_tnfa_transition_t *final;
	tre_submatch_data_t *submatch_data;
	char *firstpos_chars;
	int first_char;
	unsigned int num_submatches;
	tre_tag_direction_t *tag_directions;
	int *minimal_tags;
	int num_tags;
	int num_minimals;
	int end_tag;
	int num_states;
	int cflags;
	int have_backrefs;
	int have_approx;
	int params_depth;
};


int tre_compile (regex_t *preg, const tre_char_t *regex, size_t n, int cflags);

void tre_free (regex_t *preg);

void tre_fill_pmatch(
	size_t nmatch, regmatch_t pmatch[], int cflags,
	const tre_tnfa_t *tnfa, int *tags, int match_eo);

reg_errcode_t tre_tnfa_run_backtrack(
	qse_mmgr_t* mmgr, const tre_tnfa_t *tnfa, const void *string,
	int len, tre_str_type_t type, int *match_tags,
	int eflags, int *match_end_ofs);


reg_errcode_t tre_tnfa_run_parallel(
	qse_mmgr_t* mmgr, const tre_tnfa_t *tnfa, const void *string, int len,
	tre_str_type_t type, int *match_tags, int eflags,
	int *match_end_ofs);


#endif

/* EOF */
