/*
 * $Id: str_cnv.c 141 2008-03-18 04:09:04Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

ase_int_t ase_strtoint (const ase_char_t* str)
{
	ase_int_t v;
	ASE_STRTOI (v, str, ASE_NULL, 10);
	return v;
}

ase_long_t ase_strtolong (const ase_char_t* str)
{
	ase_long_t v;
	ASE_STRTOI (v, str, ASE_NULL, 10);
	return v;
}

ase_uint_t ase_strtouint (const ase_char_t* str)
{
	ase_uint_t v;
	ASE_STRTOI (v, str, ASE_NULL, 10);
	return v;
}

ase_ulong_t ase_strtoulong (const ase_char_t* str)
{
	ase_ulong_t v;
	ASE_STRTOI (v, str, ASE_NULL, 10);
	return v;
}
