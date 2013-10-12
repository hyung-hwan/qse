/*-
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
 *	@(#)subr_prf.c	8.3 (Berkeley) 1/21/94
 */

#define NBBY    8               /* number of bits in a byte */

/* Max number conversion buffer length: a u_quad_t in base 2, plus NUL byte. */
#define MAXNBUF	(QSE_SIZEOF(qse_intmax_t) * NBBY + 1)

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

#undef PUT_CHAR
#undef PUT_OCHAR

/* TODO: error check */
#define PUT_CHAR(c) do { \
	put_char (c, arg); \
	retval++; \
} while (0)

#define PUT_OCHAR(c) do { \
	put_ochar (c, arg); \
	retval++; \
} while (0)

int xprintf (char_t const *fmt, void (*put_char)(char_t, void*), void (*put_ochar) (ochar_t, void*), void *arg, va_list ap)
{
	char_t nbuf[MAXNBUF];
	const char_t* p, * percent;
	uchar_t ch; 
	int n;
	qse_uintmax_t num;
	int base, tmp, width, neg, sign;
	int precision, upper;
	char_t padc, * sp;
	ochar_t opadc, * osp;
	int stop = 0, retval = 0;
	int lm_flag, lm_dflag, flagc;
	int numlen;

	num = 0;

	/*if (fmt == QSE_NULL) fmt = T("(fmt null)\n");*/

	while (1)
	{

		while ((ch = (uchar_t)*fmt++) != T('%') || stop) 
		{
			if (ch == T('\0')) return retval;
			PUT_CHAR(ch);
		}
		percent = fmt - 1;

		padc = T(' '); opadc = OT(' ');
		width = 0; precision = 0;
		neg = 0; sign = 0; upper = 0;

		lm_flag = 0; lm_dflag = 0; flagc = 0; 

reswitch:	
		switch (ch = (uchar_t)*fmt++) 
		{
		case T('%'): /* %% */
			PUT_CHAR(ch);
			break;

		/* flag characters */
		case T('.'):
			flagc |= FLAGC_DOT;
			goto reswitch;

		case T('#'): 
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT)) goto invalid_format;
			flagc |= FLAGC_SHARP;
			goto reswitch;

		case T(' '):
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT)) goto invalid_format;
			flagc |= FLAGC_SPACE;
			goto reswitch;

		case T('+'): /* place sign for signed conversion */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT)) goto invalid_format;
			flagc |= FLAGC_SIGN;
			goto reswitch;

		case T('-'): /* left adjusted */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT)) goto invalid_format;
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
					opadc = OT(' ');
					flagc &= ~FLAGC_ZEROPAD;
				}
			}
			
			goto reswitch;

		case T('*'): /* take the length from the parameter */
			if (!(flagc & FLAGC_DOT)) 
			{
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
			else 
			{
				precision = va_arg(ap, int);
				if (precision < 0) 
				{
					/* if precision is less than 0, 
					 * treat it as if no .precision is specified */
					flagc &= ~FLAGC_DOT;
					precision = 0;
				}
			}
			goto reswitch;

		case T('0'): /* zero pad */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT)) goto invalid_format;
			if (!(flagc & FLAGC_LEFTADJ))
			{
				padc = T('0');
				opadc = OT('0');
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
			if (flagc & FLAGC_DOT) precision = n;
			else 
			{
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
			if (lm_dflag)
			{
				/* error */
				PUT_CHAR (fmt[-4]);
				PUT_CHAR (fmt[-3]);
				PUT_CHAR (fmt[-2]);
				PUT_CHAR (fmt[-1]);
				break;
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
					PUT_CHAR (fmt[-3]);
					PUT_CHAR (fmt[-2]);
					PUT_CHAR (fmt[-1]);
					break;
				}
			}
			else 
			{
				lm_flag |= lm_tab[ch - T('a')].flag;
				goto reswitch;
			}
			break;
		/* end of length modifiers */

		case T('n'):
			if (lm_flag & LF_J)
				*(va_arg(ap, qse_intmax_t *)) = retval;
#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				*(va_arg(ap, long long int*)) = retval;
#endif
			else if (lm_flag & LF_L)
				*(va_arg(ap, long int*)) = retval;
			else if (lm_flag & LF_Z)
				*(va_arg(ap, qse_size_t*)) = retval;
			else if (lm_flag & LF_H)
				*(va_arg(ap, short int*)) = retval;
			else if (lm_flag & LF_C)
				*(va_arg(ap, char*)) = retval;
			else
				*(va_arg(ap, int*)) = retval;
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
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_c;
		lowercase_c:
			if (QSE_SIZEOF(char_t) < QSE_SIZEOF(int))
				PUT_CHAR(va_arg(ap, int));
			else
				PUT_CHAR(va_arg(ap, char_t));
			break;

		case T('C'):
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t)))) goto lowercase_c;
		uppercase_c:
			if (QSE_SIZEOF(ochar_t) < QSE_SIZEOF(int))
				PUT_OCHAR(va_arg(ap, int));
			else
				PUT_OCHAR(va_arg(ap, ochar_t));
			break;

		case T('s'):
		{
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_s;
		lowercase_s:
			sp = va_arg (ap, char_t *);
			if (sp == QSE_NULL) p = T("(null)");
			if (!(flagc & FLAGC_DOT)) 
			{
				char_t* p = sp;
				while (*p) p++;
				n = p - sp;
			}
			else
			{
				for (n = 0; n < precision && sp[n]; n++) continue;
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
		}

		case T('S'):
		{
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t)))) goto lowercase_s;
		uppercase_s:
			osp = va_arg (ap, ochar_t*);
			if (osp == QSE_NULL) osp = OT("(null)");
			if (!(flagc & FLAGC_DOT)) 
			{
				ochar_t* p = osp;
				while (*p) p++;
				n = p - osp;
			}
			else
			{
				for (n = 0; n < precision && osp[n]; n++) continue;
			}

			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_OCHAR (opadc);
			}
			while (n--) PUT_OCHAR(*osp++);
			if ((flagc & FLAGC_LEFTADJ) && width > 0)
			{
				while (width--) PUT_OCHAR (opadc);
			}
			break;
		}

handle_nosign:
			sign = 0;
			if (lm_flag & LF_J)
				num = va_arg(ap, qse_uintmax_t);
#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg(ap, unsigned long long int);
#endif

#if 0
			else if (lm_flag & LF_T)
				num = va_arg(ap, ptrdiff_t);
#endif

			else if (lm_flag & LF_L)
				num = va_arg(ap, long int);
			else if (lm_flag & LF_Z)
				num = va_arg(ap, qse_size_t);
			else if (lm_flag & LF_H)
				num = (unsigned short int)va_arg(ap, int);
			else if (lm_flag & LF_C)
				num = (unsigned char)va_arg(ap, int);
			else
				num = va_arg(ap, unsigned int);
			goto number;

handle_sign:
			if (lm_flag & LF_J)
				num = va_arg(ap, qse_intmax_t);
#if (QSE_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg(ap, long long int);
#endif
#if 0
			else if (lm_flag & LF_T)
				num = va_arg(ap, ptrdiff_t);
#endif
			else if (lm_flag & LF_L)
				num = va_arg(ap, long int);
			else if (lm_flag & LF_Z)
				num = va_arg(ap, qse_ssize_t);
			else if (lm_flag & LF_H)
				num = (short int)va_arg(ap, int);
			else if (lm_flag & LF_C)
				num = (char)va_arg(ap, int);
			else
				num = va_arg(ap, int);

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

		default:
invalid_format:
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
}
#undef PUT_CHAR
#undef PUT_OCHAR

