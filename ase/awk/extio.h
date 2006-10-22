/*
 * $Id: extio.h,v 1.14 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_EXTIO_H_
#define _SSE_AWK_EXTIO_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C"
#endif

int sse_awk_readextio (
	sse_awk_run_t* run, int in_type, 
	const sse_char_t* name, sse_awk_str_t* buf);

int sse_awk_writeextio_val (
	sse_awk_run_t* run, int out_type, 
	const sse_char_t* name, sse_awk_val_t* v);

int sse_awk_writeextio_str (
	sse_awk_run_t* run, int out_type, 
	const sse_char_t* name, sse_char_t* str, sse_size_t len);

int sse_awk_flushextio (
	sse_awk_run_t* run, int out_type, const sse_char_t* name);

int sse_awk_nextextio_read (
	sse_awk_run_t* run, int in_type, const sse_char_t* name);

/* TODO:
int sse_awk_nextextio_write (
	sse_awk_run_t* run, int out_type, const sse_char_t* name);
*/

int sse_awk_closeextio_read (
	sse_awk_run_t* run, int in_type, const sse_char_t* name);
int sse_awk_closeextio_write (
	sse_awk_run_t* run, int out_type, const sse_char_t* name);
int sse_awk_closeextio (sse_awk_run_t* run, const sse_char_t* name);

void sse_awk_clearextio (sse_awk_run_t* run);

#ifdef __cplusplus
}
#endif

#endif
