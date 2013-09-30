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
	return (p);
}

#define PCHAR(c) do { \
	func (c, arg); \
	retval++; \
} while (0)

#define OPCHAR(c) do { \
	ofunc (c, arg); \
	retval++; \
} while (0)

int xprintf (char_t const *fmt, void (*func)(char_t, void*), void (*ofunc) (ochar_t, void*), void *arg, va_list ap)
{
	char_t nbuf[MAXNBUF];
	const char_t* p, * percent;
	uchar_t ch; 
	int n;
	qse_uintmax_t num;
	int base, tmp, width, ladjust, sharpflag, neg, sign, dot;
	int dwidth, upper;
	char_t padc, * sp;
	ochar_t opadc, * osp;
	int stop = 0, retval = 0;
	int lm_flag, lm_dflag;

	num = 0;

	if (fmt == QSE_NULL) fmt = T("(fmt null)\n");

	while (1)
	{
		padc = T(' ');
		opadc = OT(' ');

		width = 0;
		while ((ch = (uchar_t)*fmt++) != T('%') || stop) 
		{
			if (ch == T('\0')) return (retval);
			PCHAR(ch);
		}
		percent = fmt - 1;
		ladjust = 0; sharpflag = 0; neg = 0;
		sign = 0; dot = 0; dwidth = 0; upper = 0;

		lm_flag = 0; lm_dflag = 0;

reswitch:	
		switch (ch = (uchar_t)*fmt++) 
		{
		case T('%'): /* %% */
			PCHAR(ch);
			break;

		case T('.'):
			dot = 1;
			goto reswitch;
		case T('#'):
			sharpflag = 1;
			goto reswitch;
		case T('+'):
			sign = 1;
			goto reswitch;
		case T('-'):
			ladjust = 1;
			goto reswitch;

		case T('*'):
			if (!dot) 
			{
				width = va_arg(ap, int);
				if (width < 0) 
				{
					ladjust = !ladjust;
					width = -width;
				}
			} 
			else 
			{
				dwidth = va_arg(ap, int);
			}
			goto reswitch;

		case T('0'):
			if (!dot) 
			{
				padc = T('0');
				opadc = OT('0');
				goto reswitch;
			}
		case T('1'): case T('2'): case T('3'): case T('4'):
		case T('5'): case T('6'): case T('7'): case T('8'): case T('9'):
			for (n = 0;; ++fmt) 
			{
				n = n * 10 + ch - T('0');
				ch = *fmt;
				if (ch < T('0') || ch > T('9')) break;
			}
			if (dot) dwidth = n;
			else width = n;
			goto reswitch;

		case T('c'):
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_c;
		lowercase_c:
			if (QSE_SIZEOF(char_t) < QSE_SIZEOF(int))
				PCHAR(va_arg(ap, int));
			else
				PCHAR(va_arg(ap, char_t));
			break;

		case T('C'):
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t)))) goto lowercase_c;
		uppercase_c:
			if (QSE_SIZEOF(ochar_t) < QSE_SIZEOF(int))
				OPCHAR(va_arg(ap, int));
			else
				OPCHAR(va_arg(ap, ochar_t));
			break;


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
				PCHAR (fmt[-4]);
				PCHAR (fmt[-3]);
				PCHAR (fmt[-2]);
				PCHAR (fmt[-1]);
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
					PCHAR (fmt[-3]);
					PCHAR (fmt[-2]);
					PCHAR (fmt[-1]);
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

		case T('o'):
			base = 8;
			goto handle_nosign;
		case T('d'):
		case T('i'):
			base = 10;
			sign = 1;
			goto handle_sign;
		case T('u'):
			base = 10;
			goto handle_nosign;
		case T('X'):
			upper = 1;
		case T('x'):
			base = 16;
			goto handle_nosign;
		case T('y'):
			base = 16;
			sign = 1;
			goto handle_sign;

		case T('p'):
			base = 16;
			sharpflag = (width == 0);
			sign = 0;
			num = (qse_uintptr_t)va_arg(ap, void *);
			goto number;

		case T('s'):
		{
			if (((lm_flag & LF_H) && (QSE_SIZEOF(char_t) > QSE_SIZEOF(ochar_t))) ||
			    ((lm_flag & LF_L) && (QSE_SIZEOF(char_t) < QSE_SIZEOF(ochar_t)))) goto uppercase_s;
		lowercase_s:
			sp = va_arg (ap, char_t *);
			if (sp == QSE_NULL) p = T("(null)");
			if (!dot) 
			{
				char_t* p = sp;
				while (*p) p++;
				n = p - sp;
			}
			else
			{
				for (n = 0; n < dwidth && sp[n]; n++) continue;
			}

			width -= n;

			if (!ladjust && width > 0)
			{
				while (width--) PCHAR(padc);
			}
			while (n--) PCHAR(*sp++);
			if (ladjust && width > 0)
			{
				while (width--) PCHAR(padc);
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
			if (!dot)
			{
				ochar_t* p = osp;
				while (*p) p++;
				n = p - osp;
			}
			else
			{
				for (n = 0; n < dwidth && osp[n]; n++) continue;
			}

			width -= n;

			if (!ladjust && width > 0)
			{
				while (width--) OPCHAR (opadc);
			}
			while (n--) OPCHAR(*osp++);
			if (ladjust && width > 0)
			{
				while (width--) OPCHAR (opadc);
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
			if (sharpflag && num != 0) 
			{
				if (base == 8) tmp++;
				else if (base == 16) tmp += 2;
			}
			if (neg) tmp++;

			if (!ladjust && padc != T('0') && width && (width -= tmp) > 0)
			{
				while (width--) PCHAR(padc);
			}
			if (neg) PCHAR(T('-'));

			if (sharpflag && num != 0) 
			{
				if (base == 8) {
					PCHAR(T('0'));
				} 
				else if (base == 16) 
				{
					PCHAR(T('0'));
					PCHAR(T('x'));
				}
			}
			if (!ladjust && width && (width -= tmp) > 0)
			{
				while (width--) PCHAR(padc);
			}

			while (*p) PCHAR(*p--);

			if (ladjust && width && (width -= tmp) > 0)
			{
				while (width--) PCHAR(padc);
			}

			break;

		default:
			while (percent < fmt) PCHAR(*percent++);
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
#undef PCHAR
#undef OPCHAR

