/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mod-math.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/alg.h>
#include <qse/cmn/time.h>
#include "../cmn/mem.h"
#include "fnc.h"


#include <math.h>
#if defined(HAVE_QUADMATH_H)
#	include <quadmath.h>
#endif

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_CEIL
#		define HAVE_FLOOR
#		if !defined(__WATCOMC__)
#		define HAVE_ROUND
#		endif
#		define HAVE_SINH
#		define HAVE_COSH
#		define HAVE_TANH
#		define HAVE_ASIN
#		define HAVE_ACOS

#		define HAVE_SIN
#		define HAVE_COS
#		define HAVE_TAN
#		define HAVE_ATAN
#		define HAVE_ATAN2
#		define HAVE_LOG
#		define HAVE_LOG10
#		define HAVE_EXP
#		define HAVE_SQRT
#	endif
#endif

typedef struct modctx_t
{
	qse_awk_int_t seed;	
	qse_awk_uint_t prand; /* last random value returned */
} modctx_t;

static int fnc_math_1 (
	qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi, qse_awk_math1_t f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_awk_flt_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	n = qse_awk_rtx_valtoflt (rtx, a0, &rv);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makefltval (rtx, f (qse_awk_rtx_getawk(rtx), rv));
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_math_2 (
	qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi, qse_awk_math2_t f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_awk_flt_t rv0, rv1;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	n = qse_awk_rtx_valtoflt (rtx, a0, &rv0);
	if (n <= -1) return -1;

	n = qse_awk_rtx_valtoflt (rtx, a1, &rv1);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makefltval (rtx, f (qse_awk_rtx_getawk(rtx), rv0, rv1));
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}


static qse_awk_flt_t math_ceil (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_CEILQ)
	return ceilq (x);
#elif defined(HAVE_CEILL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ceill (x);
#elif defined(HAVE_CEIL)
	return ceil (x);
#elif defined(HAVE_CEILF)
	return ceilf (x);
#else
	#error ### no ceil function available ###
#endif
}

static qse_awk_flt_t math_floor (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_FLOORQ)
	return floorq (x);
#elif defined(HAVE_FLOORL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return floorl (x);
#elif defined(HAVE_FLOOR)
	return floor (x);
#elif defined(HAVE_FLOORF)
	return floorf (x);
#else
	#error ### no floor function available ###
#endif
}

static qse_awk_flt_t math_round (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_ROUNDQ)
	return roundq (x);
#elif defined(HAVE_ROUNDL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return roundl (x);
#elif defined(HAVE_ROUND)
	return round (x);
#elif defined(HAVE_ROUNDF)
	return roundf (x);
#else

	qse_flt_t f, d;

	f = math_floor (awk, x);
	d = x - f; /* get fraction */

	if (d > (qse_awk_flt_t)0.5)
	{
		/* round up to the nearest */
		f =  f + (qse_awk_flt_t)1.0;
	}
	else if (d == (qse_awk_flt_t)0.5)
	{
	#if 1
		/* round half away from zero */
		if (x >= 0)
		{
			f = x + (qse_awk_flt_t)0.5;
		}
		else
		{
			f = x - (qse_awk_flt_t)0.5;
		}
	#else
		/* round half to even - C99's rint() does this, i guess. */
		d = f - (qse_awk_flt_t)2.0 * math_floor(awk, f * (qse_awk_flt_t)0.5);
		if (d == (qse_awk_flt_t)1.0) f =  f + (qse_awk_flt_t)1.0;
	#endif
	}

	/* this implementation doesn't keep the signbit for -0.0.
	 * The signbit() function defined in C99 may get used to
	 * preserve the sign bit. but this is a fall-back rountine
	 * for a system without round also defined in C99. 
	 * don't get annoyed by the lost sign bit for the value of 0.0.
	 */

	return f;
	
#endif
}

static qse_awk_flt_t math_sinh (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_SINHQ)
	return sinhq (x);
#elif defined(HAVE_SINHL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return sinhl (x);
#elif defined(HAVE_SINH)
	return sinh (x);
#elif defined(HAVE_SINHF)
	return sinhf (x);
#else
	#error ### no sinh function available ###
#endif
}

static qse_awk_flt_t math_cosh (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_COSHQ)
	return coshq (x);
#elif defined(HAVE_COSHL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return coshl (x);
#elif defined(HAVE_COSH)
	return cosh (x);
#elif defined(HAVE_COSHF)
	return coshf (x);
#else
	#error ### no cosh function available ###
#endif
}

static qse_awk_flt_t math_tanh (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_TANHQ)
	return tanhq (x);
#elif defined(HAVE_TANHL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return tanhl (x);
#elif defined(HAVE_TANH)
	return tanh (x);
#elif defined(HAVE_TANHF)
	return tanhf (x);
#else
	#error ### no tanh function available ###
#endif
}

static qse_awk_flt_t math_asin (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_ASINQ)
	return asinq (x);
