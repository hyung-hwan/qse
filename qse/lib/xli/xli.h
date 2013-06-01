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

#ifndef _QSE_LIB_XLI_XLI_H_
#define _QSE_LIB_XLI_XLI_H_

#include <qse/xli/xli.h>
#include <qse/cmn/str.h>
#include "../cmn/mem.h"

typedef struct qse_xli_tok_t qse_xli_tok_t;

struct qse_xli_tok_t
{
     int           type;
     qse_str_t*    name;
     qse_xli_loc_t loc;
};

struct qse_xli_t
{
	qse_mmgr_t* mmgr;

	qse_xli_errstr_t errstr; /**< error string getter */
	qse_xli_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */
	qse_xli_loc_t errloc;    /**< location of the last error */

	struct
	{
		int  trait;          
	} opt;

	qse_xli_ecb_t* ecb;

	qse_xli_list_t root;
	qse_xli_nil_t xnil;

	qse_str_t* tmp[1];
	qse_xli_tok_t tok;
	struct
	{
		qse_xli_io_impl_t impl; /* input handler */
		qse_xli_io_lxc_t  last;	
		qse_xli_io_arg_t  arg; /* for top level */
		qse_xli_io_arg_t* inp; /* current */
	} sio;
	qse_link_t* sio_names;

	struct 
	{
		qse_xli_io_impl_t impl; /* output handler */
		qse_xli_io_arg_t  arg; /* for top level */
		qse_xli_io_arg_t* inp; /* current */
	} wio;
};


#if defined(__cplusplus)
extern "C" {
#endif

int qse_xli_init (qse_xli_t* xli, qse_mmgr_t* mmgr);

void qse_xli_fini (qse_xli_t* xli);

const qse_char_t* qse_xli_dflerrstr (
     const qse_xli_t* xli, qse_xli_errnum_t errnum);

void qse_xli_clearsionames (qse_xli_t* xli);

#if defined(__cplusplus)
}
#endif



#endif
