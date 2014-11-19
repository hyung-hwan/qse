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

#ifndef _QSE_AWK_STDAWK_H_
#define _QSE_AWK_STDAWK_H_

#include <qse/awk/awk.h>

/** \file
 * This file defines functions and data types that help you create
 * an awk interpreter with less effort. It is designed to be as close
 * to conventional awk implementations as possible.
 * 
 * The source script handler does not evaluate a file name of the "var=val"
 * form as an assignment expression. Instead, it just treats it as a
 * normal file name.
 */

/**
 * The qse_awk_parsestd_type_t type defines standard source I/O types.
 */
enum qse_awk_parsestd_type_t
{
	QSE_AWK_PARSESTD_NULL  = 0, /**< pseudo-value to indicate no script */
	QSE_AWK_PARSESTD_FILE  = 1, /**< file */
	QSE_AWK_PARSESTD_STR   = 2  /**< length-bounded string */
};
typedef enum qse_awk_parsestd_type_t qse_awk_parsestd_type_t;

/**
 * The qse_awk_parsestd_t type defines a source I/O.
 */
struct qse_awk_parsestd_t
{
	qse_awk_parsestd_type_t type;

	union
	{
		struct
		{
			/** file path to open. #QSE_NULL or '-' for stdin/stdout. */
			const qse_char_t*  path; 

			/** a stream created with the file path is set with this
			 * cmgr if it is not #QSE_NULL. */
			qse_cmgr_t*  cmgr; 
		} file;

		/** 
		 * input string or dynamically allocated output string
		 *
		 * For input, the ptr and the len field of str indicates the 
		 * pointer and the length of a string to read. You must set
		 * these fields before calling qse_awk_parsestd().
		 *
		 * For output, the ptr and the len field of str indicates the
		 * pointer and the length of a deparsed source string. The output
		 * string is dynamically allocated. You don't need to set these
		 * fields before calling qse_awk_parsestd() because they are set
		 * by qse_awk_parsestd() and valid while the relevant awk object
		 * is alive. You must free the memory chunk pointed to by the
		 * ptr field with qse_awk_freemem() once you're done with it to 
		 * avoid memory leaks. 
		 */
		qse_cstr_t str;
	} u;
};

typedef struct qse_awk_parsestd_t qse_awk_parsestd_t;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_awk_openstd() function creates an awk object using the default 
 * memory manager and primitive functions. Besides, it adds a set of
 * standard intrinsic functions like atan, system, etc. Use this function
 * over qse_awk_open() if you don't need finer-grained customization.
 */
QSE_EXPORT qse_awk_t* qse_awk_openstd (
	qse_size_t        xtnsize,  /**< extension size in bytes */
	qse_awk_errnum_t* errnum    /**< pointer to an error number variable */
);

/**
 * The qse_awk_openstdwithmmgr() function creates an awk object with a 
 * user-defined memory manager. It is equivalent to qse_awk_openstd(), 
 * except that you can specify your own memory manager.
 */
QSE_EXPORT qse_awk_t* qse_awk_openstdwithmmgr (
	qse_mmgr_t*       mmgr,     /**< memory manager */
	qse_size_t        xtnsize,  /**< extension size in bytes */
	qse_awk_errnum_t* errnum    /**< pointer to an error number variable */
);

/**
 * The qse_awk_getxtnstd() gets the pointer to extension area created with 
 * qse_awk_openstd() or qse_awk_openstdwithmmgr(). You must not call 
 * qse_awk_getxtn() for sunch an object.
 */
QSE_EXPORT void* qse_awk_getxtnstd (
	qse_awk_t* awk
);

/**
 * The qse_awk_parsestd() functions parses source script.
 * The code below shows how to parse a literal string 'BEGIN { print 10; }' 
 * and deparses it out to a buffer 'buf'.
 * \code
 * int n;
 * qse_awk_parsestd_t in[2];
 * qse_awk_parsestd_t out;
 *
 * in[0].type = QSE_AWK_PARSESTD_STR;
 * in[0].u.str.ptr = QSE_T("BEGIN { print 10; }");
 * in[0].u.str.len = qse_strlen(in.u.str.ptr);
 * in[1].type = QSE_AWK_PARSESTD_NULL;
 * out.type = QSE_AWK_PARSESTD_STR;
 * n = qse_awk_parsestd (awk, in, &out);
 * if (n >= 0) 
 * {
 *   qse_printf (QSE_T("%s\n"), out.u.str.ptr);
 *   QSE_MMGR_FREE (out.u.str.ptr);
 * }
 * \endcode
 */
QSE_EXPORT int qse_awk_parsestd (
	qse_awk_t*          awk,
	qse_awk_parsestd_t  in[], 
	qse_awk_parsestd_t* out
);

/**
 * The qse_awk_rtx_openstd() function creates a standard runtime context.
 * The caller should keep the contents of \a icf and \a ocf valid throughout
 * the lifetime of the runtime context created. The \a cmgr is set to the
 * streams created with \a icf and \a ocf if it is not #QSE_NULL.
 */
QSE_EXPORT qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*        awk,
	qse_size_t        xtnsize,
	const qse_char_t* id,
	const qse_char_t* icf[],
	const qse_char_t* ocf[],
	qse_cmgr_t*       cmgr
);

/**
 * The qse_awk_rtx_getxtnstd() function gets the pointer to extension area
 * created with qse_awk_rtx_openstd().
 */
QSE_EXPORT void* qse_awk_rtx_getxtnstd (
	qse_awk_rtx_t* rtx
);


/**
 * The qse_awk_rtx_getcmgrstd() function gets the current character 
 * manager associated with a particular I/O target indicated by the name 
 * \a ioname if #QSE_CHAR_IS_WCHAR is defined. It always returns #QSE_NULL
 * if #QSE_CHAR_IS_MCHAR is defined.
 */
QSE_EXPORT qse_cmgr_t* qse_awk_rtx_getcmgrstd (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* ioname
);

#if defined(__cplusplus)
}
#endif


#endif
