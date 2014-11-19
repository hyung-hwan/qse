/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/cp950.h>
#include "cp950.h"

qse_size_t qse_uctocp950 (qse_wchar_t uc, qse_mchar_t* cp950, qse_size_t size)
{
	if (uc & ~(qse_wchar_t)0x7F)
	{

		if (uc >= 0xffffu) return 0; /* illegal character */
		if (size >= 2)
		{
			qse_uint16_t mb;

			mb = wctomb (uc);
			if (mb == 0xffffu) return 0; /* illegal character */

			cp950[0] = (mb >> 8);
			cp950[1] = (mb & 0xFF);
		}

		return 2;
	}
	else
	{	
		/* uc >= 0 && uc <= 127 */
		if (size >= 1) *cp950 = uc;
		return 1; /* ok or buffer to small */
	}
}

qse_size_t qse_cp950touc (
	const qse_mchar_t* cp950, qse_size_t size, qse_wchar_t* uc)
{
	QSE_ASSERT (cp950 != QSE_NULL);
	QSE_ASSERT (size > 0);
	QSE_ASSERT (QSE_SIZEOF(qse_mchar_t) == 1);
	QSE_ASSERT (QSE_SIZEOF(qse_wchar_t) >= 2);

	if (cp950[0] & 0x80)
	{
		if (size >= 2)
		{
			qse_uint16_t wc;	
			wc = mbtowc ((((qse_uint16_t)(qse_uint8_t)cp950[0]) << 8) | (qse_uint8_t)cp950[1]);
			if (wc == 0xffffu) return 0; /* illegal sequence */
			if (uc) *uc = wc;
		}
		return 2; /* ok or incomplete sequence */
	}
	else
	{
		if (uc) *uc = cp950[0];
		return 1;
	}
}

qse_size_t qse_cp950len (const qse_mchar_t* cp950, qse_size_t size)
{
	return qse_cp950touc (cp950, size, QSE_NULL);
}

qse_size_t qse_cp950lenmax (void)
{
	return QSE_CP950LEN_MAX;
}

