/*
 * $Id: tgp.h 235 2008-07-05 07:25:54Z baconevi $
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

#ifndef _QSE_UTL_TGP_H_
#define _QSE_UTL_TGP_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_tgp_t qse_tgp_t;

enum qse_tgp_iocmd_t
{
	QSE_TGP_IO_OPEN = 0,
	QSE_TGP_IO_CLOSE = 1,
	QSE_TGP_IO_READ = 2,
	QSE_TGP_IO_WRITE = 3
};

typedef qse_ssize_t (*qse_tgp_io_t) (
	int cmd, void* arg, qse_char_t* data, qse_size_t count);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (tgp)

qse_tgp_t* qse_tgp_open (
	qse_mmgr_t* mmgr,
	qse_size_t xtn
);

void qse_tgp_close (
	qse_tgp_t* tgp
);

qse_tgp_t* qse_tgp_init (
	qse_tgp_t* tgp,
	qse_mmgr_t* mmgr
);

void qse_tgp_fini (
	qse_tgp_t* tgp
);

#ifdef __cplusplus
}
#endif

#endif
