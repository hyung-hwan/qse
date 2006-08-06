/*
 * $Id: err.c,v 1.34 2006-08-06 15:02:55 bacon Exp $
 */

#include <xp/awk/awk_i.h>

int xp_awk_geterrnum (xp_awk_t* awk)
{
	return awk->errnum;
}

int xp_awk_getsuberrnum (xp_awk_t* awk)
{
	if (awk->errnum == XP_AWK_EREXBUILD ||
	    awk->errnum == XP_AWK_EREXMATCH)
	{
		return awk->suberrnum;
	}

	return XP_AWK_ENOERR;
}

const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk)
{
	static const xp_char_t* __errstr[] =
 	{
		XP_T("no error"),
		XP_T("out of memory"),
		XP_T("invalid parameter"),

		XP_T("cannot open source input"),
		XP_T("cannot close source input"),
		XP_T("cannot read source input"),

		XP_T("cannot open source output"),
		XP_T("cannot close source output"),
		XP_T("cannot write source output"),

		XP_T("cannot open console for read"),
		XP_T("cannot close console for read"),
		XP_T("cannot switch to next console for read"),
		XP_T("cannot read from console"),

		XP_T("cannot open console for write"),
		XP_T("cannot close console for write"),
		XP_T("cannot switch to next console for write"),
		XP_T("cannot write to console"),

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
		XP_T("BEGIN requires an action block"),
		XP_T("END requires an action block"),
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
		XP_T("too many global variables"),
		XP_T("too many local variables"),
		XP_T("too many parameters"),
		XP_T("break outside a loop"),
		XP_T("continue outside a loop"),
		XP_T("next illegal in BEGIN or END block"),
		XP_T("nextfile illegal in BEGIN or END block"),
		XP_T("getline expected"),
		XP_T("cannot build the regular expression"),
		XP_T("an error occurred in the regular expression match"),

		XP_T("divide by zero"),
		XP_T("invalid operand"),
		XP_T("wrong position index"),
		XP_T("no such function"),
		XP_T("value not assignable"),
		XP_T("variable not indexable"),
		XP_T("variable not deletable"),
		XP_T("variable not scalarizable"),
		XP_T("wrong value type"),
		XP_T("pipe operation error"),
		XP_T("next cannot be called from the BEGIN or END block"),
		XP_T("nextfile cannot be called from the BEGIN or END block"),
		XP_T("wrong implementation of user-defined io handler"),
		XP_T("internal error that should never have happened")
	};

	if (awk->errnum >= 0 && awk->errnum < xp_countof(__errstr)) 
	{
		return __errstr[awk->errnum];
	}

	return XP_T("unknown error");
}

const xp_char_t* xp_awk_getsuberrstr (xp_awk_t* awk)
{
	if (awk->errnum == XP_AWK_EREXBUILD ||
	    awk->errnum == XP_AWK_EREXMATCH)
	{
		return xp_awk_getrexerrstr (awk->suberrnum);
	}

	return XP_T("no error");
}
