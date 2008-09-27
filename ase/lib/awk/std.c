/*
 * $Id$
 */

#include "awk.h"
#include <ase/utl/stdio.h>
#include <math.h>

typedef struct ext_t
{
	ase_awk_prmfns_t prmfns;
} 
ext_t;

static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int custom_awk_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static void custom_awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

ase_awk_t* ase_awk_openstd (void)
{
	ase_awk_t* awk;
	ext_t* ext;

	awk = ase_awk_open (ASE_MMGR_GETDFL(), ASE_SIZEOF(ext_t));
	ase_awk_setccls (awk, ASE_CCLS_GETDFL());

	ext = (ext_t*)ASE_AWK_EXTENSION(awk);
	ext->prmfns.pow     = custom_awk_pow;
	ext->prmfns.sprintf = custom_awk_sprintf;
	ext->prmfns.dprintf = custom_awk_dprintf;
	ext->prmfns.data    = ASE_NULL;
	ase_awk_setprmfns (awk, &ext->prmfns);

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	/*ase_awk_addfunction ();*/

	return awk;
}
