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

#include "mod-str.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem-prv.h"
#include "fnc.h"
#include "val.h"

static int fnc_normspace (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	/* normalize spaces 
	 * - trim leading and trailing spaces
	 * - replace a series of spaces to a single space
	 */
	qse_awk_val_t* retv;
	qse_awk_val_t* a0;

	a0 = qse_awk_rtx_getarg(rtx, 0);
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mchar_t* str0;
		qse_size_t len0;

		str0 = qse_awk_rtx_valtombsdup(rtx, a0, &len0);
		if (!str0) return -1;
		len0 = qse_mbsxpac(str0, len0);
		retv = qse_awk_rtx_makembsval(rtx, str0, len0);
		qse_awk_rtx_freemem (rtx, str0);
	}
	else
	{
		qse_char_t* str0;
		qse_size_t len0;

		str0 = qse_awk_rtx_valtostrdup(rtx, a0, &len0);
		if (!str0) return -1;
		len0 = qse_strxpac(str0, len0);
		retv = qse_awk_rtx_makestrval(rtx, str0, len0);
		qse_awk_rtx_freemem (rtx, str0);
	}

	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);

	return 0;
}

static int trim (qse_awk_rtx_t* rtx, int flags)
{
	qse_awk_val_t* retv;
	qse_awk_val_t* a0;

	a0 = qse_awk_rtx_getarg(rtx, 0);

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mcstr_t path;
		qse_mchar_t* npath;
		path.ptr = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		path.len = ((qse_awk_val_mbs_t*)a0)->val.len;
		npath = qse_mbsxtrmx(path.ptr, &path.len, flags);
		retv = qse_awk_rtx_makembsval(rtx, npath, path.len);
	}
	else
	{
		qse_cstr_t path;
		qse_char_t* npath;
		path.ptr = qse_awk_rtx_getvalstr(rtx, a0, &path.len);
		if (!path.ptr) return -1;
		/* because qse_strxtrmx() returns the pointer and the legnth without 
		 * affecting the string given, it's safe to pass the original value.
		 * qse_awk_rtx_getvalstr() doesn't duplicate the value if it's of 
		 * the string type. */
		npath = qse_strxtrmx(path.ptr, &path.len, flags); 
		retv = qse_awk_rtx_makestrval(rtx, npath, path.len);
		qse_awk_rtx_freevalstr (rtx, a0, path.ptr);
	}

	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_trim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim(rtx, QSE_STRTRMX_LEFT | QSE_STRTRMX_RIGHT);
}
static int fnc_ltrim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim(rtx, QSE_STRTRMX_LEFT);
}
static int fnc_rtrim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim(rtx, QSE_STRTRMX_RIGHT);
}  

static int is_class (qse_awk_rtx_t* rtx, qse_ctype_t ctype)
{
	qse_awk_val_t* a0;
	int tmp;

	a0 = qse_awk_rtx_getarg (rtx, 0);

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mchar_t* str0;
		qse_size_t len0;

		str0 = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		len0 = ((qse_awk_val_mbs_t*)a0)->val.len;

		if (len0 <= 0) tmp = 0;
		else
		{
			tmp = 1;
			do
			{
				len0--;
				if (!qse_ismctype(str0[len0], ctype)) 
				{
					tmp = 0;
					break;
				}
			}
			while (len0 > 0);
		}
	}
	else
	{
		qse_char_t* str0;
		qse_size_t len0;

		str0 = qse_awk_rtx_getvalstr(rtx, a0, &len0);
		if (!str0) return -1;

		if (len0 <= 0) tmp = 0;
		else
		{
			tmp = 1;
			do
			{
				len0--;
				if (!qse_isctype(str0[len0], ctype)) 
				{
					tmp = 0;
					break;
				}
			}
			while (len0 > 0);
		}
		qse_awk_rtx_freevalstr (rtx, a0, str0);
	}

	a0 = qse_awk_rtx_makeintval (rtx, tmp);
	if (!a0) return -1;

	qse_awk_rtx_setretval (rtx, a0);
	return 0;
}

