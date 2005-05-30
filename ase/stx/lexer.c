/*
 * $Id
 */

#include <xp/stx/lexer.h>
#include <xp/stx/parser.h>
#include <xp/stx/misc.h>

xp_stx_lexer_t* xp_stx_lexer_open (xp_stx_lexer_t* lexer)
{
	if (lexer == XP_NULL) {
		lexer = (xp_stx_lexer_t*)
			xp_stx_malloc (xp_sizeof(xp_stx_lexer_t));
		if (lexer == XP_NULL) return XP_NULL;
		lexer->__malloced = xp_true;
	}
	else lexer->__malloced = xp_false;

	if (xp_stx_token_open (&lexer->token, 256) == XP_NULL) {
		if (lexer->__malloced) xp_stx_free (lexer);
		return XP_NULL;
	}

	return lexer;
};

void xp_stx_lexer_close (xp_stx_lexer_t* lexer)
{
	xp_stx_token_close (&lexer->token);
	if (lexer->__malloced) xp_stx_free (lexer);
}

xp_stx_token_t* xp_stx_lexer_consume (xp_stx_lexer_t* lexer)
{
	/* TODO */
	return &lexer->token;
}