#elif defined(HAVE_ASINL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return asinl (x);
#elif defined(HAVE_ASIN)
	return asin (x);
#elif defined(HAVE_ASINF)
	return asinf (x);
#else
	#error ### no asin function available ###
#endif
}

static qse_awk_flt_t math_acos (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_ACOSQ)
	return acosq (x);
#elif defined(HAVE_ACOSL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return acosl (x);
#elif defined(HAVE_ACOS)
	return acos (x);
#elif defined(HAVE_ACOSF)
	return acosf (x);
#else
	#error ### no acos function available ###
#endif
}

/* ----------------------------------------------------------------------- */


static qse_awk_flt_t math_sin (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_SINQ)
	return sinq (x);
#elif defined(HAVE_SINL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return sinl (x);
#elif defined(HAVE_SIN)
	return sin (x);
#elif defined(HAVE_SINF)
	return sinf (x);
#else
	#error ### no sin function available ###
#endif
}

static qse_awk_flt_t math_cos (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_COSQ)
	return cosq (x);
#elif defined(HAVE_COSL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return cosl (x);
#elif defined(HAVE_COS)
	return cos (x);
#elif defined(HAVE_COSF)
	return cosf (x);
#else
	#error ### no cos function available ###
#endif
}

static qse_awk_flt_t math_tan (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_TANQ)
	return tanq (x);
#elif defined(HAVE_TANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return tanl (x);
#elif defined(HAVE_TAN)
	return tan (x);
#elif defined(HAVE_TANF)
	return tanf (x);
#else
	#error ### no tan function available ###
#endif
}

static qse_awk_flt_t math_atan (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_ATANQ)
	return atanq (x);
#elif defined(HAVE_ATANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return atanl (x);
#elif defined(HAVE_ATAN)
	return atan (x);
#elif defined(HAVE_ATANF)
	return atanf (x);
#else
	#error ### no atan function available ###
#endif
}

static qse_awk_flt_t math_atan2 (qse_awk_t* awk, qse_awk_flt_t x, qse_awk_flt_t y)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_ATAN2Q)
	return atan2q (x, y);
#elif defined(HAVE_ATAN2L) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return atan2l (x, y);
#elif defined(HAVE_ATAN2)
	return atan2 (x, y);
#elif defined(HAVE_ATAN2F)
	return atan2f (x, y);
#else
	#error ### no atan2 function available ###
#endif
}

static qse_awk_flt_t math_log (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_LOGQ)
	return logq (x);
#elif defined(HAVE_LOGL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return logl (x);
#elif defined(HAVE_LOG)
	return log (x);
#elif defined(HAVE_LOGF)
	return logf (x);
#else
	#error ### no log function available ###
#endif
}

static qse_awk_flt_t math_log10 (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_LOG10Q)
	return log10q (x);
#elif defined(HAVE_LOG10L) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return log10l (x);
#elif defined(HAVE_LOG10)
	return log10 (x);
#elif defined(HAVE_LOG10F)
	return log10f (x);
#else
	#error ### no log10 function available ###
#endif
}

static qse_awk_flt_t math_exp (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_EXPQ)
	return expq (x);
#elif defined(HAVE_EXPL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return expl (x);
#elif defined(HAVE_EXP)
	return exp (x);
#elif defined(HAVE_EXPF)
	return expf (x);
#else
	#error ### no exp function available ###
#endif
}

static qse_awk_flt_t math_sqrt (qse_awk_t* awk, qse_awk_flt_t x)
{
#if defined(QSE_USE_AWK_FLTMAX) && defined(HAVE_SQRTQ)
	return sqrtq (x);
#elif defined(HAVE_SQRTL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return sqrtl (x);
#elif defined(HAVE_SQRT)
	return sqrt (x);
#elif defined(HAVE_SQRTF)
	return sqrtf (x);
#else
	#error ### no sqrt function available ###
#endif
}

/* ----------------------------------------------------------------------- */

static int fnc_ceil (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_ceil);
}

static int fnc_floor (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_floor);
}

static int fnc_round (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_round);
}

static int fnc_sinh (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_sinh);
}

static int fnc_cosh (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_cosh);
}

static int fnc_tanh (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_tanh);
}

static int fnc_asin (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_asin);
}

static int fnc_acos (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_acos);
}


/* ----------------------------------------------------------------------- */

static int fnc_sin (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_sin);
}

static int fnc_cos (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_cos);
}

static int fnc_tan (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_tan);
}

static int fnc_atan (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_atan);
}

static int fnc_atan2 (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_2 (rtx, fi, math_atan2);
}

static int fnc_log (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_log);
}

static int fnc_log10 (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_log10);
}

static int fnc_exp (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_exp);
}

static int fnc_sqrt (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, math_sqrt);
}

/* ----------------------------------------------------------------------- */

