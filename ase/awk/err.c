/*
 * $Id: err.c,v 1.23 2006-06-26 15:09:28 bacon Exp $
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
		XP_T("no error"),
		XP_T("out of memory"),
		XP_T("invalid parameter"),

		XP_T("no source io handler set"),
		XP_T("cannot open source input"),
		XP_T("cannot close source input"),
		XP_T("cannot switch to next source input"),
		XP_T("cannot read source input"),

		XP_T("cannot open text input"),
		XP_T("cannot close text input"),
		XP_T("cannot switch to next text input"),
		XP_T("cannot read text input"),

		XP_T("invalid character"),
		XP_T("cannot unget character"),

		XP_T("unexpected end of source"),
		XP_T("unexpected end of a string"),
		XP_T("unexpected end of a regular expression"),
		XP_T("left brace expected"),
		XP_T("left parenthesis expected"),
		XP_T("right parenthesis expected"),
		XP_T("right bracket expected"),
		XP_T("comma expected"),
		XP_T("semicolon expected"),
		XP_T("colon expected"),
		XP_T("keyword 'in' expected"),
		XP_T("not a variable after 'in'"),
		XP_T("expression expected"),

		XP_T("keyword 'while' expected"),
		XP_T("assignment statement expected"),
		XP_T("identifier expected"),
		XP_T("duplicate BEGIN"),
		XP_T("duplicate END"),
		XP_T("duplicate function name"),
		XP_T("duplicate parameter name"),
		XP_T("duplicate variable name"),
		XP_T("duplicate name"),
		XP_T("undefined identifier"),
		XP_T("l-value required"),
		XP_T("too few arguments"),
		XP_T("too many arguments"),
		XP_T("getline expected"),

		XP_T("divide by zero"),
		XP_T("invalid operand"),
		XP_T("no such function"),
		XP_T("value not assignable"),
		XP_T("value not indexable"),
		XP_T("wrong value type"),
		XP_T("pipe operation error"),
		XP_T("wrong implementation of user-defined io handler"),

		XP_T("internal error that should never have happened")
	};

	if (awk->errnum >= 0 && awk->errnum < xp_countof(__errstr)) {
		return __errstr[awk->errnum];
	}

	return XP_T("unknown error");
}
