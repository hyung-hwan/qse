/*
 * $Id: ctype.c 132 2008-03-17 10:27:02Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

#include <qse/cmn/chr.h>

#if defined(QSE_CHAR_IS_MCHAR)

#include <ctype.h>

static qse_bool_t is_upper (qse_cint_t c) { return isupper(c); }
static qse_bool_t is_lower (qse_cint_t c) { return islower(c); }
static qse_bool_t is_alpha (qse_cint_t c) { return isalpha(c); }
static qse_bool_t is_digit (qse_cint_t c) { return isdigit(c); }
static qse_bool_t is_xdigit (qse_cint_t c) { return isxdigit(c); }
static qse_bool_t is_alnum (qse_cint_t c) { return isalnum(c); }
static qse_bool_t is_space (qse_cint_t c) { return isspace(c); }
static qse_bool_t is_print (qse_cint_t c) { return isprint(c); }
static qse_bool_t is_graph (qse_cint_t c) { return isgraph(c); }
static qse_bool_t is_cntrl (qse_cint_t c) { return iscntrl(c); }
static qse_bool_t is_punct (qse_cint_t c) { return ispunct(c); }

qse_bool_t qse_ccls_is (qse_cint_t c, qse_ccls_type_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static qse_bool_t (*f[]) (qse_cint_t) = 
	{
		is_upper,
		is_lower,
		is_alpha,
		is_digit,
		is_xdigit,
		is_alnum,
		is_space,
		is_print,
		is_graph,
		is_cntrl,
		is_punct
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_PUNCT,
		"The character type should be one of qse_ccls_type_t values");
	return f[type] (c);
}

qse_cint_t qse_ccls_to (qse_cint_t c, qse_ccls_type_t type)  
{ 
	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_LOWER,
		"The character type should be one of QSE_CCLS_UPPER and QSE_CCLS_LOWER");

	if (type == QSE_CCLS_UPPER) return toupper(c);
	if (type == QSE_CCLS_LOWER) return tolower(c);
	return c;
}

#elif defined(QSE_CHAR_IS_WCHAR)

#include <wctype.h>

qse_bool_t qse_ccls_is (qse_cint_t c, qse_ccls_type_t type)
{ 
	static const char* name[] = 
	{
		"upper",
		"lower",
		"alpha",
		"digit",
		"xdigit",
		"alnum",
		"space",
		"print",
		"graph",
		"cntrl",
		"punct"
	};

	static wctype_t desc[] =
	{
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_PUNCT,
		"The character type should be one of qse_ccls_type_t values");
		
	if (desc[type] == (wctype_t)0) desc[type] = wctype(name[type]);
	return iswctype (c, desc[type]);
}

qse_cint_t qse_ccls_to (qse_cint_t c, qse_ccls_type_t type)  
{ 
	static const char* name[] = 
	{
		"toupper",
		"tolower"
	};

	static wctrans_t desc[] =
	{
		(wctrans_t)0,
		(wctrans_t)0
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_LOWER,
		"The character type should be one of QSE_CCLS_UPPER and QSE_CCLS_LOWER");

	if (desc[type] == (wctrans_t)0) desc[type] = wctrans(name[type]);
	return towctrans (c, desc[type]);
}

#else
	#error unsupported character type
#endif

static qse_bool_t ccls_is (void* data, qse_cint_t c, qse_ccls_type_t type)
{
	return qse_ccls_is (c, type);
}

static qse_cint_t ccls_to (void* data, qse_cint_t c, qse_ccls_type_t type)  
{
	return qse_ccls_to (c, type);
}

static qse_ccls_t ccls =
{
	ccls_is,
	ccls_to,
	QSE_NULL
};

qse_ccls_t* qse_ccls = &ccls;

