/*
 * $Id: parser.h,v 1.3 2005-05-22 15:03:20 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/scanner.h>

struct xp_stx_parser_t
{
	xp_bool_t __malloced;
};

typedef struct xp_stx_parser_t xp_stx_parser_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser);
void xp_stx_parser_close (xp_stx_parser_t* parser);
int xp_stx_parser_parse (xp_stx_parser_t* parser, const xp_char_t* text);

int xp_stx_filein_raw (xp_stx_t* parser, xp_stx_getc_t getc);

#ifdef __cplusplus
}
#endif

#endif
