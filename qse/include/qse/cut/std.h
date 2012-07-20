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

#ifndef _QSE_CUT_STD_H_
#define _QSE_CUT_STD_H_

#include <qse/cut/cut.h>

/** @file
 * This file provides easier-to-use versions of selected API functions
 * by implementing default handlers for I/O and memory management.
 *
 * @example cut01.c
 * This example shows how to write a simple text cutter using helper
 * functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_cut_openstd() function creates a text cutter with the default
 * memory manager and initialized it for other qse_cut_xxxxstd functions.
 * @return pointer to a text cutter on success, QSE_NULL on failure.
 */
qse_cut_t* qse_cut_openstd (
	qse_size_t xtnsize  /**< extension size in bytes */
);

/**
 * The qse_cut_openstdwithmmgr() function creates a text cutter with a 
 * user-defined memory manager. It is equivalent to qse_cut_openstd(), 
 * except that you can specify your own memory manager.
 * @return pointer to a text cutter on success, QSE_NULL on failure.
 */
qse_cut_t* qse_cut_openstdwithmmgr (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

/**
 * The qse_cut_getxtnstd() gets the pointer to extension space. 
 * Note that you must not call qse_cut_getxtn() for a text cutter
 * created with qse_cut_openstd().
 */
void* qse_cut_getxtnstd (
	qse_cut_t* cut
);

/**
 * The qse_cut_compstd() function compiles a null-terminated selector.
 * Call qse_cut_comp() for a length delimited selector.
 */
int qse_cut_compstd (
	qse_cut_t*        cut,
	const qse_char_t* str
);

/**
 * The qse_cut_execstd() function executes the compiled script
 * over an input file @a infile and an output file @a outfile.
 * If @a infile is QSE_NULL, the standard console input is used.
 * If @a outfile is QSE_NULL, the standard console output is used..
 */
int qse_cut_execstd (
	qse_cut_t*        cut,
	const qse_char_t* infile,
	const qse_char_t* outfile
);

#ifdef __cplusplus
}
#endif


#endif
