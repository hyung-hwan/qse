/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

/** @file
 * This file provides easier-to-use versions of selected API functions
 * by implementing default handlers for I/O and memory management.
 *
 * @example sed01.c
 * This example shows how to write a simple stream editor using easy API 
 * functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sed_openstd() function creates a stream editor.
 */
qse_sed_t* qse_sed_openstd (
	qse_size_t xtnsize /**< size of extension in bytes */
);

qse_sed_t* qse_sed_openstdwithmmgr (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize /**< size of extension in bytes */
);

/**
 * The qse_sed_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_sed_getxtn() for a stream editor
 * created with qse_sed_openstd().
 */
void* qse_sed_getxtnstd (
	qse_sed_t* sed
);

/**
 * The qse_sed_compstd() function compiles a null-terminated sed script.
 * Call qse_sed_comp() for a length delimited script.
 */
int qse_sed_compstd (
	qse_sed_t*        sed,
	const qse_char_t* str
);

/**
 * The qse_sed_execstd() function executes the compiled script
 * over an input file @a infile and an output file @a outfile.
 * If @a infile is QSE_NULL, the standard console input is used.
 * If @a outfile is QSE_NULL, the standard console output is used.
 */
int qse_sed_execstd (
	qse_sed_t*        sed,
	const qse_char_t* infile,
	const qse_char_t* outfile
);

#ifdef __cplusplus
}
#endif


#endif
