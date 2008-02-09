/*
 * $Id: err.c,v 1.14 2007/11/17 11:25:55 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

static const ase_char_t* __geterrstr (ase_awk_errnum_t errnum)
{
	static const ase_char_t* errstr[] =
 	{
		ASE_T("no error"),
		ASE_T("custom error"),

		ASE_T("invalid parameter or data"),
		ASE_T("out of memory"),
		ASE_T("not supported"),
		ASE_T("operation not allowed"),
		ASE_T("no such device"),
		ASE_T("no space left on device"),
		ASE_T("too many open files"),
		ASE_T("too many links"),
		ASE_T("resource temporarily unavailable"),
		ASE_T("'%.*s' not existing"),
		ASE_T("'%.*s' already exists"),
		ASE_T("file or data too big"),
		ASE_T("system too busy"),
		ASE_T("is a directory"),
		ASE_T("i/o error"),

		ASE_T("cannot open '%.*s'"),
		ASE_T("cannot read '%.*s'"),
		ASE_T("cannot write '%.*s'"),
		ASE_T("cannot close '%.*s'"),
		
		ASE_T("internal error that should never have happened"),
		ASE_T("general run-time error"),
		ASE_T("block nested too deeply"),
		ASE_T("expressio nested too deeply"),

		ASE_T("cannot open source input"),
		ASE_T("cannot close source input"),
		ASE_T("cannot read source input"),

		ASE_T("cannot open source output"),
		ASE_T("cannot close source output"),
		ASE_T("cannot write source output"),

		ASE_T("invalid character '%.*s'"),
		ASE_T("invalid digit '%.*s'"),
		ASE_T("cannot unget character"),

		ASE_T("unexpected end of source"),
		ASE_T("a comment not closed properly"),
		ASE_T("a string not closed with a quote"),
		ASE_T("unexpected end of a regular expression"),
		ASE_T("a left brace expected in place of '%.*s'"),
		ASE_T("a left parenthesis expected in place of '%.*s'"),
		ASE_T("a right parenthesis expected in place of '%.*s'"),
		ASE_T("a right bracket expected in place of '%.*s'"),
		ASE_T("a comma expected in place of '%.*s'"),
		ASE_T("a semicolon expected in place of '%.*s'"),
		ASE_T("a colon expected in place of '%.*s'"),
		ASE_T("statement not ending with a semicolon"),
		ASE_T("'in' expected in place of '%.*s'"),
		ASE_T("right-hand side of the 'in' operator not a variable"),
		ASE_T("invalid expression"),

		ASE_T("keyword 'function' expected in place of '%.*s'"),
		ASE_T("keyword 'while' expected in place of '%.*s'"),
		ASE_T("invalid assignment statement"),
		ASE_T("an identifier expected in place of '%.*s'"),
		ASE_T("'%.*s' not a valid function name"),
		ASE_T("BEGIN not followed by a left bracket on the same line"),
		ASE_T("END not followed by a left bracket on the same line"),
		ASE_T("duplicate BEGIN"),
		ASE_T("duplicate END"),
		ASE_T("intrinsic function '%.*s' redefined"),
		ASE_T("function '%.*s' redefined"),
		ASE_T("global variable '%.*s' redefined"),
		ASE_T("parameter '%.*s' redefined"),
		ASE_T("variable '%.*s' redefined"),
		ASE_T("duplicate parameter name '%.*s'"),
		ASE_T("duplicate global variable '%.*s'"),
		ASE_T("duplicate local variable '%.*s'"),
		ASE_T("'%.*s' not a valid parameter name"),
		ASE_T("'%.*s' not a valid variable name"),
		ASE_T("undefined identifier '%.*s'"),
		ASE_T("l-value required"),
		ASE_T("too many global variables"),
		ASE_T("too many local variables"),
		ASE_T("too many parameters"),
		ASE_T("delete statement not followed by a normal variable"),
		ASE_T("reset statement not followed by a normal variable"),
		ASE_T("break statement outside a loop"),
		ASE_T("continue statement outside a loop"),
		ASE_T("next statement illegal in the BEGIN block"),
		ASE_T("next statement illegal in the END block"),
		ASE_T("nextfile statement illegal in the BEGIN block"),
		ASE_T("nextfile statement illegal in the END block"),
		ASE_T("printf not followed by any arguments"),
		ASE_T("both prefix and postfix increment/decrement operator present"),
		ASE_T("coprocess not supported by getline"),

		ASE_T("divide by zero"),
		ASE_T("invalid operand"),
		ASE_T("wrong position index"),
		ASE_T("too few arguments"),
		ASE_T("too many arguments"),
		ASE_T("function '%.*s' not found"),
		ASE_T("variable not indexable"),
		ASE_T("variable '%.*s' not deletable"),
		ASE_T("value not a map"),
		ASE_T("right-hand side of the 'in' operator not a map"),
		ASE_T("right-hand side of the 'in' operator not a map nor nil"),
		ASE_T("value not referenceable"),
		ASE_T("value not assignable"),
		ASE_T("an indexed value cannot be assigned a map"),
		ASE_T("a positional value cannot be assigned a map"),
		ASE_T("map '%.*s' not assignable with a scalar"),
		ASE_T("cannot change a scalar value to a map"),
		ASE_T("a map is not allowed"),
		ASE_T("invalid value type"),
		ASE_T("delete statement called with a wrong target"),
		ASE_T("reset statement called with a wrong target"),
		ASE_T("next statement called from the BEGIN block"),
		ASE_T("next statement called from the END block"),
		ASE_T("nextfile statement called from the BEGIN block"),
		ASE_T("nextfile statement called from the END block"),
		ASE_T("wrong implementation of intrinsic function handler"),
		ASE_T("intrinsic function handler returned an error"),
		ASE_T("wrong implementation of user-defined io handler"),
		ASE_T("no such io name found"),
		ASE_T("i/o handler returned an error"),
		ASE_T("i/o name empty"),
		ASE_T("i/o name containing a null character"),
		ASE_T("not sufficient arguments to formatting sequence"),
		ASE_T("recursion detected in format conversion"),
		ASE_T("invalid character in CONVFMT"),
		ASE_T("invalid character in OFMT"),

		ASE_T("recursion too deep in the regular expression"),
		ASE_T("a right parenthesis expected in the regular expression"),
		ASE_T("a right bracket expected in the regular expression"),
		ASE_T("a right brace expected in the regular expression"),
		ASE_T("unbalanced parenthesis in the regular expression"),
		ASE_T("a colon expected in the regular expression"),
		ASE_T("invalid character range in the regular expression"),
		ASE_T("invalid character class in the regular expression"),
		ASE_T("invalid boundary range in the regular expression"),
		ASE_T("unexpected end of the regular expression"),
		ASE_T("garbage after the regular expression")
	};

	if (errnum >= 0 && errnum < ASE_COUNTOF(errstr)) 
	{
		return errstr[errnum];
	}

	return ASE_T("unknown error");
}

const ase_char_t* ase_awk_geterrstr (ase_awk_t* awk, int num)
{
	if (awk != ASE_NULL && 
	    awk->errstr[num] != ASE_NULL) return awk->errstr[num];
	return __geterrstr (num);
}

int ase_awk_seterrstr (ase_awk_t* awk, int num, const ase_char_t* str)
{
	ase_char_t* dup;
       
	if (str == ASE_NULL) dup = ASE_NULL;
	else
	{
		dup = ase_strdup (str, &awk->prmfns.mmgr);
		if (dup == ASE_NULL) return -1;
	}

	if (awk->errstr[num] != ASE_NULL) 
		ASE_AWK_FREE (awk, awk->errstr[num]);
	else awk->errstr[num] = dup;
	return 0;
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
		return ase_awk_geterrstr (awk, awk->errnum);
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
			*errmsg = ase_awk_geterrstr (awk, awk->errnum);
		else
			*errmsg = awk->errmsg;
	}
}

void ase_awk_seterrnum (ase_awk_t* awk, ase_awk_errnum_t errnum)
{
	awk->errnum = errnum;
	awk->errlin = 0;
	awk->errmsg[0] = ASE_T('\0');
}

void ase_awk_seterrmsg (ase_awk_t* awk, 
	ase_awk_errnum_t errnum, ase_size_t errlin, const ase_char_t* errmsg)
{
	awk->errnum = errnum;
	awk->errlin = errlin;
	ase_strxcpy (awk->errmsg, ASE_COUNTOF(awk->errmsg), errmsg);
}

void ase_awk_seterror (
	ase_awk_t* awk, ase_awk_errnum_t errnum, ase_size_t errlin,
	const ase_cstr_t* errarg, ase_size_t argcnt)
{
	const ase_char_t* errfmt;
	ase_size_t fmtlen;

	ASE_ASSERT (argcnt <= 5);

	awk->errnum = errnum;
	awk->errlin = errlin;

	errfmt = ase_awk_geterrstr (awk, errnum);
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
				errfmt, (int)len, tmp);
			return;
		}

		case 2:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr);
			return;

		case 3:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr);
			return;

		case 4:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr,
				(int)errarg[3].len, errarg[3].ptr);
			return;

		case 5:
			awk->prmfns.misc.sprintf (
				awk->prmfns.misc.custom_data,
				awk->errmsg, 
				ASE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr,
				(int)errarg[3].len, errarg[3].ptr,
				(int)errarg[4].len, errarg[4].ptr);
			return;
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
		return ase_awk_geterrstr (run->awk, run->errnum);

	return run->errmsg;
}

void ase_awk_setrunerrnum (ase_awk_run_t* run, ase_awk_errnum_t errnum)
{
	run->errnum = errnum;
	run->errlin = 0;
	run->errmsg[0] = ASE_T('\0');
}

void ase_awk_setrunerrmsg (ase_awk_run_t* run, 
	ase_awk_errnum_t errnum, ase_size_t errlin, const ase_char_t* errmsg)
{
	run->errnum = errnum;
	run->errlin = errlin;
	ase_strxcpy (run->errmsg, ASE_COUNTOF(run->errmsg), errmsg);
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
			*errmsg = ase_awk_geterrstr (run->awk, run->errnum);
		else
			*errmsg = run->errmsg;
	}
}

void ase_awk_setrunerror (
	ase_awk_run_t* run, ase_awk_errnum_t errnum, ase_size_t errlin,
	const ase_cstr_t* errarg, ase_size_t argcnt)
{
	const ase_char_t* errfmt;
	ase_size_t fmtlen;

	ASE_ASSERT (argcnt <= 5);

	run->errnum = errnum;
	run->errlin = errlin;

	errfmt = ase_awk_geterrstr (run->awk, errnum);
	fmtlen = ase_strlen (errfmt);

	switch (argcnt)
	{
		case 0:
			/* TODO: convert % to %% if the original % is not
			 *       the first % of the %% sequence */
			run->awk->prmfns.misc.sprintf (
				run->awk->prmfns.misc.custom_data,
				run->errmsg, 
				ASE_COUNTOF(run->errmsg), 
				errfmt);
			return;

		case 1:
		{
			/* TODO: what if the argument contains a null character? 
			 *       handle the case more gracefully than now... */

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
