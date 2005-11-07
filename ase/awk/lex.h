/*
 * $Id: lex.h,v 1.2 2005-11-07 16:02:44 bacon Exp $
 */

#ifndef _XP_AWK_LEX_H_
#define _XP_AWK_LEX_H_

#include <xp/awk/awk.h>

/*
struct xp_awk_lex_t
{
	xp_awk_t* awk;
	xp_str_t  token;
	xp_cint_t curc;
	xp_cint_t ungotc[5];
	xp_size_t ungotc_count;
	xp_bool_t __malloced;
};
*/

#ifdef __cplusplus
extern "C" {
#endif

int xp_awk_lex (xp_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
