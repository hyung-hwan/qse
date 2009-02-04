/*
 * $Id: eio.h 363 2008-09-04 10:58:08Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_LIB_AWK_EIO_H_
#define _QSE_LIB_AWK_EIO_H_

#ifdef __cplusplus
extern "C" {
#endif

int qse_awk_readeio (
	qse_awk_rtx_t* run, int in_type, 
	const qse_char_t* name, qse_str_t* buf);

int qse_awk_writeeio_val (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_awk_val_t* v);

int qse_awk_writeeio_str (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_char_t* str, qse_size_t len);

int qse_awk_flusheio (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_nexteio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name);

int qse_awk_nexteio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);

int qse_awk_closeeio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name);
int qse_awk_closeeio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name);
int qse_awk_closeeio (qse_awk_rtx_t* run, const qse_char_t* name);

void qse_awk_cleareio (qse_awk_rtx_t* run);

#ifdef __cplusplus
}
#endif

#endif