static int fnc_isalnum (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_ALNUM);
}

static int fnc_isalpha (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_ALPHA);
}

static int fnc_isblank (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_BLANK);
}

static int fnc_iscntrl (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_CNTRL);
}

static int fnc_isdigit (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_DIGIT);
}

static int fnc_isgraph (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_GRAPH);
}

static int fnc_islower (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_LOWER);
}

static int fnc_isprint (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_PRINT);
}

static int fnc_ispunct (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_PUNCT);
}

static int fnc_isspace (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_SPACE);
}

static int fnc_isupper (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_UPPER);
}

static int fnc_isxdigit (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return is_class(rtx, QSE_CTYPE_XDIGIT);
}

static int fnc_value (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	/* return the numeric value for the first character.
	 * you can use sprintf("%c", num_val) for reverse conversion. */
	qse_awk_val_t* retv;
	qse_awk_val_t* a0;
	qse_awk_int_t iv = -1;

	a0 = qse_awk_rtx_getarg(rtx, 0);

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mchar_t* str0;
		qse_size_t len0;

		str0 = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		len0 = ((qse_awk_val_mbs_t*)a0)->val.len;

		if (len0 >= 1)
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			/* typecasting in case qse_mchar_t is signed */
			iv = (unsigned char)str0[0];
		#else
			iv = str0[0];
		#endif
		}
	}
	else
	{
		qse_char_t* str0;
		qse_size_t len0;

		str0 = qse_awk_rtx_getvalstr(rtx, a0, &len0);
		if (!str0) return -1;

		if (len0 >= 1)
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			/* typecasting in case qse_mchar_t is signed */
			iv = (unsigned char)str0[0];
		#else
			iv = str0[0];
		#endif
		}

		qse_awk_rtx_freevalstr(rtx, a0, str0);
	}

	if (iv >= 0)
	{
		retv = qse_awk_rtx_makeintval(rtx, iv);
		if (!retv) return -1;
		qse_awk_rtx_setretval (rtx, retv);
	}

	return 0;
}

static int fnc_tonum (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	/* str::tonum(value) */
	/* str::tonum(string, base) */

	qse_awk_val_t* retv;
	qse_awk_val_t* a0;
	qse_awk_int_t lv;
	qse_awk_flt_t rv;
	int rx;

	a0 = qse_awk_rtx_getarg(rtx, 0);

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS && qse_awk_rtx_getnargs(rtx) >= 2)
	{
		/* if the value is known to be a byte string, it supports the optional
		 * base parameter */
		qse_awk_val_t* a1 = qse_awk_rtx_getarg(rtx, 1);
		qse_awk_int_t base;

		if (qse_awk_rtx_valtoint(rtx, a1, &base) <= -1) return -1;
		rx = qse_awk_rtx_mbstonum (
			rtx,
			QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, base),
			((qse_awk_val_mbs_t*)a0)->val.ptr,
			((qse_awk_val_mbs_t*)a0)->val.len,
			&lv, &rv
		);
	}
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_STR && qse_awk_rtx_getnargs(rtx) >= 2)
	{
		/* if the value is known to be a string, it supports the optional
		 * base parameter */
		qse_awk_val_t* a1 = qse_awk_rtx_getarg(rtx, 1);
		qse_awk_int_t base;

		if (qse_awk_rtx_valtoint(rtx, a1, &base) <= -1) return -1;
		rx = qse_awk_rtx_strtonum (
			rtx,
			QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, base),
			((qse_awk_val_str_t*)a0)->val.ptr,
			((qse_awk_val_str_t*)a0)->val.len,
			&lv, &rv
		);
	}
	else
	{
		rx = qse_awk_rtx_valtonum(rtx, a0, &lv, &rv);
	}

	if (rx == 0)
	{
		retv = qse_awk_rtx_makeintval(rtx, lv);
	}
	else if (rx >= 1)
	{
		retv = qse_awk_rtx_makefltval(rtx, rv);
	}
	else
	{
		retv = qse_awk_rtx_makeintval(rtx, 0);
	}

	if (!retv) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

