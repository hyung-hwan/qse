/*
 * $Id: err.c,v 1.67 2006-12-30 08:54:43 bacon Exp $
 */

#include <ase/awk/awk_i.h>

int ase_awk_geterrnum (ase_awk_t* awk)
{
	return awk->errnum;
}

ase_size_t ase_awk_geterrlin (ase_awk_t* awk)
{
	return awk->errlin;
}

const ase_char_t* ase_awk_geterrmsg (ase_awk_t* awk)
{
	if (awk->errmsg[0] == ASE_T('\0')) 
		return ase_awk_geterrstr (awk->errnum);
	return awk->errmsg;
}


void ase_awk_geterror (
	ase_awk_t* awk, int* errnum, 
	ase_size_t* errlin, const ase_char_t** errmsg)
{
	if (errnum != ASE_NULL) *errnum = awk->errnum;
	if (errlin != ASE_NULL) *errlin = awk->errlin;
	if (errmsg != ASE_NULL) *errmsg = awk->errmsg;
}

void ase_awk_seterror (
	ase_awk_t* awk, int errnum, 
	ase_size_t errlin, const ase_char_t* errmsg)
{
	awk->errnum = errnum;
	awk->errlin = errlin;
	if (errmsg == ASE_NULL) awk->errmsg[0] = ASE_T('\0');
	else if (awk->errmsg != errmsg)
	{
		ase_awk_strxcpy (
			awk->errmsg, ASE_COUNTOF(awk->errmsg), errmsg);
	}
}

const ase_char_t* ase_awk_geterrstr (int errnum)
{
	static const ase_char_t* __errstr[] =
 	{
		ASE_T("no error"),

		ASE_T("invalid parameter"),
		ASE_T("out of memory"),
		ASE_T("not supported"),
		ASE_T("operation not allowed"),
		ASE_T("no such device"),
		ASE_T("no space left on device"),
		ASE_T("no such file, directory, or data"),
		ASE_T("too many open files"),
		ASE_T("too many links"),
		ASE_T("resource temporarily unavailable"),
		ASE_T("file or data exists"),
		ASE_T("file or data too big"),
		ASE_T("operation interrupted"),
		ASE_T("system too busy"),
		ASE_T("is a directory"),
		ASE_T("i/o error"),
		
		ASE_T("general run-time error"),
		ASE_T("one or more running instances"),
		ASE_T("too many running instances"),
		ASE_T("recursion too deep"),
		ASE_T("system functions not provided or not proper"),

		ASE_T("cannot open source input"),
		ASE_T("cannot close source input"),
		ASE_T("cannot read source input"),

		ASE_T("cannot open source output"),
		ASE_T("cannot close source output"),
		ASE_T("cannot write source output"),

		ASE_T("cannot open console for read"),
		ASE_T("cannot close console for read"),
		ASE_T("cannot switch to next console for read"),
		ASE_T("cannot read from console"),

		ASE_T("cannot open console for write"),
		ASE_T("cannot close console for write"),
		ASE_T("cannot switch to next console for write"),
		ASE_T("cannot write to console"),

		ASE_T("invalid character"),
		ASE_T("cannot unget character"),

		ASE_T("unexpected end of source"),
		ASE_T("unexpected end of a comment"),
		ASE_T("unexpected end of a string"),
		ASE_T("unexpected end of a regular expression"),
		ASE_T("left brace expected"),
		ASE_T("left parenthesis expected"),
		ASE_T("right parenthesis expected"),
		ASE_T("right bracket expected"),
		ASE_T("comma expected"),
		ASE_T("semicolon expected"),
		ASE_T("colon expected"),
		ASE_T("keyword 'in' expected"),
		ASE_T("not a variable after 'in'"),
		ASE_T("expression expected"),

		ASE_T("keyword 'while' expected"),
		ASE_T("assignment statement expected"),
		ASE_T("identifier expected"),
		ASE_T("BEGIN requires an action block"),
		ASE_T("END requires an action block"),
		ASE_T("duplicate BEGIN"),
		ASE_T("duplicate END"),
		ASE_T("built-in function redefined"),
		ASE_T("function redefined"),
		ASE_T("global variable redefined"),
		ASE_T("parameter redefined"),
		ASE_T("duplicate parameter name"),
		ASE_T("duplicate global variable name"),
		ASE_T("duplicate local variable name"),
		ASE_T("undefined identifier"),
		ASE_T("l-value required"),
		ASE_T("too many global variables"),
		ASE_T("too many local variables"),
		ASE_T("too many parameters"),
		ASE_T("break outside a loop"),
		ASE_T("continue outside a loop"),
		ASE_T("next illegal in BEGIN or END block"),
		ASE_T("nextfile illegal in BEGIN or END block"),
		ASE_T("getline expected"),
		ASE_T("printf requires one or more arguments"),

		ASE_T("internal error that should never have happened"),
		ASE_T("divide by zero"),
		ASE_T("invalid operand"),
		ASE_T("wrong position index"),
		ASE_T("too few arguments"),
		ASE_T("too many arguments"),
		ASE_T("no such function"),
		ASE_T("variable not indexable"),
		ASE_T("variable not deletable"),
		ASE_T("value not a map"),
		ASE_T("value not referenceable"),
		ASE_T("value not assignable"),
		ASE_T("an indexed value cannot be assigned a map"),
		ASE_T("a positional value cannot be assigned a map"),
		ASE_T("cannot change a map to a scalar value"),
		ASE_T("cannot change a scalar value to a map"),
		ASE_T("a map is not allowed"),
		ASE_T("wrong value type"),
		ASE_T("next cannot be called from the BEGIN or END block"),
		ASE_T("nextfile cannot be called from the BEGIN or END block"),
		ASE_T("wrong implementation of user-defined io handler"),
		ASE_T("wrong implementation of built-in function handler"),
		ASE_T("built-in function handler returned an error"),
		ASE_T("no such io name found"),
		ASE_T("i/o handler returned an error"),
		ASE_T("invalid i/o name"),
		ASE_T("not sufficient arguments to formatting sequence"),
		ASE_T("recursion detected in format conversion"),
		ASE_T("invalid character in CONVFMT"),
		ASE_T("invalid character in OFMT"),

		ASE_T("a right parenthesis expected in the regular expression"),
		ASE_T("a right bracket expected in the regular expression"),
		ASE_T("a right brace expected in the regular expression"),
		ASE_T("a colon expected in the regular expression"),
		ASE_T("invalid character range in the regular expression"),
		ASE_T("invalid character class in the regular expression"),
		ASE_T("invalid boundary range in the regular expression"),
		ASE_T("unexpected end of the regular expression"),
		ASE_T("garbage after the regular expression")
	};

	if (errnum >= 0 && errnum < ASE_COUNTOF(__errstr)) 
	{
		return __errstr[errnum];
	}

	return ASE_T("unknown error");
}

