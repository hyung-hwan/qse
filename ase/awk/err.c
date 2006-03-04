/*
 * $Id: err.c,v 1.1 2006-03-04 10:08:13 bacon Exp $
 */

#include <xp/awk/awk.h>

int xp_awk_geterrnum (xp_awk_t* awk)
{
	return awk->errnum;
}

const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk)
{
	static const xp_char_t* __errstr[] =
 	{
		XP_TEXT("no error"),
		XP_TEXT("out of memory"),
		XP_TEXT("cannot open source"),
		XP_TEXT("cannot close source"),
		XP_TEXT("cannot read source"),
		XP_TEXT("invalid character"),
		XP_TEXT("cannot unget character"),
		XP_TEXT("unexpected end of source"),
		XP_TEXT("left brace expected"),
		XP_TEXT("left parenthesis expected"),
		XP_TEXT("right parenthesis expected"),
		XP_TEXT("right bracket expected"),
		XP_TEXT("comma expected"),
		XP_TEXT("semicolon expected"),
		XP_TEXT("expression expected"),
		XP_TEXT("keyword 'while' expected"),
		XP_TEXT("assignment statement expected"),
		XP_TEXT("identifier expected"),
		XP_TEXT("duplicate BEGIN"),
		XP_TEXT("duplicate END"),
		XP_TEXT("duplicate function name"),
		XP_TEXT("duplicate parameter name"),
		XP_TEXT("duplicate variable name"),
		XP_TEXT("duplicate name"),
		XP_TEXT("undefined identifier")	
	};

	if (awk->errnum >= 0 && awk->errnum < xp_countof(__errstr)) {
		return __errstr[awk->errnum];
	}

	return XP_TEXT("unknown error");
}