#define A_MAX QSE_TYPE_MAX(int)

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("gsub"),      { { 2, 3, QSE_T("xvr")},   qse_awk_fnc_gsub,      0 } },
	{ QSE_T("index"),     { { 2, 3, QSE_NULL },      qse_awk_fnc_index,     0 } },
	{ QSE_T("isalnum"),   { { 1, 1, QSE_NULL },      fnc_isalnum,           0 } },
	{ QSE_T("isalpha"),   { { 1, 1, QSE_NULL },      fnc_isalpha,           0 } },
	{ QSE_T("isblank"),   { { 1, 1, QSE_NULL },      fnc_isblank,           0 } },
	{ QSE_T("iscntrl"),   { { 1, 1, QSE_NULL },      fnc_iscntrl,           0 } },
	{ QSE_T("isdigit"),   { { 1, 1, QSE_NULL },      fnc_isdigit,           0 } },
	{ QSE_T("isgraph"),   { { 1, 1, QSE_NULL },      fnc_isgraph,           0 } },
	{ QSE_T("islower"),   { { 1, 1, QSE_NULL },      fnc_islower,           0 } },
	{ QSE_T("isprint"),   { { 1, 1, QSE_NULL },      fnc_isprint,           0 } },
	{ QSE_T("ispunct"),   { { 1, 1, QSE_NULL },      fnc_ispunct,           0 } },
	{ QSE_T("isspace"),   { { 1, 1, QSE_NULL },      fnc_isspace,           0 } },
	{ QSE_T("isupper"),   { { 1, 1, QSE_NULL },      fnc_isupper,           0 } },
	{ QSE_T("isxdigit"),  { { 1, 1, QSE_NULL },      fnc_isxdigit,          0 } },
	{ QSE_T("length"),    { { 1, 1, QSE_NULL },      qse_awk_fnc_length,    0 } },
	{ QSE_T("ltrim"),     { { 1, 1, QSE_NULL },      fnc_ltrim,             0 } },
	{ QSE_T("match"),     { { 2, 4, QSE_T("vxvr") }, qse_awk_fnc_match,     0 } },
	{ QSE_T("normspace"), { { 1, 1, QSE_NULL },      fnc_normspace,         0 } },
	{ QSE_T("printf"),    { { 1, A_MAX, QSE_NULL },  qse_awk_fnc_sprintf,   0 } },
	{ QSE_T("rindex"),    { { 2, 3, QSE_NULL },      qse_awk_fnc_rindex,    0 } },
	{ QSE_T("rtrim"),     { { 1, 1, QSE_NULL },      fnc_rtrim,             0 } },
	{ QSE_T("split"),     { { 2, 3, QSE_T("vrx") },  qse_awk_fnc_split,     0 } },
	{ QSE_T("sub"),       { { 2, 3, QSE_T("xvr") },  qse_awk_fnc_sub,       0 } },
	{ QSE_T("substr"),    { { 2, 3, QSE_NULL },      qse_awk_fnc_substr,    0 } },
	{ QSE_T("tolower"),   { { 1, 1, QSE_NULL },      qse_awk_fnc_tolower,   0 } },
	{ QSE_T("tonum"),     { { 1, 2, QSE_NULL },      fnc_tonum,             0 } },
	{ QSE_T("toupper"),   { { 1, 1, QSE_NULL },      qse_awk_fnc_toupper,   0 } },
	{ QSE_T("trim"),      { { 1, 1, QSE_NULL },      fnc_trim,              0 } },
	{ QSE_T("value"),     { { 1, 1, QSE_NULL },      fnc_value,             0 } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (fnctab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

#if 0
	left = 0; right = QSE_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (inttab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
     }
#endif

	ea.ptr = (qse_char_t*)name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int qse_awk_mod_str (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	/*
	mod->ctx...
	 */

	return 0;
}

