/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#ifndef _QSE_SED_STD_H_
#define _QSE_SED_STD_H_

#include <qse/sed/sed.h>
#include <qse/cmn/sio.h>

/** @file
 * This file defines easier-to-use helper interface for a stream editor.
 * If you don't care about the details of memory management and I/O handling,
 * you can choose to use the helper functions provided here. It is  
 * a higher-level interface that is easier to use as it implements 
 * default handlers for I/O and memory management.
 *
 * @example sed01.c
 * This example shows how to write a simple stream editor using easy API 
 * functions.
 */

/**
 * The qse_sed_iostd_t type defines standard I/O resources.
 */
typedef struct qse_sed_iostd_t qse_sed_iostd_t;

struct qse_sed_iostd_t
{
	enum
	{
		QSE_SED_IOSTD_NULL, /** invalid resource */
		QSE_SED_IOSTD_SIO,
		QSE_SED_IOSTD_FILE,
		QSE_SED_IOSTD_MEM
	} type;

	union
	{
		qse_sio_t*        sio;
		const qse_char_t* file;
		qse_xstr_t        mem;
	} u;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sed_openstd() function creates a stream editor with the default
 * memory manager and initializes it. Use qse_getxtnstd(), not qse_getxtn() 
 * to get the pointer to the extension area.
 * @return pointer to a stream editor on success, #QSE_NULL on failure.
 */
qse_sed_t* qse_sed_openstd (
	qse_size_t xtnsize  /**< extension size in bytes */
);

/**
 * The qse_sed_openstdwithmmgr() function creates a stream editor with a 
 * user-defined memory manager. It is equivalent to qse_sed_openstd(), 
 * except that you can specify your own memory manager.Use qse_getxtnstd(), 
 * not qse_getxtn() to get the pointer to the extension area.
 * @return pointer to a stream editor on success, #QSE_NULL on failure.
 */
qse_sed_t* qse_sed_openstdwithmmgr (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

/**
 * The qse_sed_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_sed_getxtn() for a stream editor
 * created with qse_sed_openstd() and qse_sed_openstdwithmmgr().
 */
void* qse_sed_getxtnstd (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_compstd() function compiles a null-terminated sed script.
 * Call qse_sed_comp() for a length delimited script.
 * @return 0 on success, -1 on failure
 */
int qse_sed_compstd (
	qse_sed_t*        sed, /**< stream editor */
	const qse_char_t* str  /**< null-terminated script */
);

/**
 * The qse_sed_execstd() function executes a compiled script
 * over input streams @a in and an output stream @a out.
 *
 * If @a in is not #QSE_NULL, it must point to a null-terminated array
 * of standard I/O resources. if in[0].type is QSE_SED_IOSTD_NULL, this 
 * function returns failure, requiring at least 1 valid resource to be 
 * included in the array.
 *
 * If @a in is #QSE_NULL, the standard console input is used.
 * If @a out is #QSE_NULL, the standard console output is used.
 *
 * @return 0 on success, -1 on failure
 */
int qse_sed_execstd (
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
int qse_sed_execstdfile (
     qse_sed_t*        sed,
	const qse_char_t* infile,
	const qse_char_t* outfile
);

#ifdef __cplusplus
}
#endif

#endif
