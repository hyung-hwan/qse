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

/*
 * This file contains a formatted output routine derived from kvprintf() 
 * of FreeBSD. It has been heavily modified and bug-fixed.
 */

/*
 * Copyright (c) 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static char_t* sprintn (char_t* nbuf, qse_uintmax_t num, int base, int *lenp, int upper)
{
	char_t *p, c;

	p = nbuf;
	*p = T('\0');
	do 
	{
		c = hex2ascii(num % base);
		*++p = upper ? toupper(c) : c;
	} 
	while (num /= base);

	if (lenp) *lenp = p - nbuf;
	return p;
}

/* NOTE: data output is aborted if the data limit is reached or 
 *       I/O error occurs  */

#undef PUT_CHAR

#define PUT_CHAR(c) do { \
	int xx; \
	if (data->count >= data->limit) goto done; \
	if ((xx = data->put (c, data->ctx)) <= -1) goto oops; \
	if (xx == 0) goto done; \
	data->count++; \
} while (0)

int fmtout (const char_t* fmt, fmtout_t* data, va_list ap)
{
	char_t nbuf[MAXNBUF];
	const char_t* p, * percent;
	int n, base, tmp, width, neg, sign, precision, upper;
	uchar_t ch; 
	char_t ach, padc, * sp;
	ochar_t oach, * osp;
	qse_size_t oslen, slen;
	int lm_flag, lm_dflag, flagc, numlen;
	qse_uintmax_t num = 0;
	int stop = 0;

	struct
	{
		qse_mchar_t  sbuf[32];	
		qse_mchar_t* ptr;
		qse_size_t   capa;
	} fltfmt;

	struct
	{
		char_t       sbuf[96];	
		char_t*      ptr;
		qse_size_t   capa;
	} fltout;

	data->count = 0;

	fltfmt.ptr  = fltfmt.sbuf;
	fltfmt.capa = QSE_COUNTOF(fltfmt.sbuf) - 1;

	fltout.ptr  = fltout.sbuf;
	fltout.capa = QSE_COUNTOF(fltout.sbuf) - 1;

	while (1)
	{
		while ((ch = (uchar_t)*fmt++) != T('%') || stop) 
		{
			if (ch == T('\0')) goto done;
			PUT_CHAR (ch);
		}
		percent = fmt - 1;

		padc = T(' '); 
		width = 0; precision = 0;
		neg = 0; sign = 0; upper = 0;

		lm_flag = 0; lm_dflag = 0; flagc = 0; 

reswitch:	
		switch (ch = (uchar_t)*fmt++) 
		{
		case T('%'): /* %% */
			ach = ch;
			goto print_lowercase_c;
			break;

		/* flag characters */
		case T('.'):
			if (flagc & FLAGC_DOT) goto invalid_format;
			flagc |= FLAGC_DOT;
			goto reswitch;

		case T('#'): 
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SHARP;
			goto reswitch;

		case T(' '):
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SPACE;
			goto reswitch;

		case T('+'): /* place sign for signed conversion */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SIGN;
			goto reswitch;

		case T('-'): /* left adjusted */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			if (flagc & FLAGC_DOT)
			{
				goto invalid_format;
			}
			else
			{
				flagc |= FLAGC_LEFTADJ;
				if (flagc & FLAGC_ZEROPAD)
				{
					padc = T(' ');
					flagc &= ~FLAGC_ZEROPAD;
				}
			}
			
			goto reswitch;

		case T('*'): /* take the length from the parameter */
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & (FLAGC_STAR2 | FLAGC_PRECISION)) goto invalid_format;
				flagc |= FLAGC_STAR2;

				precision = va_arg(ap, int);
				if (precision < 0) 
				{
					/* if precision is less than 0, 
					 * treat it as if no .precision is specified */
					flagc &= ~FLAGC_DOT;
					precision = 0;
				}
			} 
			else 
			{
				if (flagc & (FLAGC_STAR1 | FLAGC_WIDTH)) goto invalid_format;
				flagc |= FLAGC_STAR1;

				width = va_arg(ap, int);
				if (width < 0) 
				{
					/*
					if (flagc & FLAGC_LEFTADJ) 
						flagc  &= ~FLAGC_LEFTADJ;
					else
					*/
						flagc |= FLAGC_LEFTADJ;
					width = -width;
				}
			}
			goto reswitch;

		case T('0'): /* zero pad */
			if (flagc & FLAGC_LENMOD) goto invalid_format;
			if (!(flagc & (FLAGC_DOT | FLAGC_LEFTADJ)))
			{
				padc = T('0');
				flagc |= FLAGC_ZEROPAD;
				goto reswitch;
			}
		/* end of flags characters */

		case T('1'): case T('2'): case T('3'): case T('4'):
		case T('5'): case T('6'): case T('7'): case T('8'): case T('9'):
			for (n = 0;; ++fmt) 
			{
				n = n * 10 + ch - T('0');
				ch = *fmt;
				if (ch < T('0') || ch > T('9')) break;
			}
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & FLAGC_STAR2) goto invalid_format;
				precision = n;
				flagc |= FLAGC_PRECISION;
			}
			else 
			{
				if (flagc & FLAGC_STAR1) goto invalid_format;
				width = n;
				flagc |= FLAGC_WIDTH;
			}
			goto reswitch;

		/* length modifiers */
		case T('h'): /* short int */
		case T('l'): /* long int */
		case T('q'): /* long long int */
		case T('j'): /* uintmax_t */
		case T('z'): /* size_t */
		case T('t'): /* ptrdiff_t */
			if (lm_flag & (LF_LD | LF_QD)) goto invalid_format;

			flagc |= FLAGC_LENMOD;
			if (lm_dflag)
			{
				/* error */
				goto invalid_format;
			}
			else if (lm_flag)
			{
				if (lm_tab[ch - T('a')].dflag && lm_flag == lm_tab[ch - T('a')].flag)
				{
					lm_flag &= ~lm_tab[ch - T('a')].flag;
					lm_flag |= lm_tab[ch - T('a')].dflag;
					lm_dflag |= lm_flag;
					goto reswitch;
				}
				else
				{
					/* error */
					goto invalid_format;
				}
			}
			else 
			{
				lm_flag |= lm_tab[ch - T('a')].flag;
				goto reswitch;
			}
			break;

		case T('L'): /* long double */
			if (flagc & FLAGC_LENMOD) 
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_LD;
			goto reswitch;

		case T('Q'): /* __float128 */
			if (flagc & FLAGC_LENMOD)
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_QD;
			goto reswitch;
			
		/* end of length modifiers */

		case T('n'):
			if (lm_flag & LF_J) /* j */
				*(va_arg(ap, qse_intmax_t*)) = data->count;
			else if (lm_flag & LF_Z) /* z */
				*(va_arg(ap, qse_size_t*)) = data->count;
		#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q) /* ll */
				*(va_arg(ap, long long int*)) = data->count;
		#endif
			else if (lm_flag & LF_L) /* l */
				*(va_arg(ap, long int*)) = data->count;
			else if (lm_flag & LF_H) /* h */
				*(va_arg(ap, short int*)) = data->count;
			else if (lm_flag & LF_C) /* hh */
				*(va_arg(ap, char*)) = data->count;
			else if (flagc & FLAGC_LENMOD)
				goto oops;
			else
				*(va_arg(ap, int*)) = data->count;
			break;


		/* signed integer conversions */
		case T('d'):
		case T('i'): /* signed conversion */
			base = 10;
			sign = 1;
			goto handle_sign;
		/* end of signed integer conversions */

		/* unsigned integer conversions */
		case T('o'): 
			base = 8;
			goto handle_nosign;
		case T('u'):
			base = 10;
			goto handle_nosign;
		case T('X'):
			upper = 1;
		case T('x'):
			base = 16;
			goto handle_nosign;
		/* end of unsigned integer conversions */

		case T('p'): /* pointer */
			base = 16;

			if (width == 0) flagc |= FLAGC_SHARP;
			else flagc &= ~FLAGC_SHARP;

			num = (qse_uintptr_t)va_arg(ap, void*);
			goto number;

		case T('c'):
			/* zerpad must not take effect for 'c' */
			if (flagc & FLAGC_ZEROPAD) padc = QSE_T(' '); 
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_c;
		lowercase_c:
			ach = QSE_SIZEOF(char_t) < QSE_SIZEOF(int)? va_arg(ap, int): va_arg(ap, char_t);

		print_lowercase_c:
			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}
			PUT_CHAR (ach);
			if ((flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}
			break;

		case T('C'):
			/* zerpad must not take effect for 'C' */
			if (flagc & FLAGC_ZEROPAD) padc = QSE_T(' ');
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t)))) goto lowercase_c;
		uppercase_c:
			oach = QSE_SIZEOF(ochar_t) < QSE_SIZEOF(int)? va_arg(ap, int): va_arg(ap, ochar_t);

			oslen = 1;
			if (data->conv (&oach, &oslen, QSE_NULL, &slen, data->ctx) <= -1)
			{
				/* conversion error */
				goto oops;
			}

			/* precision 0 doesn't kill the letter */
			width -= slen;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}

			{
				char_t conv_buf[CONV_MAX]; 
				qse_size_t i, conv_len;

				oslen = 1;
				conv_len = QSE_COUNTOF(conv_buf);

				/* this must not fail since the dry-run above was successful */
				data->conv (&oach, &oslen, conv_buf, &conv_len, data->ctx);

				for (i = 0; i < conv_len; i++)
				{
					PUT_CHAR (conv_buf[i]);
				}
			}

			if ((flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}
			break;

		case T('s'):
			/* zerpad must not take effect for 's' */
			if (flagc & FLAGC_ZEROPAD) padc = QSE_T(' ');
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_s;
		lowercase_s:
			sp = va_arg (ap, char_t*);
			if (sp == QSE_NULL) p = T("(null)");

		print_lowercase_s:
			if (flagc & FLAGC_DOT)
			{
				for (n = 0; n < precision && sp[n]; n++);
			}
			else
			{
				for (n = 0; sp[n]; n++);
			}

			width -= n;
		
			if (!(flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR(padc);
			}
			while (n--) PUT_CHAR(*sp++);
			if ((flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR(padc);
			}
			break;

		case T('S'):
			/* zerpad must not take effect for 'S' */
			if (flagc & FLAGC_ZEROPAD) padc = QSE_T(' ');
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t)))) goto lowercase_s;
		uppercase_s:

			osp = va_arg (ap, ochar_t*);
			if (osp == QSE_NULL) osp = OT("(null)");

			/* get the length */
			for (oslen = 0; osp[oslen]; oslen++);

			if (data->conv (osp, &oslen, QSE_NULL, &slen, data->ctx) <= -1)
			{
				/* conversion error */
				goto oops;
			}

			/* slen hold the length after conversion */
			n = slen;
			if ((flagc & FLAGC_DOT) && precision < slen) n = precision;
			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}

			{
				char_t conv_buf[CONV_MAX]; 
				qse_size_t i, conv_len, src_len, tot_len = 0;
				while (n > 0)
				{
					QSE_ASSERT (oslen > tot_len);

				#if CONV_MAX == 1
					src_len = oslen - tot_len;
				#else
					src_len = 1;
				#endif
					conv_len = QSE_COUNTOF(conv_buf);

					/* this must not fail since the dry-run above was successful */
					data->conv (&osp[tot_len], &src_len, conv_buf, &conv_len, data->ctx);
					tot_len += src_len;

					/* stop outputting if a converted character can't be printed 
					 * in its entirety (limited by precision). but this is not an error */
					if (n < conv_len) break; 

					for (i = 0; i < conv_len; i++)
					{
						PUT_CHAR (conv_buf[i]);
					}

					n -= conv_len;
				}
			}
			
			if ((flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_CHAR (padc);
			}
			break;

		case T('e'):
		case T('E'):
		case T('f'):
		case T('F'):
		case T('g'):
		case T('G'):
		/*
		case T('a'):
		case T('A'):
		*/
		{
			/* let me rely on snprintf until i implement float-point to string conversion */
			int q;
			qse_size_t fmtlen;
		#if (QSE_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			__float128 v_qd;
		#endif
			long double v_ld;
			double v_d;
			int dtype = 0;
			qse_size_t newcapa;

			if (lm_flag & LF_J)
			{
			#if (QSE_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF) && (QSE_SIZEOF_FLTMAX_T == QSE_SIZEOF___FLOAT128)
				v_qd = va_arg (ap, qse_fltmax_t);
				dtype = LF_QD;
			#elif QSE_SIZEOF_FLTMAX_T == QSE_SIZEOF_DOUBLE
				v_d = va_arg (ap, qse_fltmax_t);
			#elif QSE_SIZEOF_FLTMAX_T == QSE_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, qse_fltmax_t);
				dtype = LF_LD;
			#else
				#error Unsupported qse_flt_t
			#endif
			}
			else if (lm_flag & LF_Z)
			{
				/* qse_flt_t is limited to double or long double */

				/* precedence goes to double if sizeof(double) == sizeof(long double) 
				 * for example, %Lf didn't work on some old platforms.
				 * so i prefer the format specifier with no modifier.
				 */
			#if QSE_SIZEOF_FLT_T == QSE_SIZEOF_DOUBLE
				v_d = va_arg (ap, qse_flt_t);
			#elif QSE_SIZEOF_FLT_T == QSE_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, qse_flt_t);
				dtype = LF_LD;
			#else
				#error Unsupported qse_flt_t
			#endif
			}
			else if (lm_flag & (LF_LD | LF_L))
			{
				v_ld = va_arg (ap, long double);
				dtype = LF_LD;
			}
		#if (QSE_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			else if (lm_flag & (LF_QD | LF_Q))
			{
				v_qd = va_arg (ap, __float128);	
				dtype = LF_QD;
			}
		#endif
			else if (flagc & FLAGC_LENMOD)
			{
				goto oops;
			}
			else
			{
				v_d = va_arg (ap, double);
			}

			fmtlen = fmt - percent;
			if (fmtlen > fltfmt.capa)
			{
				if (fltfmt.ptr == fltfmt.sbuf)
				{
					fltfmt.ptr = QSE_MMGR_ALLOC (QSE_MMGR_GETDFL(), QSE_SIZEOF(*fltfmt.ptr) * (fmtlen + 1));
					if (fltfmt.ptr == QSE_NULL) goto oops;
				}
				else
				{
					qse_mchar_t* tmpptr;

					tmpptr = QSE_MMGR_REALLOC (QSE_MMGR_GETDFL(), fltfmt.ptr, QSE_SIZEOF(*fltfmt.ptr) * (fmtlen + 1));
					if (tmpptr == QSE_NULL) goto oops;
					fltfmt.ptr = tmpptr;
				}

				fltfmt.capa = fmtlen;
			}

			/* compose back the format specifier */
			fmtlen = 0;
			fltfmt.ptr[fmtlen++] = QSE_MT('%');
			if (flagc & FLAGC_SPACE) fltfmt.ptr[fmtlen++] = QSE_T(' ');
			if (flagc & FLAGC_SHARP) fltfmt.ptr[fmtlen++] = QSE_T('#');
			if (flagc & FLAGC_SIGN) fltfmt.ptr[fmtlen++] = QSE_T('+');
			if (flagc & FLAGC_LEFTADJ) fltfmt.ptr[fmtlen++] = QSE_T('-');
			if (flagc & FLAGC_ZEROPAD) fltfmt.ptr[fmtlen++] = QSE_T('0');

			if (flagc & FLAGC_STAR1) fltfmt.ptr[fmtlen++] = QSE_T('*');
			else if (flagc & FLAGC_WIDTH) 
			{
				fmtlen += qse_fmtuintmaxtombs (
					&fltfmt.ptr[fmtlen], fltfmt.capa - fmtlen, 
					width, 10, -1, QSE_MT('\0'), QSE_NULL);
			}
			if (flagc & FLAGC_DOT) fltfmt.ptr[fmtlen++] = QSE_T('.');
			if (flagc & FLAGC_STAR2) fltfmt.ptr[fmtlen++] = QSE_T('*');
			else if (flagc & FLAGC_PRECISION) 
			{
				fmtlen += qse_fmtuintmaxtombs (
					&fltfmt.ptr[fmtlen], fltfmt.capa - fmtlen, 
					precision, 10, -1, QSE_MT('\0'), QSE_NULL);
			}

			if (dtype == LF_LD)
				fltfmt.ptr[fmtlen++] = QSE_MT('L');
		#if (QSE_SIZEOF___FLOAT128 > 0)
			else if (dtype == LF_QD)
				fltfmt.ptr[fmtlen++] = QSE_MT('Q');
		#endif

			fltfmt.ptr[fmtlen++] = ch;
			fltfmt.ptr[fmtlen] = QSE_MT('\0');

		#if defined(HAVE_SNPRINTF)
			/* nothing special here */
		#else
			/* best effort to avoid buffer overflow when no snprintf is available. 
			 * i really can't do much if it happens. */
			newcapa = precision + width + 32;
			if (fltout.capa < newcapa)
			{
				QSE_ASSERT (fltout.ptr == fltout.sbuf);

				fltout.ptr = QSE_MMGR_ALLOC (QSE_MMGR_GETDFL(), QSE_SIZEOF(char_t) * (newcapa + 1));
				if (fltout.ptr == QSE_NULL) goto oops;
				fltout.capa = newcapa;
			}
		#endif

			while (1)
			{

				if (dtype == LF_LD)
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((qse_mchar_t*)fltout.ptr, fltout.capa + 1, fltfmt.ptr, v_ld);
				#else
					q = sprintf ((qse_mchar_t*)fltout.ptr, fltfmt.ptr, v_ld);
				#endif
				}
			#if (QSE_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
				else if (dtype == LF_QD)
				{
					q = quadmath_snprintf ((qse_mchar_t*)fltout.ptr, fltout.capa + 1, fltfmt.ptr, v_qd);
				}
			#endif
				else
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((qse_mchar_t*)fltout.ptr, fltout.capa + 1, fltfmt.ptr, v_d);
				#else
					q = sprintf ((qse_mchar_t*)fltout.ptr, fltfmt.ptr, v_d);
				#endif
				}
				if (q <= -1) goto oops;
				if (q <= fltout.capa) break;

				newcapa = fltout.capa * 2;
				if (newcapa < q) newcapa = q;

				if (fltout.ptr == fltout.sbuf)
				{
					fltout.ptr = QSE_MMGR_ALLOC (QSE_MMGR_GETDFL(), QSE_SIZEOF(char_t) * (newcapa + 1));
					if (fltout.ptr == QSE_NULL) goto oops;
				}
				else
				{
					char_t* tmpptr;

					tmpptr = QSE_MMGR_REALLOC (QSE_MMGR_GETDFL(), fltout.ptr, QSE_SIZEOF(char_t) * (newcapa + 1));
					if (tmpptr == QSE_NULL) goto oops;
					fltout.ptr = tmpptr;
				}
				fltout.capa = newcapa;
			}

			if (QSE_SIZEOF(char_t) != QSE_SIZEOF(qse_mchar_t))
			{
				fltout.ptr[q] = T('\0');	
				while (q > 0)
				{
					q--;
					fltout.ptr[q] = ((qse_mchar_t*)fltout.ptr)[q];	
				}
			}

			sp = fltout.ptr;
			flagc &= ~FLAGC_DOT;
			width = 0;
			precision = 0;
			goto print_lowercase_s;
		}

