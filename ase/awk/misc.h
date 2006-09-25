/*
 * $Id: misc.h,v 1.2 2006-09-25 06:17:19 bacon Exp $
 */

#ifndef _XP_AWK_MISC_H_
#define _XP_AWK_MISC_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* xp_awk_memcpy  (void* dst, const void* src, xp_size_t n);
void* xp_awk_memset (void* dst, int val, xp_size_t n);

xp_char_t* xp_awk_strtok (
	xp_awk_run_t* awk, const xp_char_t* s, 
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len);

xp_char_t* xp_awk_strxtok (
	xp_awk_run_t* awk, const xp_char_t* s, xp_size_t len,
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len);

xp_char_t* xp_awk_strntok (
	xp_awk_run_t* awk, const xp_char_t* s, 
	const xp_char_t* delim, xp_size_t delim_len,
	xp_char_t** tok, xp_size_t* tok_len);

xp_char_t* xp_awk_strxntok (
	xp_awk_run_t* awk, const xp_char_t* s, xp_size_t len,
	const xp_char_t* delim, xp_size_t delim_len,
	xp_char_t** tok, xp_size_t* tok_len);

xp_char_t* xp_awk_strxntokbyrex (
	xp_awk_run_t* awk, const xp_char_t* s, xp_size_t len,
	void* rex, xp_char_t** tok, xp_size_t* tok_len, int* errnum);

#ifdef __cplusplus
}
#endif

#endif

