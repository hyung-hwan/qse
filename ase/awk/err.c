/*
 * $Id: err.c,v 1.44 2006-10-22 12:39:29 bacon Exp $
 */

#include <sse/awk/awk_i.h>

int sse_awk_geterrnum (sse_awk_t* awk)
{
	return awk->errnum;
}

const sse_char_t* sse_awk_geterrstr (int errnum)
{
	static const sse_char_t* __errstr[] =
 	{
		SSE_T("no error"),
		SSE_T("out of memory"),
		SSE_T("invalid parameter"),
		SSE_T("general run-time error"),
		SSE_T("one or more running instances"),
		SSE_T("too many running instances"),
		SSE_T("recursion too deep"),

		SSE_T("cannot open source input"),
		SSE_T("cannot close source input"),
		SSE_T("cannot read source input"),

		SSE_T("cannot open source output"),
		SSE_T("cannot close source output"),
		SSE_T("cannot write source output"),

		SSE_T("cannot open console for read"),
		SSE_T("cannot close console for read"),
		SSE_T("cannot switch to next console for read"),
		SSE_T("cannot read from console"),

		SSE_T("cannot open console for write"),
		SSE_T("cannot close console for write"),
		SSE_T("cannot switch to next console for write"),
		SSE_T("cannot write to console"),

		SSE_T("invalid character"),
		SSE_T("cannot unget character"),

		SSE_T("unexpected end of source"),
		SSE_T("unexpected end of a comment"),
		SSE_T("unexpected end of a string"),
		SSE_T("unexpected end of a regular expression"),
		SSE_T("left brace expected"),
		SSE_T("left parenthesis expected"),
		SSE_T("right parenthesis expected"),
		SSE_T("right bracket expected"),
		SSE_T("comma expected"),
		SSE_T("semicolon expected"),
		SSE_T("colon expected"),
		SSE_T("keyword 'in' expected"),
		SSE_T("not a variable after 'in'"),
		SSE_T("expression expected"),

		SSE_T("keyword 'while' expected"),
		SSE_T("assignment statement expected"),
		SSE_T("identifier expected"),
		SSE_T("BEGIN requires an action block"),
		SSE_T("END requires an action block"),
		SSE_T("duplicate BEGIN"),
		SSE_T("duplicate END"),
		SSE_T("duplicate function name"),
		SSE_T("duplicate parameter name"),
		SSE_T("duplicate variable name"),
		SSE_T("duplicate name"),
		SSE_T("undefined identifier"),
		SSE_T("l-value required"),
		SSE_T("too few arguments"),
		SSE_T("too many arguments"),
		SSE_T("too many global variables"),
		SSE_T("too many local variables"),
		SSE_T("too many parameters"),
		SSE_T("break outside a loop"),
		SSE_T("continue outside a loop"),
		SSE_T("next illegal in BEGIN or END block"),
		SSE_T("nextfile illegal in BEGIN or END block"),
		SSE_T("getline expected"),

		SSE_T("divide by zero"),
		SSE_T("invalid operand"),
		SSE_T("wrong position index"),
		SSE_T("no such function"),
		SSE_T("value not assignable"),
		SSE_T("variable not indexable"),
		SSE_T("variable not deletable"),
		SSE_T("value not referenceable"),
		SSE_T("an indexed value cannot be assigned a map"),
		SSE_T("a positional value cannot be assigned a map"),
		SSE_T("cannot change a map to a scalar value"),
		SSE_T("cannot change a scalar value to a map"),
		SSE_T("a map is not allowed"),
		SSE_T("wrong value type"),
		SSE_T("pipe operation error"),
		SSE_T("next cannot be called from the BEGIN or END block"),
		SSE_T("nextfile cannot be called from the BEGIN or END block"),
		SSE_T("wrong implementation of user-defined io handler"),
		SSE_T("no such io name found"),
		SSE_T("io handler has returned an error"),
		SSE_T("internal error that should never have happened"),

		SSE_T("a right parenthesis is expected in the regular expression"),
		SSE_T("a right bracket is expected in the regular expression"),
		SSE_T("a right brace is expected in the regular expression"),
		SSE_T("a colon is expected in the regular expression"),
		SSE_T("invalid character range in the regular expression"),
		SSE_T("invalid character class in the regular expression"),
		SSE_T("invalid boundary range in the regular expression"),
		SSE_T("unexpected end of the regular expression"),
		SSE_T("garbage after the regular expression")
	};

	if (errnum >= 0 && errnum < sse_countof(__errstr)) 
	{
		return __errstr[errnum];
	}

	return SSE_T("unknown error");
}

