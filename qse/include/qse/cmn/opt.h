/*
 * $Id: getopt.h 290 2008-07-27 06:16:54Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

#ifndef _QSE_CMN_OPT_H_
#define _QSE_CMN_OPT_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_opt_t qse_opt_t;
typedef struct qse_opt_lng_t qse_opt_lng_t;

struct qse_opt_lng_t
{
	const qse_char_t* str;
	qse_cint_t val;
};

struct qse_opt_t
{
	/* input */
	const qse_char_t* str; /* option string  */
	qse_opt_lng_t* lng;    /* long options */

	/* output */
	qse_cint_t opt;  /* character checked for validity */
	qse_char_t* arg; /* argument associated with an option */

	/* output */
	const qse_char_t* lngopt; 

	/* input + output */
	int ind;         /* index into parent argv vector */

	/* input + output - internal*/
	qse_char_t* cur;
};

#ifdef __cplusplus
extern "C" {
#endif

/****f* qse.cmn.opt/qse_getopt
 * NAME
 *  qse_getopt - parse command line options
 *
 * SYNOPSIS
 */
qse_cint_t qse_getopt (
	int                argc  /* argument count */, 
	qse_char_t* const* argv  /* argument array */,
	qse_opt_t*         opt   /* option configuration */
);
/******/

#ifdef __cplusplus
}
#endif

#endif
