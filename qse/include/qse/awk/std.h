/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_AWK_STD_H_
#define _QSE_AWK_STD_H_

#include <qse/awk/awk.h>
#include <qse/cmn/sio.h>

/** @file
 * This file defines functions and data types that help you create
 * an awk interpreter with less effort. It is designed to be as close
 * to conventional awk implementations as possible.
 * 
 * The source script handler does not evaluate a file name of the "var=val"
 * form as an assignment expression. Instead, it just treats it as a
 * normal file name.
 */

/**
 * @example awk09.c
 * This programs shows how to specify multiple console output files.
 *
 * @example awk11.c
 * This programs shows how to extend an I/O handler implemented by 
 * qse_awk_rtx_openstd().
 *
 */

/**
 * The qse_awk_parsestd_type_t type defines the types of source I/O.
 */
enum qse_awk_parsestd_type_t
{
	QSE_AWK_PARSESTD_NULL = 0, /**< invalid type */
	QSE_AWK_PARSESTD_FILE = 1, /**< file */
	QSE_AWK_PARSESTD_STR  = 2  /**< length-bounded string */
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
		/**
		 * You can create a sio stream in advance and pass it to 
		 * qse_awk_parsestd() via this field. */
		qse_sio_t* sio;

		struct
		{
			/** file path to open. QSE_NULL or '-' for stdin/stdout. */
			const qse_char_t* path; 
			/* the streams created with the file path is set with this
			 * cmgr if it is not #QSE_NULL. */
			qse_cmgr_t*       cmgr; 
		} file;

		/** 
		 * For input, the ptr and the len field of str indicates the 
		 * pointer and the length of a string to read.
		 *
		 * For output, the ptr and the len field of str indicates the
		 * pointer and the length of a deparsed source string. The output
		 * string is dynamically allocated. You must free this output 
		 * pointer using #QSE_MMGR_FREE once you're done with it to avoid 
		 * memory leaks. 
		 */
		qse_xstr_t str;
	} u;
};

typedef struct qse_awk_parsestd_t qse_awk_parsestd_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_awk_openstd() function creates an awk object using the default 
 * memory manager and primitive functions. Besides, it adds a set of
 * standard intrinsic functions like atan, system, etc. Use this function
 * over qse_awk_open() if you don't need finer-grained customization.
 */
qse_awk_t* qse_awk_openstd (
	qse_size_t xtnsize  /**< extension size in bytes */
);

/**
 * The qse_awk_openstdwithmmgr() function creates an awk object with a 
 * user-defined memory manager. It is equivalent to qse_awk_openstd(), 
 * except that you can specify your own memory manager.
 */
qse_awk_t* qse_awk_openstdwithmmgr (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

/**
 * The qse_awk_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_awk_getxtn() for an awk object
 * created with qse_awk_openstd() and qse_awk_openstdwithmmgr().
 */
void* qse_awk_getxtnstd (
	qse_awk_t* awk
);

/**
 * The qse_awk_parsestd() functions parses source script.
 * The code below shows how to parse a literal string 'BEGIN { print 10; }' 
 * and deparses it out to a buffer 'buf'.
 * @code
 * int n;
 * qse_awk_parsestd_t in;
 * qse_awk_parsestd_t out;
 *
 * in.type = QSE_AWK_PARSESTD_STR;
 * in.u.str.ptr = QSE_T("BEGIN { print 10; }");
 * in.u.str.len = qse_strlen(in.u.str.ptr);
 * out.type = QSE_AWK_PARSESTD_STR;
 * n = qse_awk_parsestd (awk, &in, &out);
 * if (n >= 0) 
 * {
 *   qse_printf (QSE_T("%s\n"), out.u.str.ptr);
 *   QSE_MMGR_FREE (out.u.str.ptr);
 * }
 * @endcode
 */
int qse_awk_parsestd (
	qse_awk_t*          awk,
	qse_awk_parsestd_t* in,
	qse_awk_parsestd_t* out
);

/**
 * The qse_awk_rtx_openstd() function creates a standard runtime context.
 * The caller should keep the contents of @a icf and @a ocf valid throughout
 * the lifetime of the runtime context created. The @a cmgr is set to the
 * streams created with @a icf and @a ocf if it is not #QSE_NULL.
 */
qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*        awk,
	qse_size_t        xtnsize,
	const qse_char_t* id,
	const qse_char_t* icf[],
	const qse_char_t* ocf[],
	qse_cmgr_t*       cmgr
);

/**
 * The qse_awk_rtx_getxtnstd() function gets the pointer to extension space.
 */
void* qse_awk_rtx_getxtnstd (
	qse_awk_rtx_t* rtx
);


/**
 * The qse_awk_rtx_getcmgrstd() function gets the current character 
 * manager associated with a particular I/O target indicated by the name 
 * @a ioname if #QSE_CHAR_IS_WCHAR is defined. It always returns #QSE_NULL
 * if #QSE_CHAR_IS_MCHAR is defined.
 */
qse_cmgr_t* qse_awk_rtx_getcmgrstd (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* ioname
);

#ifdef __cplusplus
}
#endif


#endif