static int fnc_rand (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
#define RANDV_MAX QSE_TYPE_MAX(qse_awk_int_t)
	qse_awk_val_t* r;
	qse_awk_int_t randv;
	modctx_t* modctx;

	modctx = (modctx_t*)fi->mod->ctx;

#if defined(QSE_USE_AWK_INTMAX)
	modctx->prand = qse_randxsuintmax (modctx->prand);
#else
	modctx->prand = qse_randxsulong (modctx->prand);
#endif
	randv = modctx->prand % RANDV_MAX;

	r = qse_awk_rtx_makefltval (rtx, (qse_awk_flt_t)randv / RANDV_MAX);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
#undef RANDV_MAX
}

static int fnc_srand (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_awk_int_t lv;
	qse_awk_val_t* r;
	int n;
	qse_awk_int_t prev;
	qse_ntime_t now;
	modctx_t* modctx;

	modctx = (modctx_t*)fi->mod->ctx;
	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	prev = modctx->seed;

	if (nargs <= 0)
	{
		modctx->seed = (qse_gettime (&now) <= -1)? 
			(modctx->seed * modctx->seed): ((qse_awk_int_t)now.sec + (qse_awk_int_t)now.nsec);
	}
	else
	{
		a0 = qse_awk_rtx_getarg (rtx, 0);
		n = qse_awk_rtx_valtoint (rtx, a0, &lv);
		if (n <= -1) return -1;
		modctx->seed = lv;
	}
	/* i don't care if the seed becomes negative or overflows.
	 * i just convert the signed value to the unsigned one. */
	modctx->prand = (qse_awk_uint_t)(modctx->seed * modctx->seed * modctx->seed);
	/* make sure that the actual seeding is not 0 */
	if (modctx->prand == 0) modctx->prand++;

	r = qse_awk_rtx_makeintval (rtx, prev);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

/* ----------------------------------------------------------------------- */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("acos"),    { { 1, 1, QSE_NULL },     fnc_acos,       0 } },
	{ QSE_T("asin"),    { { 1, 1, QSE_NULL },     fnc_asin,       0 } },
	{ QSE_T("atan"),    { { 1, 1, QSE_NULL },     fnc_atan,       0 } },
	{ QSE_T("atan2"),   { { 2, 2, QSE_NULL },     fnc_atan2,      0 } },
	{ QSE_T("ceil"),    { { 1, 1, QSE_NULL },     fnc_ceil,       0 } },
	{ QSE_T("cos"),     { { 1, 1, QSE_NULL },     fnc_cos,        0 } },
	{ QSE_T("cosh"),    { { 1, 1, QSE_NULL },     fnc_cosh,       0 } },
	{ QSE_T("exp"),     { { 1, 1, QSE_NULL },     fnc_exp,        0 } },
	{ QSE_T("floor"),   { { 1, 1, QSE_NULL },     fnc_floor,      0 } },
	{ QSE_T("log"),     { { 1, 1, QSE_NULL },     fnc_log,        0 } },
	{ QSE_T("log10"),   { { 1, 1, QSE_NULL },     fnc_log10,      0 } },
	{ QSE_T("rand"),    { { 0, 0, QSE_NULL },     fnc_rand,       0 } },
	{ QSE_T("round"),   { { 1, 1, QSE_NULL },     fnc_round,      0 } },
	{ QSE_T("sin"),     { { 1, 1, QSE_NULL },     fnc_sin,        0 } },
	{ QSE_T("sinh"),    { { 1, 1, QSE_NULL },     fnc_sinh,       0 } },
	{ QSE_T("sqrt"),    { { 1, 1, QSE_NULL },     fnc_sqrt,       0 } },
	{ QSE_T("srand"),   { { 0, 1, QSE_NULL },     fnc_srand,      0 } },
	{ QSE_T("tan"),     { { 1, 1, QSE_NULL },     fnc_tan,        0 } },
	{ QSE_T("tanh"),    { { 1, 1, QSE_NULL },     fnc_tanh,       0 } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

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
		mid = (left + right) / 2;

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
	modctx_t* modctx;

	modctx = (modctx_t*)mod->ctx;
	qse_awk_freemem (awk, modctx);
}

int qse_awk_mod_math (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	modctx_t* modctx;
	qse_ntime_t now;

	modctx = qse_awk_allocmem (awk, QSE_SIZEOF(*modctx));
	if (modctx == QSE_NULL) 
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}

	QSE_MEMSET (modctx, 0, QSE_SIZEOF(*modctx));

	modctx->seed = (qse_gettime (&now) <= -1)? 0u: ((qse_awk_int_t)now.sec + (qse_awk_int_t)now.nsec);
	/* i don't care if the seed becomes negative or overflows.
	 * i just convert the signed value to the unsigned one. */
	modctx->prand = (qse_awk_uint_t)(modctx->seed * modctx->seed * modctx->seed);
	/* make sure that the actual seeding is not 0 */
	if (modctx->prand == 0) modctx->prand++;

	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	mod->ctx = modctx;

	return 0;
}

