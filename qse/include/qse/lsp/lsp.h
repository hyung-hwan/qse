/*
 * $Id: lsp.h 183 2008-06-03 08:18:55Z baconevi $
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

#ifndef _QSE_LSP_LSP_H_
#define _QSE_LSP_LSP_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_lsp_t qse_lsp_t;
typedef struct qse_lsp_obj_t qse_lsp_obj_t;
typedef struct qse_lsp_prmfns_t qse_lsp_prmfns_t;

typedef qse_ssize_t (*qse_lsp_io_t) (
	int cmd, void* arg, qse_char_t* data, qse_size_t count);

typedef qse_real_t (*qse_lsp_pow_t) (
	void* data, qse_real_t x, qse_real_t y);
typedef int (*qse_lsp_sprintf_t) (
	void* data, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...);
typedef void (*qse_lsp_dprintf_t) (void* data, const qse_char_t* fmt, ...); 

struct qse_lsp_prmfns_t
{
	qse_mmgr_t mmgr;
	qse_ccls_t ccls;

	/* utilities */
	struct
	{
		qse_lsp_sprintf_t sprintf;
		qse_lsp_dprintf_t dprintf;
		void* data;
	} misc;
};

/* io function commands */
enum 
{
	QSE_LSP_IO_OPEN   = 0,
	QSE_LSP_IO_CLOSE  = 1,
	QSE_LSP_IO_READ   = 2,
	QSE_LSP_IO_WRITE  = 3
};

/* option code */
enum
{
	QSE_LSP_UNDEFSYMBOL = (1 << 0)
};

/* error code */
enum 
{
	QSE_LSP_ENOERR,
	QSE_LSP_ENOMEM,

	QSE_LSP_EEXIT,
	QSE_LSP_EEND,
	QSE_LSP_EENDSTR,
	QSE_LSP_ENOINP,
	QSE_LSP_EINPUT,
	QSE_LSP_ENOOUTP,
	QSE_LSP_EOUTPUT,

	QSE_LSP_ESYNTAX,
	QSE_LSP_ERPAREN,
	QSE_LSP_EARGBAD,
	QSE_LSP_EARGFEW,
	QSE_LSP_EARGMANY,
	QSE_LSP_EUNDEFFN,
	QSE_LSP_EBADFN,
	QSE_LSP_EDUPFML,
	QSE_LSP_EBADSYM,
	QSE_LSP_EUNDEFSYM,
	QSE_LSP_EEMPBDY,
	QSE_LSP_EVALBAD,
	QSE_LSP_EDIVBY0
};

typedef qse_lsp_obj_t* (*qse_lsp_prim_t) (qse_lsp_t* lsp, qse_lsp_obj_t* obj);

#ifdef __cplusplus
extern "C" {
#endif

qse_lsp_t* qse_lsp_open (
	const qse_lsp_prmfns_t* prmfns,
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc);

void qse_lsp_close (qse_lsp_t* lsp);

/**
 *  @function qse_lsp_setassocdata
 *  @brief ssociats the user-specified data with an interpreter
 */
void qse_lsp_setassocdata (qse_lsp_t* lsp, void* data);
/**
 *  @function qse_lsp_getassocdata
 *  @brief returns the user-specified data associated with an interpreter
 */
void* qse_lsp_getassocdata (qse_lsp_t* lsp);

void qse_lsp_geterror (
	qse_lsp_t* lsp, int* errnum, const qse_char_t** errmsg);

void qse_lsp_seterror (
	qse_lsp_t* lsp, int errnum, 
	const qse_char_t** errarg, qse_size_t argcnt);

int qse_lsp_attinput (qse_lsp_t* lsp, qse_lsp_io_t input, void* arg);
int qse_lsp_detinput (qse_lsp_t* lsp);

int qse_lsp_attoutput (qse_lsp_t* lsp, qse_lsp_io_t output, void* arg);
int qse_lsp_detoutput (qse_lsp_t* lsp);

qse_lsp_obj_t* qse_lsp_read (qse_lsp_t* lsp);
qse_lsp_obj_t* qse_lsp_eval (qse_lsp_t* lsp, qse_lsp_obj_t* obj);
int qse_lsp_print (qse_lsp_t* lsp, const qse_lsp_obj_t* obj);

int qse_lsp_addprim (
	qse_lsp_t* lsp, const qse_char_t* name, qse_size_t name_len, 
	qse_lsp_prim_t prim, qse_size_t min_args, qse_size_t max_args);
int qse_lsp_removeprim (qse_lsp_t* lsp, const qse_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
