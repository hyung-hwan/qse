/*
 * $Id: err.c,v 1.81 2007-03-04 06:45:43 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

static const ase_char_t* __geterrstr (int errnum)
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
		ASE_T("system too busy"),
		ASE_T("is a directory"),
		ASE_T("i/o error"),
		
		ASE_T("internal error that should never have happened"),
		ASE_T("general run-time error"),
		ASE_T("one or more running instances"),
		ASE_T("block nested too deeply"),
		ASE_T("expressio nested too deeply"),
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

		ASE_T("invalid character '%.*s'"),
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
		ASE_T("'%.*s' not a valid function name"),
		ASE_T("BEGIN requires an action block"),
		ASE_T("END requires an action block"),
		ASE_T("duplicate BEGIN"),
		ASE_T("duplicate END"),
		ASE_T("built-in function '%.*s' redefined"),
		ASE_T("function '%.*s' redefined"),
		ASE_T("global variable '%.*s' redefined"),
		ASE_T("parameter '%.*s' redefined"),
		ASE_T("duplicate parameter name '%.*s'"),
		ASE_T("duplicate global variable '%.*s'"),
		ASE_T("duplicate local variable '%.*s'"),
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

		ASE_T("divide by zero"),
		ASE_T("invalid operand"),
		ASE_T("wrong position index"),
		ASE_T("too few arguments"),
		ASE_T("too many arguments"),
		ASE_T("function '%.*s' not found"),
		ASE_T("variable not indexable"),
		ASE_T("variable '%.*s' not deletable"),
		ASE_T("value not a map"),
		ASE_T("value not referenceable"),
		ASE_T("value not assignable"),
		ASE_T("an indexed value cannot be assigned a map"),
		ASE_T("a positional value cannot be assigned a map"),
		ASE_T("map '%.*s' not assignable with a scalar"),
		ASE_T("cannot change a scalar value to a map"),
		ASE_T("a map is not allowed"),
		ASE_T("wrong value type"),
		ASE_T("next cannot be called from the BEGIN or END block"),
		ASE_T("nextfile cannot be called from the BEGIN or END block"),
		ASE_T("wrong implementation of built-in function handler"),
		ASE_T("built-in function handler returned an error"),
		ASE_T("wrong implementation of user-defined io handler"),
		ASE_T("no such io name found"),
		ASE_T("i/o handler returned an error"),
		ASE_T("invalid i/o name"),
		ASE_T("not sufficient arguments to formatting sequence"),
		ASE_T("recursion detected in format conversion"),
		ASE_T("invalid character in CONVFMT"),
		ASE_T("invalid character in OFMT"),

		ASE_T("recursion too deep in the regular expression"),
		ASE_T("a right parenthesis expected in the regular expression"),
		ASE_T("a right bracket expected in the regular expression"),
		ASE_T("a right brace expected in the regular expression"),
		ASE_T("unbalanced parenthesis"),
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

const ase_char_t* ase_awk_geterrstr (int errnum)
{
	return __geterrstr (errnum);
}


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
	if (errmsg != ASE_NULL) 
	{
		if (awk->errmsg[0] == ASE_T('\0'))
			*errmsg = ase_awk_geterrstr (awk->errnum);
		else
			*errmsg = awk->errmsg;
	}
}

void ase_awk_seterror (
	ase_awk_t* awk, int errnum, ase_size_t errlin,
	const ase_cstr_t* errarg, ase_size_t argcnt)
{
	const ase_char_t* errfmt;
	ase_size_t fmtlen;

	ASE_AWK_ASSERT (awk, argcnt <= 5);

	awk->errnum = errnum;
	awk->errlin = errlin;

	errfmt = __geterrstr (errnum);
	fmtlen = ase_strlen(errfmt);

	switch (argcnt)
	{
		case 0:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt);
			return;

		case 1:
		{
			ase_char_t tmp[ASE_COUNTOF(awk->errmsg)];
			ase_size_t len, tl;

			if (fmtlen < ASE_COUNTOF(awk->errmsg) &&
			    errarg[0].len + fmtlen >= ASE_COUNTOF(awk->errmsg))
			{
				len = ASE_COUNTOF(awk->errmsg) - fmtlen - 3 - 1;
				tl = ase_strxncpy (tmp, ASE_COUNTOF(tmp), errarg[0].ptr, len);
				tmp[tl] = ASE_T('.');
				tmp[tl+1] = ASE_T('.');
				tmp[tl+2] = ASE_T('.');
				len += 3;
			}
			else 
			{
				len = errarg[0].len;
				ase_strxncpy (tmp, ASE_COUNTOF(tmp), errarg[0].ptr, len);
			}

			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt, len, tmp);
			return;
		}

		case 2:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr);
			return;

		case 3:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr);
			return;

		case 4:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr);
			return;

		case 5:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr,
				errarg[4].len, errarg[4].ptr);
			return;
	}
}

void ase_awk_seterror_old (
	ase_awk_t* awk, int errnum, 
	ase_size_t errlin, const ase_char_t* errmsg)
{
	awk->errnum = errnum;
	awk->errlin = errlin;
	if (errmsg == ASE_NULL) awk->errmsg[0] = ASE_T('\0');
	else if (awk->errmsg != errmsg)
	{
		ase_strxcpy (
			awk->errmsg, ASE_COUNTOF(awk->errmsg), errmsg);
	}
}


int ase_awk_getrunerrnum (ase_awk_run_t* run)
{
	return run->errnum;
}

ase_size_t ase_awk_getrunerrlin (ase_awk_run_t* run)
{
	return run->errlin;
}

const ase_char_t* ase_awk_getrunerrmsg (ase_awk_run_t* run)
{
	if (run->errmsg[0] == ASE_T('\0')) 
		return ase_awk_geterrstr (run->errnum);

	return run->errmsg;
}

void ase_awk_setrunerrnum (ase_awk_run_t* run, int errnum)
{
	run->errnum = errnum;
	run->errlin = 0;
	run->errmsg[0] = ASE_T('\0');
}

void ase_awk_getrunerror (
	ase_awk_run_t* run, int* errnum, 
	ase_size_t* errlin, const ase_char_t** errmsg)
{
	if (errnum != ASE_NULL) *errnum = run->errnum;
	if (errlin != ASE_NULL) *errlin = run->errlin;
	if (errmsg != ASE_NULL) 
	{
		if (run->errmsg[0] == ASE_T('\0'))
			*errmsg = ase_awk_geterrstr (run->errnum);
		else
			*errmsg = run->errmsg;
	}
}

void ase_awk_setrunerror (
	ase_awk_run_t* run, int errnum, ase_size_t errlin,
	const ase_cstr_t* errarg, ase_size_t argcnt)
{
	const ase_char_t* errfmt;
	ase_size_t fmtlen;

	ASE_AWK_ASSERT (run->awk, argcnt <= 5);

	run->errnum = errnum;
	run->errlin = errlin;

	errfmt = __geterrstr (errnum);
	fmtlen = ase_strlen (errfmt);

	switch (argcnt)
	{
		case 0:
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt);
			return;

		case 1:
		{
			ase_char_t tmp[ASE_COUNTOF(run->errmsg)];
			ase_size_t len, tl;

			if (fmtlen < ASE_COUNTOF(run->errmsg) &&
			    errarg[0].len + fmtlen >= ASE_COUNTOF(run->errmsg))
			{
				len = ASE_COUNTOF(run->errmsg) - fmtlen - 3 - 1;
				tl = ase_strxncpy (tmp, ASE_COUNTOF(tmp), errarg[0].ptr, len);
				tmp[tl] = ASE_T('.');
				tmp[tl+1] = ASE_T('.');
				tmp[tl+2] = ASE_T('.');
				len += 3;
			}
			else 
			{
				len = errarg[0].len;
				ase_strxncpy (tmp, ASE_COUNTOF(tmp), errarg[0].ptr, len);
			}

			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt, len, tmp);
			return;
		}

		case 2:
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr);
			return;

		case 3:
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr);
			return;

		case 4:
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr);
			return;

		case 5:
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr,
				errarg[4].len, errarg[4].ptr);
			return;
	}
}

void ase_awk_setrunerror_old (
	ase_awk_run_t* run, int errnum, 
	ase_size_t errlin, const ase_char_t* errmsg)
{
	run->errnum = errnum;
	run->errlin = errlin;

	if (errmsg == ASE_NULL) run->errmsg[0] = ASE_T('\0');
	else if (errmsg != run->errmsg)
	{
		ase_strxcpy (run->errmsg, ASE_COUNTOF(run->errmsg), errmsg);
	}
}
