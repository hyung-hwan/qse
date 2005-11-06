/*
 * $Id: lex.c,v 1.1 2005-11-06 12:01:29 bacon Exp $
 */

#include <xp/awk/lex.h>
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>

static int __get_char (xp_awk_lex_t* lex);
static int __unget_char (xp_awk_lex_t* lex, xp_cint_t c);
static int __skip_spaces (xp_awk_lex_t* lex);
static int __skip_comment (xp_awk_lex_t* lex);

#define GET_CHAR(lex) \
	do { if (__get_char(lex) == -1) return -1; } while(0)
#define GET_CHAR_TO(lex, c) \
	do { if (__get_char(lex) == -1) return -1; c = (lex)->curc; } while(0)

#define SET_TOKEN_TYPE(lex,code) ((lex)->token.type = code)
#define ADD_TOKEN_STR(lex,str) \
	do { if (xp_str_cat(&(lex)->token, (str)) == -1) return -1; } while (0)

xp_awk_lex_t* xp_awk_lex_open (
	xp_awk_lex_t* lex, xp_awk_t* awk)
{
	if (awk == XP_NULL) return XP_NULL;

	if (lex == XP_NULL) {
		lex = (xp_awk_lex_t*) xp_malloc (xp_sizeof(xp_awk_lex_t));
		if (lex == XP_NULL) return XP_NULL;
		lex->__malloced = xp_true;
	}
	else lex->__malloced = xp_false;

	if (xp_str_open (&lex->token, 128) == XP_NULL) {
		if (lex->__malloced) xp_free (lex);
		return XP_NULL;
	}

	lex->awk = awk;
	lex->ungotc_count = 0;

	/* if rewind is not supported, the following rewind call
	 * would fail. in this case, we just ignore the failure
	 * assuming that this lex can still be used in a single-pass
	 * compiler */
	xp_awk_lex_rewind(lex);

	if (__get_char(lex) == -1) {
		xp_str_close (&lex->token);
		if (lex->__malloced) xp_free (lex);
		return XP_NULL;
	}

	return lex;
}

void xp_awk_lex_close (xp_awk_lex_t* lex)
{
	xp_str_close (&lex->token);
	if (lex->__malloced) xp_free (lex);
}

int xp_awk_lex_rewind (xp_awk_lex_t* lex)
{
	xp_awk_t* awk = lex->awk;
	lex->ungotc_count = 0;
	return awk->input_func (awk, XP_SCE_INPUT_REWIND, XP_NULL);
}

int xp_awk_lex_fetch_token (xp_awk_lex_t* lex)
{
	xp_awk_t* awk = lex->awk;
	xp_cint_t c;
	int n;

	do {
		if (__skip_spaces(lex) == -1) return -1;
		if ((n = __skip_comment(lex)) == -1) return -1;
	} while (n == 1);

	xp_str_clear (&lex->token);
	c = lex->curc;

	if (c == XP_CHAR_EOF) {
		SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_END);
	}	
	else if (xp_isdigit(c)) {
	}
	else if (xp_isalpha(c) || c == XP_CHAR('_')) {
	}
	else if (c == XP_CHAR('\"')) {
	}
	else if (c == XP_CHAR('=')) {
		GET_CHAR_TO (lex, c);
		if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_EQ);
			ADD_TOKEN_STR(lex, XP_TEXT("=="));
			GET_CHAR_TO (lex, c);
		}
		else {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_ASSIGN);
			ADD_TOKEN_STR(lex, XP_TEXT("="));
		}
	}
	else if (c == XP_CHAR('!')) {
		GET_CHAR_TO (lex, c);
		if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_NE);
			ADD_TOKEN_STR(lex, XP_TEXT("!="));
			GET_CHAR_TO (lex, c);
		}
		else {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_NOT);
			ADD_TOKEN_STR(lex, XP_TEXT("!"));
		}
	}
	else if (c == XP_CHAR('+')) {
		GET_CHAR_TO (lex, c);
		if (c == XP_CHAR('+')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_INC);
			ADD_TOKEN_STR(lex, XP_TEXT("++"));
			GET_CHAR_TO (lex, c);
		}
		else if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_PLUS_ASSIGN);
			ADD_TOKEN_STR(lex, XP_TEXT("+="));
			GET_CHAR_TO (lex, c);
		}
		else if (xp_isdigit(c)) {
		//	read_number (XP_CHAR('+'));
		}
		else {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_PLUS);
			ADD_TOKEN_STR(lex, XP_TEXT("+"));
		}
	}
	else if (c == XP_CHAR('-')) {
		GET_CHAR_TO (lex, c);
		if (c == XP_CHAR('-')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_DEC);
			ADD_TOKEN_STR(lex, XP_TEXT("--"));
			GET_CHAR_TO (lex, c);
		}
		else if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_MINUS_ASSIGN);
			ADD_TOKEN_STR(lex, XP_TEXT("-="));
			GET_CHAR_TO (lex, c);
		}
		else if (xp_isdigit(c)) {
		//	read_number (XP_CHAR('-'));
		}
		else {
			SET_TOKEN_TYPE (lex, XP_SCE_TOKEN_MINUS);
			ADD_TOKEN_STR(lex, XP_TEXT("-"));
		}
	}
	else {
		/* set the error into awk directly though it 
		 * might look a bit awkard */
		lex->awk->error_code = XP_SCE_ERROR_WRONG_CHAR;	
		return -1;
	}

	return 0;
}

static int __get_char (xp_awk_lex_t* lex)
{
	xp_awk_t* awk = lex->awk;

	if (lex->ungotc_count > 0) {
		lex->curc = lex->ungotc[--lex->ungotc_count];
		return 0;
	}

	if (awk->input_func (awk, XP_SCE_INPUT_CONSUME, &lex->curc) == -1) {
		awk->error_code = XP_SCE_ERROR_INPUT;
		return -1;
	}

	return 0;
}

static int __unget_char (xp_awk_lex_t* lex, xp_cint_t c)
{
	xp_awk_t* awk = lex->awk;

	if (lex->ungotc_count >= xp_countof(lex->ungotc)) {
		awk->error_code = XP_SCE_ERROR_UNGET;
		return -1;
	}

	lex->ungotc[lex->ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (xp_awk_lex_t* lex)
{
	xp_cint_t c = lex->curc;
	while (xp_isspace(c)) GET_CHAR_TO (lex, c);
	return 0;
}

static int __skip_comment (xp_awk_lex_t* lex)
{
	xp_cint_t c = lex->curc;

	if (c != XP_CHAR('/')) return 0;
	GET_CHAR_TO (lex, c);

	if (c == XP_CHAR('/')) {
		do { 
			GET_CHAR_TO (lex, c);
		} while (c != '\n' && c != XP_CHAR_EOF);
		GET_CHAR (lex);
		return 1;
	}
	else if (c == XP_CHAR('*')) {
		do {
			GET_CHAR_TO (lex, c);
			if (c == XP_CHAR('*')) {
				GET_CHAR_TO (lex, c);
				if (c == XP_CHAR('/')) {
					GET_CHAR_TO (lex, c);
					break;
				}
			}
		} while (0);
		return 1;
	}

	if (__unget_char(lex, c) == -1) return -1;
	return 0;
}

