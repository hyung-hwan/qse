/*
 * $Id: scanner.h,v 1.3 2005-05-22 15:03:20 bacon Exp $
 */

#ifndef _XP_STX_SCANNER_H_
#define _XP_STX_SCANNER_H_

#include <xp/stx/stx.h>
#include <xp/stx/token.h>

struct xp_stx_scanner_t
{
	xp_stx_token_t token;
	xp_bool_t __malloced;
};

typedef struct xp_stx_scanner_t xp_stx_scanner_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_scanner_t* xp_stx_scanner_open (xp_stx_scanner_t* scanner);
void xp_stx_scanner_close (xp_stx_scanner_t* scanner);

#ifdef __cplusplus
}
#endif

#endif
