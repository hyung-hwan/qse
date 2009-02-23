/*
 * $Id: std.h 75 2009-02-22 14:10:34Z hyunghwan.chung $
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

#ifndef _QSE_AWK_STD_H_
#define _QSE_AWK_STD_H_

#include <qse/awk/awk.h>

/****e* AWK/qse_awk_parsestd_type_t
 * NAME
 *  qse_awk_parsestd_type_t - define a source type
 * SYNOPSIS
 */
enum qse_awk_parsestd_type_t
{
	QSE_AWK_PARSESTD_FILE  = 0, /* file name */
	QSE_AWK_PARSESTD_CP    = 1, /* character pointer */
	QSE_AWK_PARSESTD_CPL   = 2, /* character pointer + length */
	QSE_AWK_PARSESTD_STDIO = 3  /* standard input/output */
};
typedef enum qse_awk_parsestd_type_t qse_awk_parsestd_type_t;
/******/


/****s* AWK/qse_awk_parsestd_in_t
 * NAME
 *  qse_awk_parsestd_in_t - define source input 
 * SYNOPSIS
 */
struct qse_awk_parsestd_in_t
{
	qse_awk_parsestd_type_t type;

	union
	{
		const qse_char_t* file; 
		const qse_char_t* cp;
		qse_cstr_t        cpl;
	} u;
};
typedef struct qse_awk_parsestd_in_t qse_awk_parsestd_in_t;
/******/

/****s* AWK/qse_awk_parsestd_out_t
 * NAME
 *  qse_awk_parsestd_out_t - define source output
 * SYNOPSIS
 */
struct qse_awk_parsestd_out_t
{
	qse_awk_parsestd_type_t type;

	union
	{
		const qse_char_t* file; 
		qse_char_t*       cp;
		qse_xstr_t        cpl;
	} u;
};
typedef struct qse_awk_parsestd_out_t qse_awk_parsestd_out_t;
/******/

#define QSE_AWK_RTX_OPENSTD_STDIO (qse_awk_rtx_openstd_stdio)
extern const qse_char_t* qse_awk_rtx_openstd_stdio[];


#ifdef __cplusplus
extern "C" {
#endif

/****f* AWK/qse_awk_openstd
 * NAME
 *  qse_awk_openstd - create an awk object
 * SYNOPSIS
 */
qse_awk_t* qse_awk_openstd (
	void
);
/******/

/****f* AWK/qse_awk_parsestd
 * NAME
 *  qse_awk_parsestd - parse source code
 * EXAMPLE
 *  The following example parses the literal string 'BEGIN { print 10; }' and
 *  deparses it out to a buffer 'buf'.
 *    int n;
 *    qse_awk_parsestd_in_t in;
 *    qse_awk_parsestd_out_t out;
 *    qse_char_t buf[1000];
 *
 *    qse_memset (buf, QSE_T(' '), QSE_COUNTOF(buf));
 *    buf[QSE_COUNTOF(buf)-1] = QSE_T('\0');
 *    in.type = QSE_AWK_PARSESTD_CP;
 *    in.u.cp = QSE_T("BEGIN { print 10; }");
 *    out.type = QSE_AWK_PARSESTD_CP;
 *    out.u.cp = buf;
 *
 *    n = qse_awk_parsestd (awk, &in, &out);
 * SYNOPSIS
 */
int qse_awk_parsestd (
	qse_awk_t*                      awk,
	const qse_awk_parsestd_in_t* in,
	qse_awk_parsestd_out_t*      out
);
/******/

/****f* AWK/qse_awk_rtx_openstd
 * NAME
 *  qse_awk_rtx_openstd - create a runtime context
 * DESCRIPTION
 *  The caller should keep the contents of icf and ocf valid throughout
 *  the lifetime of the runtime context created. The runtime context 
 *  remembers the pointers without copying in the contents.
 *
 * SYNOPSIS
 */
qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*        awk,
	const qse_char_t*const* icf,
	const qse_char_t*const* ocf
);
/******/

#ifdef __cplusplus
}
#endif


#endif
