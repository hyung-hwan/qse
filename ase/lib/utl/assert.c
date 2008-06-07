/*
 * $Id$
 */

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#ifndef NDEBUG

#include <ase/cmn/stdio.h>
#include <stdlib.h>

int ase_assert_failed (
	const ase_char_t* expr, const ase_char_t* desc, 
	const ase_char_t* file, ase_size_t line)
{
	if (desc == ASE_NULL)
	{
		ase_fprintf (
			ASE_STDERR,
			ASE_T("ASSERTION FAILURE AT FILE %s LINE %lu\n%s\n"),
			file, (unsigned long)line, expr);
	}
	else
	{
		ase_fprintf (
			ASE_STDERR,
			ASE_T("ASSERTION FAILURE AT FILE %s LINE %lu\n%s\n\nDESCRIPTION:\n%s\n"),
			file, (unsigned long)line, expr, desc);

	}

	abort ();
	return 0;
}

#endif

