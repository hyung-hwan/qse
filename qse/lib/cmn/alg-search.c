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

#if defined(macintosh)
#	include <:qse:cmn:alg.h>
#else
#	include <qse/cmn/alg.h>
#endif

void* qse_bsearch (
	const void *key, const void *base, qse_size_t nmemb,
	qse_size_t size, qse_search_comper_t comper, void* ctx)
{
	int n;
	const void* mid;
	qse_size_t lim;

	for (lim = nmemb; lim > 0; lim >>= 1)
	{
		mid = ((qse_byte_t*)base) + (lim >> 1) * size;

		n = (comper) (key, mid, ctx);
		if (n == 0) return (void*)mid;
		if (n > 0) { base = ((const qse_byte_t*)mid) + size; lim--; }
	}

	return QSE_NULL;
}

void* qse_lsearch (
	const void* key, const void* base, qse_size_t nmemb,
	qse_size_t size, qse_search_comper_t comper, void* ctx)
{
	while (nmemb > 0) 
	{
		if (comper(key, base, ctx) == 0) return (void*)base;
		base = ((const qse_byte_t*)base) + size;
		nmemb--;
	}

	return QSE_NULL;
}
