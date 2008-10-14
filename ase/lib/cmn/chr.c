/*
 * $Id: ctype.c 132 2008-03-17 10:27:02Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/chr.h>

#if defined(ASE_CHAR_IS_MCHAR)

#include <ctype.h>

static ase_bool_t is_upper (ase_cint_t c) { return isupper(c); }
static ase_bool_t is_lower (ase_cint_t c) { return islower(c); }
static ase_bool_t is_alpha (ase_cint_t c) { return isalpha(c); }
static ase_bool_t is_digit (ase_cint_t c) { return isdigit(c); }
static ase_bool_t is_xdigit (ase_cint_t c) { return isxdigit(c); }
static ase_bool_t is_alnum (ase_cint_t c) { return isalnum(c); }
static ase_bool_t is_space (ase_cint_t c) { return isspace(c); }
static ase_bool_t is_print (ase_cint_t c) { return isprint(c); }
static ase_bool_t is_graph (ase_cint_t c) { return isgraph(c); }
static ase_bool_t is_cntrl (ase_cint_t c) { return iscntrl(c); }
static ase_bool_t is_punct (ase_cint_t c) { return ispunct(c); }

ase_bool_t ase_ccls_is (ase_cint_t c, int type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static ase_bool_t (*f[]) (ase_cint_t) = 
	{
		is_upper,
		is_lower,
		is_alpha,
		is_digit,
		is_xdigit,
		is_alnum,
		is_space,
		is_print,
		is_graph,
		is_cntrl,
		is_punct
	};

	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_PUNCT,
		"The character type should be one of ase_ccls_type_t values");
	return f[type] (c);
}

ase_cint_t ase_ccls_to (ase_cint_t c, int type)  
{ 
	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_LOWER,
		"The character type should be one of ASE_CCLS_UPPER and ASE_CCLS_LOWER");

	if (type == ASE_CCLS_UPPER) return toupper(c);
	if (type == ASE_CCLS_LOWER) return tolower(c);
	return c;
}

#elif defined(ASE_CHAR_IS_WCHAR)

#include <wctype.h>

ase_bool_t ase_ccls_is (ase_cint_t c, int type)
{ 
	static const char* name[] = 
	{
		"upper",
		"lower",
		"alpha",
		"digit",
		"xdigit",
		"alnum",
		"space",
		"print",
		"graph",
		"cntrl",
		"punct"
	};

	static wctype_t desc[] =
	{
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0
	};

	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_PUNCT,
		"The character type should be one of ase_ccls_type_t values");
		
	if (desc[type] == (wctype_t)0) desc[type] = wctype(name[type]);
	return iswctype (c, desc[type]);
}

ase_cint_t ase_ccls_to (ase_cint_t c, int type)  
{ 
	static const char* name[] = 
	{
		"toupper",
		"tolower"
	};

	static wctrans_t desc[] =
	{
		(wctrans_t)0,
		(wctrans_t)0
	};

	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_LOWER,
		"The character type should be one of ASE_CCLS_UPPER and ASE_CCLS_LOWER");

	if (desc[type] == (wctrans_t)0) desc[type] = wctrans(name[type]);
	return towctrans (c, desc[type]);
}

#else
	#error unsupported character type
#endif

static ase_bool_t ccls_is (void* data, ase_cint_t c, int type)
{
	return ase_ccls_is (c, type);
}

static ase_cint_t ccls_to (void* data, ase_cint_t c, int type)  
{
	return ase_ccls_to (c, type);
}

static ase_ccls_t ccls =
{
	ccls_is,
	ccls_to,
	ASE_NULL
};

ase_ccls_t* ase_ccls = &ccls;


#if 0
int ase_wctomb (ase_wchar_t wc, ase_mchar_t* mb, int mblen)
{
	if (mblen < MB_CUR_MAX) return -1;
	return wctomb (mb, wc);
}

ase_wchar_t ase_mbtowc (ase_mchar_t* mb, int mblen)
{
}
#endif
