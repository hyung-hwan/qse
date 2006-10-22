/*
 * $Id: func.h,v 1.13 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_FUNC_H_
#define _SSE_AWK_FUNC_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

typedef struct sse_awk_bfn_t sse_awk_bfn_t;

struct sse_awk_bfn_t
{
	const sse_char_t* name; 
	sse_size_t name_len;
	int valid; /* the entry is valid when this option is set */

	sse_size_t min_args;
	sse_size_t max_args;
	const sse_char_t* arg_spec;
	int (*handler) (sse_awk_run_t* run);

	sse_awk_bfn_t* next;
};

enum
{
	/* ensure that this matches __sys_bfn in func.c */

	SSE_AWK_BFN_CLOSE,
	SSE_AWK_BFN_INDEX,
	SSE_AWK_BFN_LENGTH,
	SSE_AWK_BFN_SYSTEM,
	SSE_AWK_BFN_SIN
};


#ifdef __cplusplus
extern "C" {
#endif

sse_awk_bfn_t* sse_awk_addbfn (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len, 
	int when_valid, sse_size_t min_args, sse_size_t max_args, 
	const sse_char_t* arg_spec, int (*handler)(sse_awk_run_t*));

int sse_awk_delbfn (sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len);

void sse_awk_clrbfn (sse_awk_t* awk);

sse_awk_bfn_t* sse_awk_getbfn (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len);

#ifdef __cplusplus
}
#endif

#endif
