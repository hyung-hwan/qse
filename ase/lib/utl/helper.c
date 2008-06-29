
/*
 * $Id: helper.c 231 2008-06-28 08:37:09Z baconevi $
 */

#include <ase/utl/helper.h>
#include <ase/utl/ctype.h>
#include <stdlib.h>

static void* mmgr_malloc (void* custom, ase_size_t n)
{
        return malloc (n);
}

static void* mmgr_realloc (void* custom, void* ptr, ase_size_t n)
{
        return realloc (ptr, n);
}

static void mmgr_free (void* custom, void* ptr)
{
        free (ptr);
}

static ase_bool_t ccls_isupper (void* custom, ase_cint_t c)  
{ 
	return ase_isupper (c); 
}

static ase_bool_t ccls_islower (void* custom, ase_cint_t c)  
{ 
	return ase_islower (c); 
}

static ase_bool_t ccls_isalpha (void* custom, ase_cint_t c)  
{ 
	return ase_isalpha (c); 
}

static ase_bool_t ccls_isdigit (void* custom, ase_cint_t c)  
{ 
	return ase_isdigit (c); 
}

static ase_bool_t ccls_isxdigit (void* custom, ase_cint_t c) 
{ 
	return ase_isxdigit (c); 
}

static ase_bool_t ccls_isalnum (void* custom, ase_cint_t c)
{ 
	return ase_isalnum (c); 
}

static ase_bool_t ccls_isspace (void* custom, ase_cint_t c)
{ 
	return ase_isspace (c); 
}

static ase_bool_t ccls_isprint (void* custom, ase_cint_t c)
{ 
	return ase_isprint (c); 
}

static ase_bool_t ccls_isgraph (void* custom, ase_cint_t c)
{
	return ase_isgraph (c); 
}

static ase_bool_t ccls_iscntrl (void* custom, ase_cint_t c)
{
	return ase_iscntrl (c);
}

static ase_bool_t ccls_ispunct (void* custom, ase_cint_t c)
{
	return ase_ispunct (c);
}

static ase_cint_t ccls_toupper (void* custom, ase_cint_t c)
{
	return ase_toupper (c);
}

static ase_cint_t ccls_tolower (void* custom, ase_cint_t c)
{
	return ase_tolower (c);
}

static ase_mmgr_t mmgr =
{
	mmgr_malloc,
	mmgr_realloc,
	mmgr_free,
	ASE_NULL
};

static ase_ccls_t ccls =
{
	ccls_isupper,
	ccls_islower,
	ccls_isalpha,
	ccls_isdigit,
	ccls_isxdigit,
	ccls_isalnum,
	ccls_isspace,
	ccls_isprint,
	ccls_isgraph,
	ccls_iscntrl,
	ccls_ispunct,
	ccls_toupper,
	ccls_tolower,
	ASE_NULL
};

ase_mmgr_t* ase_mmgr = &mmgr;
ase_ccls_t* ase_ccls = &ccls;
