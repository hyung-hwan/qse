/*
 * $Id: parse.h,v 1.2 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_PARSE_H_
#define _SSE_AWK_PARSE_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

int sse_awk_putsrcstr (sse_awk_t* awk, const sse_char_t* str);
int sse_awk_putsrcstrx (
	sse_awk_t* awk, const sse_char_t* str, sse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
