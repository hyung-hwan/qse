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

#ifndef _QSE_SED_STDSED_H_
#define _QSE_SED_STDSED_H_

#include <qse/sed/sed.h>
#include <qse/cmn/sio.h>

/** @file
 * This file defines easier-to-use helper interface for a stream editor.
 * If you don't care about the details of memory management and I/O handling,
 * you can choose to use the helper functions provided here. It is  
 * a higher-level interface that is easier to use as it implements 
 * default handlers for I/O and memory management.
 */

/**
 * The qse_sed_iostd_type_t type defines types of standard
 * I/O resources.
 */
enum qse_sed_iostd_type_t
{
	QSE_SED_IOSTD_NULL, /**< null resource type */
	QSE_SED_IOSTD_FILE, /**< file */
	QSE_SED_IOSTD_STR,  /**< string  */
	QSE_SED_IOSTD_SIO   /**< sio */
};
typedef enum qse_sed_iostd_type_t qse_sed_iostd_type_t;

/**
 * The qse_sed_iostd_t type defines a standard I/O resource.
 */
struct qse_sed_iostd_t
{
	/** resource type */
	qse_sed_iostd_type_t type;

	/** union describing the resource of the specified type */
	union
	{
		/** file path with character encoding */
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
		 * these two fields before calling qse_sed_execstd().
		 *
		 * For output, the ptr and the len field of str indicates the
		 * pointer and the length of produced output. The output
		 * string is dynamically allocated. You don't need to set these
		 * fields before calling qse_sed_execstd() because they are
		 * set by qse_sed_execstd() and valid while the relevant sed
		 * object is alive. You must free the memory chunk pointed to by
		 * the ptr field with qse_sed_freemem() once you're done with it
	      * to avoid memory leaks. 
           */
		qse_xstr_t        str;

		/** pre-opened sio stream */
		qse_sio_t*        sio;
	} u;
};

typedef struct qse_sed_iostd_t qse_sed_iostd_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sed_openstd() function creates a stream editor with the default
 * memory manager and initializes it. Use qse_sed_getxtnstd() to get the 
 * pointer to the extension area. Do not use qse_sed_getxtn() for this.
 * @return pointer to a stream editor on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_sed_t* qse_sed_openstd (
	qse_size_t xtnsize  /**< extension size in bytes */
);

/**
 * The qse_sed_openstdwithmmgr() function creates a stream editor with a 
 * user-defined memory manager. It is equivalent to qse_sed_openstd(), 
 * except that you can specify your own memory manager.Use qse_sed_getxtnstd(), 
 * not qse_sed_getxtn() to get the pointer to the extension area.
 * @return pointer to a stream editor on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_sed_t* qse_sed_openstdwithmmgr (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

/**
 * The qse_sed_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_sed_getxtn() for a stream editor
 * created with qse_sed_openstd() and qse_sed_openstdwithmmgr().
 */
QSE_EXPORT void* qse_sed_getxtnstd (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_compstd() function compiles sed scripts specified in
 * an array of stream resources. The end of the array is indicated
 * by an element whose type is #QSE_SED_IOSTD_NULL. However, the type
 * of the first element shall not be #QSE_SED_IOSTD_NULL. The output 
 * parameter @a count is set to the count of stream resources 
 * opened on both success and failure. You can pass #QSE_NULL to @a
 * count if the count is not needed.
 *
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_compstd (
	qse_sed_t*        sed,  /**< stream editor */
	qse_sed_iostd_t   in[], /**< input scripts */
	qse_size_t*       count /**< number of input scripts opened */
);

/**
 * The qse_sed_compstdfile() function compiles a sed script from
 * a single file @a infile.  If @a infile is #QSE_NULL, it reads
 * the script from the standard input.
 * When #QSE_CHAR_IS_WCHAR is defined, it converts the multibyte 
 * sequences in the file @a infile to wide characters via the 
 * #qse_cmgr_t interface @a cmgr. If @a cmgr is #QSE_NULL, it uses 
 * the default interface. It calls cmgr->mbtowc() for conversion.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_compstdfile (
	qse_sed_t*        sed, 
	const qse_char_t* infile,
	qse_cmgr_t*       cmgr
);

/**
 * The qse_sed_compstd() function compiles a sed script stored
 * in a null-terminated string pointed to by @a script.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_compstdstr (
	qse_sed_t*        sed, 
	const qse_char_t* script
);

/**
 * The qse_sed_execstd() function executes a compiled script
 * over input streams @a in and an output stream @a out.
 *
 * If @a in is not #QSE_NULL, it must point to an array of stream 
 * resources whose end is indicated by an element with #QSE_SED_IOSTD_NULL
 * type. However, the type of the first element @ in[0].type show not
 * be #QSE_SED_IOSTD_NULL. It requires at least 1 valid resource to be 
 * included in the array.
 *
 * If @a in is #QSE_NULL, the standard console input is used.
 * If @a out is #QSE_NULL, the standard console output is used.
 *
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_execstd (
	qse_sed_t*       sed,
	qse_sed_iostd_t  in[],
	qse_sed_iostd_t* out
);

/**
 * The qse_sed_execstdfile() function executes a compiled script
 * a single input file @a infile and a single output file @a outfile.
 *
 * If @a infile is #QSE_NULL, the standard console input is used.
 * If @a outfile is #QSE_NULL, the standard console output is used.
 *
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_execstdfile (
     qse_sed_t*        sed,
	const qse_char_t* infile,
	const qse_char_t* outfile,
	qse_cmgr_t*       cmgr
);

#ifdef __cplusplus
}
#endif

#endif
