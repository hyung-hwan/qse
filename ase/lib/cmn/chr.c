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

static ase_bool_t ccls_is (void* data, ase_cint_t c, ase_ccls_type_t type)
{ 
	static wctype_t desc[] =
	{
		wctype("upper"),
		wctype("lower"),
		wctype("alpha"),
		wctype("digit"),
		wctype("xdigit"),
		wctype("alnum"),
		wctype("space"),
		wctype("print"),
		wctype("graph"),
		wctype("cntrl"),
		wctype("punct")
	};

	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_PUNCT,
		"The character type should be one of ase_ccls_type_t values");
		
	return iswctype (c, desc[type]);
}

static ase_cint_t ccls_to (void* data, ase_cint_t c, in type)  
{ 
	static wctype_t desc[] =
	{
		wctrans("toupper"),
		wctrans("tolower")
	};

	ASE_ASSERTX (type >= ASE_CCLS_UPPER && type <= ASE_CCLS_LOWER,
		"The character type should be one of ASE_CCLS_UPPER and ASE_CCLS_LOWER");

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
