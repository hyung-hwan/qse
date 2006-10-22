/*
 * $Id: err.c,v 1.43 2006-10-22 11:34:53 bacon Exp $
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

		SSE_T("unesseected end of source"),
		SSE_T("unesseected end of a comment"),
		SSE_T("unesseected end of a string"),
		SSE_T("unesseected end of a regular esseression"),
		SSE_T("left brace esseected"),
		SSE_T("left parenthesis esseected"),
		SSE_T("right parenthesis esseected"),
		SSE_T("right bracket esseected"),
		SSE_T("comma esseected"),
		SSE_T("semicolon esseected"),
		SSE_T("colon esseected"),
		SSE_T("keyword 'in' esseected"),
		SSE_T("not a variable after 'in'"),
		SSE_T("esseression esseected"),

		SSE_T("keyword 'while' esseected"),
		SSE_T("assignment statement esseected"),
		SSE_T("identifier esseected"),
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
		SSE_T("getline esseected"),

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

		SSE_T("a right parenthesis is esseected in the regular esseression"),
		SSE_T("a right bracket is esseected in the regular esseression"),
		SSE_T("a right brace is esseected in the regular esseression"),
		SSE_T("a colon is esseected in the regular esseression"),
		SSE_T("invalid character range in the regular esseression"),
		SSE_T("invalid character class in the regular esseression"),
		SSE_T("invalid boundary range in the regular esseression"),
		SSE_T("unesseected end of the regular esseression"),
		SSE_T("garbage after the regular esseression")
	};

	if (errnum >= 0 && errnum < sse_countof(__errstr)) 
	{
		return __errstr[errnum];
	}

	return SSE_T("unknown error");
}

