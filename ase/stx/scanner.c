/*
 * $Id
 */

#include <xp/stx/scanner.h>

xp_stx_scanner_t* xp_stx_scanner_open (xp_stx_scanner_t* scanner)
{
	if (scanner == XP_NULL) {
		scanner = (xp_stx_scanner_t*)
			xp_stx_malloc (xp_sizeof(xp_stx_scanner_t));
		if (scanner == XP_NULL) return XP_NULL;
		scanner->__malloced = xp_true;
	}
	else scanner->__malloced = xp_false;

	if (xp_stx_token_open (&scanner->token) == XP_NULL) {
		if (scanner->__malloced) xp_stx_free (scanner);
		return XP_NULL;
	}

	return scanner;
};

void xp_stx_scanner_close (xp_stx_scanner_t* scanner)
{
	xp_stx_token_close (&scanner->token);
	if (scanner->__malloced) xp_stx_free (scanner);
}

