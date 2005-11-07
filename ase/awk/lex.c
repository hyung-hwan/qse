/*
 * $Id: lex.c,v 1.2 2005-11-07 16:02:44 bacon Exp $
 */

#include <xp/awk/lex.h>
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>

static int __get_char (xp_awk_t* awk);
static int __unget_char (xp_awk_t* awk, xp_cint_t c);
static int __skip_spaces (xp_awk_t* awk);
static int __skip_comment (xp_awk_t* awk);

#define GET_CHAR(awk) \
	do { if (__get_char(awk) == -1) return -1; } while(0)
#define GET_CHAR_TO(awk, c) \
	do { if (__get_char(awk) == -1) return -1; c = (awk)->lex.curc; } while(0)

#define SET_TOKEN_TYPE(awk,code) ((awk)->lex.type = code)
#define ADD_TOKEN_STR(awk,str) \
	do { if (xp_str_cat(&(awk)->lex.token, (str)) == -1) return -1; } while (0)

int xp_awk_lex (xp_awk_t* awk)
{
	xp_cint_t c;
	int n;

	do {
		if (__skip_spaces(awk) == -1) return -1;
		if ((n = __skip_comment(awk)) == -1) return -1;
	} while (n == 1);

	xp_str_clear (&awk->lex.token);
	c = awk->lex.curc;

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

static int __get_char (xp_awk_t* awk)
{
	if (awk->ungotc_count > 0) {
		awk->curc = awk->ungotc[--awk->ungotc_count];
		return 0;
	}

	if (awk->source_func(XP_AWK_IO_DATA, 
		awk->source_arg, &awk->curc, 1) == -1) {
		awk->error_code = XP_SCE_ESRCDT;
		return -1;
	}

	return 0;
}

static int __unget_char (xp_awk_t* awk, xp_cint_t c)
{
	if (awk->ungotc_count >= xp_countof(awk->ungotc)) {
		awk->error_code = XP_SCE_EUNGET;
		return -1;
	}

	lex->ungotc[awk->ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (xp_awk_t* awk)
{
	xp_cint_t c = awk->lex.curc;
	while (xp_isspace(c)) GET_CHAR_TO (awk, c);
	return 0;
}

static int __skip_comment (xp_awk_t* awk)
{
	xp_cint_t c = awk->curc;

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

