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

#ifndef _QSE_XLI_STDXLI_H_
#define _QSE_XLI_STDXLI_H_

#include <qse/xli/xli.h>
#include <qse/cmn/sio.h>

/** \file
 * This file provides easier interface to a qse_xli_t object.
 */

/**
 * The qse_xli_iostd_type_t type defines standard source I/O types.
 */
enum qse_xli_iostd_type_t
{
	QSE_XLI_IOSTD_NULL  = 0, /**< pseudo-value to indicate no script */
	QSE_XLI_IOSTD_FILE  = 1, /**< file */
	QSE_XLI_IOSTD_STR   = 2  /**< length-bounded string */
};
typedef enum qse_xli_iostd_type_t qse_xli_iostd_type_t;

/**
 * The qse_xli_iostd_t type defines a source I/O.
 */
struct qse_xli_iostd_t
{
	qse_xli_iostd_type_t type;

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
		 * these fields before calling qse_xli_readstd().
		 *
		 * For output, the ptr and the len field of str indicates the
		 * pointer and the length of a dereadd source string. The output
		 * string is dynamically allocated. You don't need to set these
		 * fields before calling qse_xli_readstd() because they are set
		 * by qse_xli_readstd() and valid while the relevant xli object
		 * is alive. You must free the memory chunk pointed to by the
		 * ptr field with qse_xli_freemem() once you're done with it to 
		 * avoid memory leaks. 
		 */
		qse_cstr_t str;
	} u;
};

typedef struct qse_xli_iostd_t qse_xli_iostd_t;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_xli_openstd() function creates an xli object using the default 
 * memory manager and primitive functions. Besides, it adds a set of
 * standard intrinsic functions like atan, system, etc. Use this function
 * over qse_xli_open() if you don't need finer-grained customization.
 */
QSE_EXPORT qse_xli_t* qse_xli_openstd (
	qse_size_t xtnsize,  /**< extension size in bytes */
	qse_size_t rootxtnsize  /**< extension size in bytes for the root list node */
);

/**
 * The qse_xli_openstdwithmmgr() function creates an xli object with a 
 * user-defined memory manager. It is equivalent to qse_xli_openstd(), 
 * except that you can specify your own memory manager.
 */
QSE_EXPORT qse_xli_t* qse_xli_openstdwithmmgr (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize, /**< extension size in bytes */
	qse_size_t rootxtnsize  /**< extension size in bytes for the root list node */
);

/**
 * The qse_xli_getxtnstd() gets the pointer to extension area created with 
 * qse_xli_openstd() or qse_xli_openstdwithmmgr(). You must not call 
 * qse_xli_getxtn() for sunch an object.
 */
QSE_EXPORT void* qse_xli_getxtnstd (
	qse_xli_t* xli
);

QSE_EXPORT int qse_xli_readstd (
	qse_xli_t*       xli,
	qse_xli_iostd_t* in
);

QSE_EXPORT int qse_xli_writestd (
	qse_xli_t*       xli,
	qse_xli_iostd_t* out
);

#if defined(__cplusplus)
}
#endif


#endif
