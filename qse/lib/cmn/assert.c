/*
 * $Id: assert.c 223 2008-06-26 06:44:41Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/types.h>
#include <qse/macros.h>

#ifndef NDEBUG

#include <qse/cmn/sio.h>
#include <stdlib.h>

void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc, 
	const qse_char_t* file, qse_size_t line)
{
	qse_sio_puts (QSE_SIO_ERR, QSE_T("=[ASSERTION FAILURE]============================================================"));

	qse_sio_puts (QSE_SIO_ERR, QSE_T("FILE "));
	qse_sio_puts (QSE_SIO_ERR, file);
	qse_sio_puts (QSE_SIO_ERR, QSE_T("LINE "));

/*qse_sio_puts the number */

	qse_sio_puts (QSE_SIO_ERR, QSE_T(": "));
	qse_sio_puts (QSE_SIO_ERR, expr);
	qse_sio_puts (QSE_SIO_ERR, QSE_T("\n"));

	if (desc != QSE_NULL)
	{
		qse_sio_puts (QSE_SIO_ERR, QSE_T("DESCRIPTION: "));
		qse_sio_puts (QSE_SIO_ERR, desc);
		qse_sio_puts (QSE_SIO_ERR, QSE_T("\n"));
	}
	qse_sio_puts (QSE_SIO_ERR, QSE_T("================================================================================"));

	abort ();
}

#endif

