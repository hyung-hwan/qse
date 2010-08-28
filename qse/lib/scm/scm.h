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

#ifndef _QSE_LIB_SCM_SCM_H_
#define _QSE_LIB_SCM_SCM_H_

#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/scm/scm.h>

#include "mem.h"

#define QSE_SCM_ALLOC(scm,size)       QSE_MMGR_ALLOC((scm)->mmgr,size)
#define QSE_SCM_REALLOC(scm,ptr,size) QSE_MMGR_REALLOC((scm)->mmgr,ptr,size)
#define QSE_SCM_FREE(scm,ptr)         QSE_MMGR_FREE((scm)->mmgr,ptr)

#define QSE_SCM_ISUPPER(scm,c)  QSE_ISUPPER(c)
#define QSE_SCM_ISLOWER(scm,c)  QSE_ISLOWER(c)
#define QSE_SCM_ISALPHA(scm,c)  QSE_ISALPHA(c)
#define QSE_SCM_ISDIGIT(scm,c)  QSE_ISDIGIT(c)
#define QSE_SCM_ISXDIGIT(scm,c) QSE_ISXDIGIT(c)
#define QSE_SCM_ISALNUM(scm,c)  QSE_ISALNUM(c)
#define QSE_SCM_ISSPACE(scm,c)  QSE_ISSPACE(c)
#define QSE_SCM_ISPRINT(scm,c)  QSE_ISPRINT(c)
#define QSE_SCM_ISGRAPH(scm,c)  QSE_ISGRAPH(c)
#define QSE_SCM_ISCNTRL(scm,c)  QSE_ISCNTRL(c)
#define QSE_SCM_ISPUNCT(scm,c)  QSE_ISPUNCT(c)
#define QSE_SCM_TOUPPER(scm,c)  QSE_TOUPPER(c)
#define QSE_SCM_TOLOWER(scm,c)  QSE_TOLOWER(c)

struct qse_scm_t 
{
	QSE_DEFINE_COMMON_FIELDS (scm)

	qse_scm_prm_t prm;
	int opt_undef_symbol;

	/** error information */
	struct 
	{
		qse_scm_errstr_t str;      /**< error string getter */
		qse_scm_errnum_t num;      /**< stores an error number */
		qse_char_t       msg[128]; /**< error message holder */
		qse_scm_loc_t    loc;      /**< location of the last error */
	} err; 

	/** memory manager */
	qse_scm_mem_t mem;

	/** I/O functions */
	struct
	{
		qse_scm_io_t fns;

		struct
		{
			qse_scm_io_arg_t in;
			qse_scm_io_arg_t out;
		} arg;
	} io;

	/** data for reading */
	struct
	{
		qse_cint_t curc; 
		qse_scm_loc_t curloc;

		/** token */
		struct
		{
			int           type;
			qse_scm_loc_t loc;
			qse_long_t    ival;
			qse_real_t    rval;
			qse_str_t     name;
		} t;
	} r;

	/** data for evaluation */
	/*
	struct
	{
	} e;
	*/
};

#ifdef __cplusplus
extern "C" {
#endif

/* err.c */
const qse_char_t* qse_scm_dflerrstr (qse_scm_t* scm, qse_scm_errnum_t errnum);

#ifdef __cplusplus
}
#endif
#endif
