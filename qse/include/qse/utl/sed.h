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
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/map.h>

enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /* no error */
	QSE_SED_ENOMEM,  /* no memory */
	QSE_SED_ETMTXT,  /* too much text */
	QSE_SED_ECMDNR,  /* command not recognized */
	QSE_SED_ECMDGB,  /* command garbled */
	QSE_SED_EREXBL,  /* regular expression build error */
	QSE_SED_EA1PHB,  /* address 1 prohibited */
	QSE_SED_EA2PHB,  /* address 2 prohibited */
	QSE_SED_ENEWLN,  /* a new line is expected */
	QSE_SED_EBSEXP,  /* \ is expected */
	QSE_SED_ELABTL,  /* label too long */
	QSE_SED_ELABEM,  /* label name is empty */
	QSE_SED_ELABDU   /* duplicate label name */
};

enum qse_sed_option_t
{
	QSE_SED_STRIPLS  = (1 << 0),  /* strip leading spaces from text*/
	QSE_SED_KEEPTBS  = (1 << 1),  /* keep an trailing backslash */
	QSE_SED_ENSURENL = (1 << 2)   /* ensure NL at the text end */
};

typedef struct qse_sed_t qse_sed_t;
typedef struct qse_sed_c_t qse_sed_c_t; /* command */
typedef enum qse_sed_errnum_t qse_sed_errnum_t;

struct qse_sed_t
{
	QSE_DEFINE_COMMON_FIELDS (sed)
	qse_sed_errnum_t errnum;
	int option;

	/* source code pointers */
	struct
	{
		const qse_char_t* ptr;
		const qse_char_t* end;
		const qse_char_t* cur;
	} src;

	void* lastrex;
	qse_str_t rexbuf; /* temporary regular expression buffer */

	/* command array */
	/*qse_lda_t cmds;*/
	struct
	{
		qse_sed_c_t* buf;
		qse_sed_c_t* end;
		qse_sed_c_t* cur;
	} cmd;

	qse_map_t labs; /* label map */
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

/****f* Text Processor/qse_sed_init
 * NAME
 *  qse_sed_init - initialize a stream editor
 * SYNOPSIS
 */
qse_sed_t* qse_sed_init (
	qse_sed_t*  sed,
	qse_mmgr_t* mmgr
);
/******/

/****f* Text Processor/qse_sed_fini
 * NAME
 *  qse_sed_fini - finalize a stream editor
 * SYNOPSIS
 */
void qse_sed_fini (
	qse_sed_t* sed
);
/******/

/****f* Text Processor/qse_sed_getoption
 * NAME
 *  qse_sed_getoption - get option
 * SYNOPSIS
 */
int qse_sed_getoption (
	qse_sed_t* sed
);
/******/

/****f* Text Processor/qse_sed_setoption
 * NAME
 *  qse_sed_setoption - set option
 * SYNOPSIS
 */
void qse_sed_setoption (
	qse_sed_t* sed,
	int        option
);
/*****/


int qse_sed_compile (
	qse_sed_t*        sed,
	const qse_char_t* sptr,
	qse_size_t        slen
);

#ifdef __cplusplus
}
#endif

#endif
