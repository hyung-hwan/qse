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

#ifndef _QSE_LIB_AWK_RIO_H_
#define _QSE_LIB_AWK_RIO_H_

#if defined(__cplusplus)
extern "C" {
#endif

int qse_awk_rtx_readio (
	qse_awk_rtx_t* run, int in_type, 
	const qse_char_t* name, qse_str_t* buf);

int qse_awk_rtx_writeio_val (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_awk_val_t* v);

int qse_awk_rtx_writeio_str (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_char_t* str, qse_size_t len);

int qse_awk_rtx_flushio (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_rtx_nextio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name);

int qse_awk_rtx_nextio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_rtx_closeio (
	qse_awk_rtx_t*    run,
	const qse_char_t* name,
	const qse_char_t* opt
);

void qse_awk_rtx_cleario (qse_awk_rtx_t* run);

#if defined(__cplusplus)
}
#endif

#endif
