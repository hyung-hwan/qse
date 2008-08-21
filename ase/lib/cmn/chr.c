/*
 * $Id: ctype.c 132 2008-03-17 10:27:02Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/chr.h>

#if defined(ASE_CHAR_IS_MCHAR)

#include <ctype.h>

static ase_bool_t ccls_is (void* data, ase_cint_t c, ase_ccls_type_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */
#error NOT IMPLEMENTED YET.
}

static ase_cint_t ccls_to (void* data, ase_cint_t c, in type)  
{ 
	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_LOWER,
		"The character type should be one of ASE_CCLS_UPPER and ASE_CCLS_LOWER");

	if (type == ASE_CCLS_UPPER) return toupper(c);
	if (type == ASE_CCLS_LOWER) return tolower(c);
	return c;
}

#elif defined(ASE_CHAR_IS_WCHAR)

#include <wctype.h>

static ase_bool_t ccls_is (void* data, ase_cint_t c, int type)
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

static ase_cint_t ccls_to (void* data, ase_cint_t c, int type)  
{ 
	static const char* name[] = 
	{
		"upper",
		"lower"
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

static ase_ccls_t ccls =
{
	ccls_is,
	ccls_to,
	ASE_NULL
};

ase_ccls_t* ase_ccls = &ccls;
