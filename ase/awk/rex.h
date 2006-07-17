/*
 * $Id: rex.h,v 1.1 2006-07-17 06:21:39 bacon Exp $
 **/

#ifndef _XP_AWK_REX_H_
#define _XP_AWK_REX_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

typedef struct xp_awk_rex_t xp_awk_rex_t;

struct xp_awk_rex_t
{
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_rex_t* xp_awk_rex_open (xp_awk_rex_t* rex);
void xp_awk_rex_close (xp_awk_rex_t* rex);
int xp_awk_rex_compile (const xp_awk_rex_t* rex, const xp_char_t* ptn);

#ifdef __cplusplus
}
#endif

#endif
