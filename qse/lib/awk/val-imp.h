/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

/* this file is supposed to be included by val.c */

int awk_rtx_strtonum (qse_awk_rtx_t* rtx, int option, const char_t* ptr, qse_size_t len, qse_awk_int_t* l, qse_awk_flt_t* r)
{
	const char_t* endptr;
	const char_t* end;

	int strict = QSE_AWK_RTX_STRTONUM_GET_OPTION_STRICT(option);
	int base = QSE_AWK_RTX_STRTONUN_GET_OPTION_BASE(option);

	end = ptr + len;
	*l = awk_strxtoint(rtx->awk, ptr, len, base, &endptr);
	if (endptr < end)
	{
		if (*endptr == _T('.') || *endptr == _T('E') || *endptr == _T('e'))
		{
		handle_float:
			*r = awk_strxtoflt(rtx->awk, ptr, len, &endptr);
			if (strict && endptr < end) return -1;
			return 1; /* flt */
		}
		else if (AWK_ISDIGIT(awk, *endptr))
		{
			const char_t* p = endptr;
			do { p++; } while (p < end && AWK_ISDIGIT(awk, *p));
			if (p < end && (*p == _T('.') || *p == _T('E') || *p == _T('e'))) 
			{
				/* it's probably an floating-point number. 
				 * 
 				 *  BEGIN { b=99; printf "%f\n", (0 b 1.112); }
				 *
				 * for the above code, this function gets '0991.112'.
				 * and endptr points to '9' after qse_awk_strxtoint() as
				 * the numeric string beginning with 0 is treated 
				 * as an octal number. 
				 * 
				 * getting side-tracked, 
				 *   BEGIN { b=99; printf "%f\n", (0 b 1.000); }
				 * the above code cause this function to get 0991, not 0991.000
				 * because of the default CONVFMT '%.6g' which doesn't produce '.000'.
				 * so such a number is not treated as a floating-point number.
				 */
				goto handle_float;
			}
		}
	}

	if (strict && endptr < end) return -1;
	return 0; /* int */
}
