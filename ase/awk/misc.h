/*
 * $Id: misc.h,v 1.4 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_MISC_H_
#define _SSE_AWK_MISC_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* sse_awk_memcpy  (void* dst, const void* src, sse_size_t n);
void* sse_awk_memset (void* dst, int val, sse_size_t n);

sse_char_t* sse_awk_strtok (
	sse_awk_run_t* run, const sse_char_t* s, 
	const sse_char_t* delim, sse_char_t** tok, sse_size_t* tok_len);

sse_char_t* sse_awk_strxtok (
	sse_awk_run_t* run, const sse_char_t* s, sse_size_t len,
	const sse_char_t* delim, sse_char_t** tok, sse_size_t* tok_len);

sse_char_t* sse_awk_strntok (
	sse_awk_run_t* run, const sse_char_t* s, 
	const sse_char_t* delim, sse_size_t delim_len,
	sse_char_t** tok, sse_size_t* tok_len);

sse_char_t* sse_awk_strxntok (
	sse_awk_run_t* run, const sse_char_t* s, sse_size_t len,
	const sse_char_t* delim, sse_size_t delim_len,
	sse_char_t** tok, sse_size_t* tok_len);

sse_char_t* sse_awk_strxntokbyrex (
	sse_awk_run_t* run, const sse_char_t* s, sse_size_t len,
	void* rex, sse_char_t** tok, sse_size_t* tok_len, int* errnum);

int sse_awk_abort (sse_awk_t* awk, 
	const sse_char_t* esser, const sse_char_t* file, int line);

#ifdef __cplusplus
}
#endif

#endif

