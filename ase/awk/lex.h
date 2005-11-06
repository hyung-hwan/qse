/*
 * $Id: lex.h,v 1.1 2005-11-06 12:01:29 bacon Exp $
 */

#ifndef _XP_AWK_LEX_H_
#define _XP_AWK_LEX_H_

#include <xp/awk/awk.h>
#include <xp/bas/string.h>

struct xp_awk_lex_t
{
	xp_awk_t* awk;
	xp_str_t  token;
	xp_cint_t curc;
	xp_cint_t ungotc[5];
	xp_size_t ungotc_count;
	xp_bool_t __malloced;
};

typedef struct xp_awk_lex_t xp_awk_lex_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_lex_t* xp_awk_lex_open (xp_awk_lex_t* lex, xp_awk_t* awk);
void xp_awk_lex_close (xp_awk_lex_t* lex);
int xp_awk_lex_rewind (xp_awk_lex_t* lex);
int xp_awk_lex_fetch_token (xp_awk_lex_t* lex);

#ifdef __cplusplus
}
#endif

#endif