handle_nosign:
			sign = 0;
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (QSE_SIZEOF_UINTMAX_T > QSE_SIZEOF_SIZE_T) && \
			    (QSE_SIZEOF_UINTMAX_T != QSE_SIZEOF_LONG_LONG) && \
			    (QSE_SIZEOF_UINTMAX_T != QSE_SIZEOF_LONG)
				/* GCC-compiled binaries crashed when getting qse_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < QSE_SIZEOF(qse_uintmax_t) / QSE_SIZEOF(qse_size_t); i++)
				{	
				#if defined(QSE_ENDIAN_BIG)
					num = num << (8 * QSE_SIZEOF(qse_size_t)) | (va_arg (ap, qse_size_t));
				#else
					register int shift = i * QSE_SIZEOF(qse_size_t);
					qse_size_t x = va_arg (ap, qse_size_t);
					num |= (qse_uintmax_t)x << (shift * 8);
				#endif
				}
			#else
				num = va_arg (ap, qse_uintmax_t);
			#endif
			}
			else if (lm_flag & LF_T)
				num = va_arg (ap, qse_ptrdiff_t);
			else if (lm_flag & LF_Z)
				num = va_arg (ap, qse_size_t);
			#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg (ap, unsigned long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg (ap, unsigned long int);
			else if (lm_flag & LF_H)
				num = (unsigned short int)va_arg (ap, int);
			else if (lm_flag & LF_C)
				num = (unsigned char)va_arg (ap, int);
			else
				num = va_arg (ap, unsigned int);
			goto number;

handle_sign:
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (QSE_SIZEOF_INTMAX_T > QSE_SIZEOF_SIZE_T) && \
			    (QSE_SIZEOF_UINTMAX_T != QSE_SIZEOF_LONG_LONG) && \
			    (QSE_SIZEOF_UINTMAX_T != QSE_SIZEOF_LONG)
				/* GCC-compiled binraries crashed when getting qse_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < QSE_SIZEOF(qse_intmax_t) / QSE_SIZEOF(qse_size_t); i++)
				{
				#if defined(QSE_ENDIAN_BIG)
					num = num << (8 * QSE_SIZEOF(qse_size_t)) | (va_arg (ap, qse_size_t));
				#else
					register int shift = i * QSE_SIZEOF(qse_size_t);
					qse_size_t x = va_arg (ap, qse_size_t);
					num |= (qse_uintmax_t)x << (shift * 8);
				#endif
				}
			#else
				num = va_arg (ap, qse_intmax_t);
			#endif
			}

			else if (lm_flag & LF_T)
				num = va_arg(ap, qse_ptrdiff_t);
			else if (lm_flag & LF_Z)
				num = va_arg (ap, qse_ssize_t);
			#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg (ap, long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg (ap, long int);
			else if (lm_flag & LF_H)
				num = (short int)va_arg (ap, int);
			else if (lm_flag & LF_C)
				num = (char)va_arg (ap, int);
			else
				num = va_arg (ap, int);

number:
			if (sign && (qse_intmax_t)num < 0) 
			{
				neg = 1;
				num = -(qse_intmax_t)num;
			}
			p = sprintn (nbuf, num, base, &tmp, upper);
			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 8) tmp++;
				else if (base == 16) tmp += 2;
			}
			if (neg) tmp++;
			else if (flagc & FLAGC_SIGN) tmp++;
			else if (flagc & FLAGC_SPACE) tmp++;

			numlen = p - nbuf;
			if ((flagc & FLAGC_DOT) && precision > numlen) 
			{
				/* extra zeros fro precision specified */
				tmp += (precision - numlen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && !(flagc & FLAGC_ZEROPAD) && width > 0 && (width -= tmp) > 0)
			{
				while (width--) PUT_CHAR(padc);
			}

			if (neg) PUT_CHAR(T('-'));
			else if (flagc & FLAGC_SIGN) PUT_CHAR(T('+'));
			else if (flagc & FLAGC_SPACE) PUT_CHAR(T(' '));

			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 8) 
				{
					PUT_CHAR(T('0'));
				} 
				else if (base == 16) 
				{
					PUT_CHAR(T('0'));
					PUT_CHAR(T('x'));
				}
			}

			if ((flagc & FLAGC_DOT) && precision > numlen)
			{
				/* extra zeros for precision specified */
				while (numlen < precision) 
				{
					PUT_CHAR (T('0'));
					numlen++;
				}
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				while (width-- > 0) PUT_CHAR (padc);
			}

			while (*p) PUT_CHAR(*p--); /* output actual digits */

			if ((flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				while (width-- > 0) PUT_CHAR (padc);
			}
			break;

invalid_format:
			while (percent < fmt) PUT_CHAR(*percent++);
			break;

		default:
			while (percent < fmt) PUT_CHAR(*percent++);
			/*
			 * Since we ignore an formatting argument it is no
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			stop = 1;
			break;
		}
	}

done:
	if (fltfmt.ptr != fltfmt.sbuf)
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), fltfmt.ptr);
	if (fltout.ptr != fltout.sbuf)
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), fltout.ptr);
	return 0;

oops:
	if (fltfmt.ptr != fltfmt.sbuf)
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), fltfmt.ptr);
	if (fltout.ptr != fltout.sbuf)
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), fltout.ptr);
	return (qse_ssize_t)-1;
}
#undef PUT_CHAR

