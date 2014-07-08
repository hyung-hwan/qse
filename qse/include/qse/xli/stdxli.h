/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_XLI_STDXLI_H_
#define _QSE_XLI_STDXLI_H_

#include <qse/xli/xli.h>
#include <qse/cmn/sio.h>

/** \file
 * This file defines functions and data types that help you create
 * an xli interpreter with less effort. It is designed to be as close
 * to conventional xli implementations as possible.
 * 
 * The source script handler does not evaluate a file name of the "var=val"
 * form as an assignment expression. Instead, it just treats it as a
 * normal file name.
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

#ifdef __cplusplus
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

#ifdef __cplusplus
}
#endif


#endif
