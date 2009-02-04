/*
 * $Id$
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

#ifndef _QSE_UTL_SED_H_
#define _QSE_UTL_SED_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_sed_t qse_sed_t;

struct qse_sed_t
{
	QSE_DEFINE_COMMON_FIELDS (sed)
};


#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (sed)

/****f* Text Processor/qse_sed_open
 * NAME
 *  qse_sed_open - create a stream editor
 * SYNOPSIS
 */
qse_sed_t* qse_sed_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtn
);
/******/

/****f* Text Processor/qse_sed_close
 * NAME
 *  qse_sed_close - destroy a stream editor
 * SYNOPSIS
 */
void qse_sed_close (
	qse_sed_t* sed
);
/******/


int qse_sed_execute (
	qse_sed_t* sed
);

#ifdef __cplusplus
}
#endif

#endif
