/*
 * $Id: err.c,v 1.17 2006-04-30 15:50:38 bacon Exp $
 */

#include <xp/awk/awk_i.h>

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

		XP_TEXT("no source io handler set"),
		XP_TEXT("cannot open source input"),
		XP_TEXT("cannot close source input"),
		XP_TEXT("cannot switch to next source input"),
		XP_TEXT("cannot read source input"),

		XP_TEXT("cannot open text input"),
		XP_TEXT("cannot close text input"),
		XP_TEXT("cannot switch to next text input"),
		XP_TEXT("cannot read text input"),

		XP_TEXT("invalid character"),
		XP_TEXT("cannot unget character"),

		XP_TEXT("unexpected end of source"),
		XP_TEXT("unexpected end of a string"),
		XP_TEXT("unexpected end of a regular expression"),
		XP_TEXT("left brace expected"),
		XP_TEXT("left parenthesis expected"),
		XP_TEXT("right parenthesis expected"),
		XP_TEXT("right bracket expected"),
		XP_TEXT("comma expected"),
		XP_TEXT("semicolon expected"),
		XP_TEXT("colon expected"),
		XP_TEXT("keyword 'in' expected"),
		XP_TEXT("not a variable after 'in'"),
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
		XP_TEXT("undefined identifier"),
		XP_TEXT("l-value required"),
		XP_TEXT("too many arguments"),

		XP_TEXT("divide by zero"),
		XP_TEXT("invalid operand"),
		XP_TEXT("no such function"),
		XP_TEXT("value not assignable"),
		XP_TEXT("value not indexable"),
		XP_TEXT("wrong index value"),

		XP_TEXT("internal error that should never have happened")
	};

	if (awk->errnum >= 0 && awk->errnum < xp_countof(__errstr)) {
		return __errstr[awk->errnum];
	}

	return XP_TEXT("unknown error");
}
