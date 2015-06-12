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

#include <qse/cmn/uni.h>

enum
{
	TRAIT_UPPER  = (1 << 0),
	TRAIT_LOWER  = (1 << 1),
	TRAIT_ALPHA  = (1 << 2),
	TRAIT_DIGIT  = (1 << 3),
	TRAIT_XDIGIT = (1 << 4),
	TRAIT_ALNUM  = (1 << 5),
	TRAIT_SPACE  = (1 << 6),
	TRAIT_PRINT  = (1 << 8),
	TRAIT_GRAPH  = (1 << 9),
	TRAIT_CNTRL  = (1 << 10),
	TRAIT_PUNCT  = (1 << 11),
	TRAIT_BLANK  = (1 << 12)
};

/* ---------------------------------------------------------- */
#include "uni-trait.h"

/* ---------------------------------------------------------- */

#include "uni-case.h"

/* ---------------------------------------------------------- */

#define TRAIT_MAP_INDEX(c) ((c) / QSE_COUNTOF(trait_page_0000))
#define TRAIT_PAGE_INDEX(c) ((c) % QSE_COUNTOF(trait_page_0000))

#define CASE_MAP_INDEX(c) ((c) / QSE_COUNTOF(case_page_0000))
#define CASE_PAGE_INDEX(c) ((c) % QSE_COUNTOF(case_page_0000))

#define UNICODE_ISTYPE(c,type) \
	((c) >= 0 && (c) <= TRAIT_MAX && \
	 (trait_map[TRAIT_MAP_INDEX(c)][TRAIT_PAGE_INDEX(c)] & (type)) != 0)

int qse_isunitype (qse_wcint_t c, int type)
{
	return UNICODE_ISTYPE (c, type);
}

int qse_isuniupper (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_UPPER);
}

int qse_isunilower (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_LOWER);
}

int qse_isunialpha (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_ALPHA);
}

int qse_isunidigit (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_DIGIT);
}

int qse_isunixdigit (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_XDIGIT);
}

int qse_isunialnum (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_ALNUM);
}

int qse_isunispace (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_SPACE);
}

int qse_isuniprint (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_PRINT);
}

int qse_isunigraph (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_GRAPH);
}

int qse_isunicntrl (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_CNTRL);
}

int qse_isunipunct (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_PUNCT);
}

int qse_isuniblank (qse_wcint_t c)
{
	return UNICODE_ISTYPE (c, TRAIT_BLANK);
}

qse_wcint_t qse_touniupper (qse_wcint_t c)
{
	if (c >= 0 && c <= CASE_MAX) 
	{
	 	case_page_t* page;
		page = case_map[CASE_MAP_INDEX(c)];
		return c - page[CASE_PAGE_INDEX(c)].upper;
	}
	return c;
}

qse_wcint_t qse_tounilower (qse_wcint_t c)
{
	if (c >= 0 && c <= CASE_MAX) 
	{
	 	case_page_t* page;
		page = case_map[CASE_MAP_INDEX(c)];
		return c + page[CASE_PAGE_INDEX(c)].lower;
	}
	return c;
}

