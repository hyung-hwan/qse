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

#include <qse/cmn/str.h>
#include "mem.h"

qse_mchar_t* qse_mbsdup (const qse_mchar_t* str, qse_mmgr_t* mmgr)
{
	qse_mchar_t* tmp;

	QSE_ASSERT (mmgr != QSE_NULL);

	tmp = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (qse_mbslen(str)+1)*QSE_SIZEOF(qse_mchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_mbscpy (tmp, str);
	return tmp;
}

qse_mchar_t* qse_mbsdup2 (
	const qse_mchar_t* str1, const qse_mchar_t* str2, qse_mmgr_t* mmgr)
{
	return qse_mbsxdup2 (
		str1, qse_mbslen(str1), str2, qse_mbslen(str2), mmgr);
}

qse_mchar_t* qse_mbsxdup (
	const qse_mchar_t* str, qse_size_t len, qse_mmgr_t* mmgr)
{
	qse_mchar_t* tmp;

	tmp = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (len+1)*QSE_SIZEOF(qse_mchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_mbsncpy (tmp, str, len);
	return tmp;
}

qse_mchar_t* qse_mbsxdup2 (
	const qse_mchar_t* str1, qse_size_t len1,
	const qse_mchar_t* str2, qse_size_t len2, qse_mmgr_t* mmgr)
{
	qse_mchar_t* tmp;

	QSE_ASSERT (mmgr != QSE_NULL);

	tmp = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (len1+len2+1) * QSE_SIZEOF(qse_mchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_mbsncpy (tmp, str1, len1);
	qse_mbsncpy (tmp + len1, str2, len2);
	return tmp;
}

qse_mchar_t* qse_mbsadup (const qse_mchar_t* str[], qse_size_t* len, qse_mmgr_t* mmgr)
{
	qse_mchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; str[i]; i++) capa += qse_mbslen(str[i]);

	buf = (qse_mchar_t*) QSE_MMGR_ALLOC (mmgr, (capa+1)*QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; str[i]; i++) ptr += qse_mbscpy (ptr, str[i]);

	if (len) *len = capa;
	return buf;
}

qse_mchar_t* qse_mcstrdup (const qse_mcstr_t* str, qse_mmgr_t* mmgr)
{
	qse_mchar_t* buf;

	QSE_ASSERT (mmgr != QSE_NULL);

	buf = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (str->len + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	qse_mbsncpy (buf, str->ptr, str->len);
	return buf;
}

qse_mchar_t* qse_mcstradup (const qse_mcstr_t str[], qse_size_t* len, qse_mmgr_t* mmgr)
{
	qse_mchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; str[i].ptr; i++) capa += str[i].len;

	buf = (qse_mchar_t*) QSE_MMGR_ALLOC (mmgr, (capa+1)*QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; str[i].ptr; i++) ptr += qse_mbsncpy (ptr, str[i].ptr, str[i].len);

	if (len) *len = capa;
	return buf;
}

/* --------------------------------------------------------------- */

qse_wchar_t* qse_wcsdup (const qse_wchar_t* str, qse_mmgr_t* mmgr)
{
	qse_wchar_t* tmp;

	QSE_ASSERT (mmgr != QSE_NULL);

	tmp = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (qse_wcslen(str)+1)*QSE_SIZEOF(qse_wchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_wcscpy (tmp, str);
	return tmp;
}

qse_wchar_t* qse_wcsdup2 (
	const qse_wchar_t* str1, const qse_wchar_t* str2, qse_mmgr_t* mmgr)
{
	return qse_wcsxdup2 (
		str1, qse_wcslen(str1), str2, qse_wcslen(str2), mmgr);
}

qse_wchar_t* qse_wcsxdup (
	const qse_wchar_t* str, qse_size_t len, qse_mmgr_t* mmgr)
{
	qse_wchar_t* tmp;

	QSE_ASSERT (mmgr != QSE_NULL);

	tmp = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (len+1)*QSE_SIZEOF(qse_wchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_wcsncpy (tmp, str, len);
	return tmp;
}

qse_wchar_t* qse_wcsxdup2 (
	const qse_wchar_t* str1, qse_size_t len1,
	const qse_wchar_t* str2, qse_size_t len2, qse_mmgr_t* mmgr)
{
	qse_wchar_t* tmp;

	QSE_ASSERT (mmgr != QSE_NULL);

	tmp = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (len1+len2+1) * QSE_SIZEOF(qse_wchar_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_wcsncpy (tmp, str1, len1);
	qse_wcsncpy (tmp + len1, str2, len2);
	return tmp;
}

qse_wchar_t* qse_wcsadup (const qse_wchar_t* str[], qse_size_t* len, qse_mmgr_t* mmgr)
{
	qse_wchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; str[i]; i++) capa += qse_wcslen(str[i]);

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (mmgr, (capa+1)*QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; str[i]; i++) ptr += qse_wcscpy (ptr, str[i]);

	if (len) *len = capa;
	return buf;
}

qse_wchar_t* qse_wcstrdup (const qse_wcstr_t* str, qse_mmgr_t* mmgr)
{
	qse_wchar_t* buf;

	QSE_ASSERT (mmgr != QSE_NULL);

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (str->len + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	qse_wcsncpy (buf, str->ptr, str->len);
	return buf;
}

qse_wchar_t* qse_wcstradup (const qse_wcstr_t str[], qse_size_t* len, qse_mmgr_t* mmgr)
{
	qse_wchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; str[i].ptr; i++) capa += str[i].len;

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (mmgr, (capa+1)*QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; str[i].ptr; i++) ptr += qse_wcsncpy (ptr, str[i].ptr, str[i].len);

	if (len) *len = capa;
	return buf;
}
