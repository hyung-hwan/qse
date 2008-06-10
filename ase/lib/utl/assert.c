/*
 * $Id: assert.c 201 2008-06-09 06:57:20Z baconevi $
 */

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#ifndef NDEBUG

#include <ase/utl/stdio.h>
#include <stdlib.h>

void ase_assert_failed (
	const ase_char_t* expr, const ase_char_t* desc, 
	const ase_char_t* file, ase_size_t line)
{
	ase_fprintf (ASE_STDERR, ASE_T("=[ASSERTION FAILURE]============================================================"));
	ase_fprintf (ASE_STDERR, ASE_T("FILE %s LINE %lu: %s\n"),
		file, (unsigned long)line, expr);

	if (desc != ASE_NULL)
	{
		ase_fprintf (ASE_STDERR, ASE_T("DESCRIPTION: %s\n"),
			file, (unsigned long)line, expr, desc);
	}
	ase_fprintf (ASE_STDERR, ASE_T("================================================================================"));

	abort ();
}

#endif

