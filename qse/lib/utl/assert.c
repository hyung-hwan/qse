/*
 * $Id: assert.c 223 2008-06-26 06:44:41Z baconevi $
 */

#include <qse/types.h>
#include <qse/macros.h>

#ifndef NDEBUG

#include <qse/utl/stdio.h>
#include <stdlib.h>

void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc, 
	const qse_char_t* file, qse_size_t line)
{
	qse_fprintf (QSE_STDERR, QSE_T("=[ASSERTION FAILURE]============================================================"));
	qse_fprintf (QSE_STDERR, QSE_T("FILE %s LINE %lu: %s\n"),
		file, (unsigned long)line, expr);

	if (desc != QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("DESCRIPTION: %s\n"),
			file, (unsigned long)line, expr, desc);
	}
	qse_fprintf (QSE_STDERR, QSE_T("================================================================================"));

	abort ();
}

#endif

