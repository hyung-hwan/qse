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

#include <qse/cmn/String.hpp>
#include "mem.h"

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		if (defined(__WATCOMC__) && (__WATCOMC__ < 1200)) || defined(__BORLANDC__)
#			undef HAVE_VA_COPY
#			undef HAVE___VA_COPY
#		else
#			define HAVE_VA_COPY
#			define HAVE___VA_COPY
#		endif
#	endif
#endif

#if !defined(HAVE_VA_COPY)
#	if defined(HAVE___VA_COPY)
#		define va_copy(dst,src) __va_copy((dst),(src))
#	else
#		define va_copy(dst,src) QSE_MEMCPY(&(dst),&(src),QSE_SIZEOF(va_list))
#	endif
#endif

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////


int WcString::format (const qse_wchar_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	int x = this->formatv (fmt, ap);
	va_end (ap);
	return x;

}
int WcString::formatv (const qse_wchar_t* fmt, va_list ap)
{
	va_list saved_ap;

	va_copy (saved_ap, ap);
	qse_size_t n = qse_wcsxvfmt(QSE_NULL, 0, fmt, ap);
	if (n == (qse_size_t)-1)
	{
		// there's mb/wc conversion error.
		return -1;
	}

	if (n > this->getCapacity()) this->possess_data (this->round_capacity(n));
	else if (this->_item->isShared()) this->possess_data ();

	qse_wcsxvfmt ((qse_wchar_t*)this->getBuffer(), this->getCapacity() + 1, fmt, saved_ap);

	this->force_size (n);
	return 0;
}

//////////////////////////////////////////////////////////////////////////

int MbString::format (const qse_mchar_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	int x = this->formatv (fmt, ap);
	va_end (ap);
	return x;
}

int MbString::formatv (const qse_mchar_t* fmt, va_list ap)
{
	va_list saved_ap;

	va_copy (saved_ap, ap);
	qse_size_t n = qse_mbsxvfmt(QSE_NULL, 0, fmt, ap);
	if (n == (qse_size_t)-1)
	{
		// there's mb/wc conversion error.
		return -1;
	}

	if (n > this->getCapacity()) this->possess_data (this->round_capacity(n));
	else if (this->_item->isShared()) this->possess_data ();

	qse_mbsxvfmt ((qse_mchar_t*)this->getBuffer(), this->getCapacity() + 1, fmt, saved_ap);

	this->force_size (n);
	return 0;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
