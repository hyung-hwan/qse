
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
