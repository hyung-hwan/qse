/*
 * $Id: ctype.c,v 1.2 2007-02-23 08:17:51 bacon Exp $
 */

#include <ase/utl/ctype.h>

#if defined(ASE_CHAR_IS_MCHAR)

#include <ctype.h>

ase_bool_t ase_isupper (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return isupper (c); 
}

ase_bool_t ase_islower (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return islower (c); 
}

ase_bool_t ase_isalpha (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return isalpha (c); 
}

ase_bool_t ase_isdigit (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return isdigit (c); 
}

ase_bool_t ase_isxdigit (ase_ccls_t* ccls, ase_cint_t c) 
{ 
	return isxdigit (c); 
}

ase_bool_t ase_isalnum (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return isalnum (c); 
}

ase_bool_t ase_isspace (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return isspace (c); 
}

ase_bool_t ase_isprint (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return isprint (c); 
}

ase_bool_t ase_isgraph (ase_ccls_t* ccls, ase_cint_t c)
{
	return isgraph (c); 
}

ase_bool_t ase_iscntrl (ase_ccls_t* ccls, ase_cint_t c)
{
	return iscntrl (c);
}

ase_bool_t ase_ispunct (ase_ccls_t* ccls, ase_cint_t c)
{
	return ispunct (c);
}

ase_cint_t ase_toupper (ase_ccls_t* ccls, ase_cint_t c)
{
	return toupper (c);
}

ase_cint_t ase_tolower (ase_ccls_t* ccls, ase_cint_t c)
{
	return tolower (c);
}

#elif defined(ASE_CHAR_IS_WCHAR)

#include <wctype.h>

ase_bool_t ase_isupper (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return iswupper (c); 
}

ase_bool_t ase_islower (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return iswlower (c); 
}

ase_bool_t ase_isalpha (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return iswalpha (c); 
}

ase_bool_t ase_isdigit (ase_ccls_t* ccls, ase_cint_t c)  
{ 
	return iswdigit (c); 
}

ase_bool_t ase_isxdigit (ase_ccls_t* ccls, ase_cint_t c) 
{ 
	return iswxdigit (c); 
}

ase_bool_t ase_isalnum (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return iswalnum (c); 
}

ase_bool_t ase_isspace (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return iswspace (c); 
}

ase_bool_t ase_isprint (ase_ccls_t* ccls, ase_cint_t c)
{ 
	return iswprint (c); 
}

ase_bool_t ase_isgraph (ase_ccls_t* ccls, ase_cint_t c)
{
	return iswgraph (c); 
}

ase_bool_t ase_iscntrl (ase_ccls_t* ccls, ase_cint_t c)
{
	return iswcntrl (c);
}

ase_bool_t ase_ispunct (ase_ccls_t* ccls, ase_cint_t c)
{
	return iswpunct (c);
}

ase_cint_t ase_toupper (ase_ccls_t* ccls, ase_cint_t c)
{
	return towupper (c);
}

ase_cint_t ase_tolower (ase_ccls_t* ccls, ase_cint_t c)
{
	return towlower (c);
}

#else

#error unsupported character type

#endif
