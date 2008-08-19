/*
 * $Id: str_cnv.c 332 2008-08-18 11:21:48Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>

int ase_strtoi (const ase_char_t* str)
{
	int v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

long ase_strtol (const ase_char_t* str)
{
	long v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

unsigned int ase_strtoui (const ase_char_t* str)
{
	unsigned int v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

unsigned long ase_strtoul (const ase_char_t* str)
{
	unsigned long v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

int ase_strxtoi (const ase_char_t* str, ase_size_t len)
{
	int v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

long ase_strxtol (const ase_char_t* str, ase_size_t len)
{
	long v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

unsigned int ase_strxtoui (const ase_char_t* str, ase_size_t len)
{
	unsigned int v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

unsigned long ase_strxtoul (const ase_char_t* str, ase_size_t len)
{
	unsigned long v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_int_t ase_strtoint (const ase_char_t* str)
{
	ase_int_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_long_t ase_strtolong (const ase_char_t* str)
{
	ase_long_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_uint_t ase_strtouint (const ase_char_t* str)
{
	ase_uint_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_ulong_t ase_strtoulong (const ase_char_t* str)
{
	ase_ulong_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_int_t ase_strxtoint (const ase_char_t* str, ase_size_t len)
{
	ase_int_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_long_t ase_strxtolong (const ase_char_t* str, ase_size_t len)
{
	ase_long_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_uint_t ase_strxtouint (const ase_char_t* str, ase_size_t len)
{
	ase_uint_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_ulong_t ase_strxtoulong (const ase_char_t* str, ase_size_t len)
{
	ase_ulong_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}
