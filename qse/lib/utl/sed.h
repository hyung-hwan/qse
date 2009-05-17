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

#ifndef _QSE_LIB_UTL_SED_H_
#define _QSE_LIB_UTL_SED_H_

#include <qse/utl/sed.h>
#include <qse/cmn/str.h>

typedef qse_int_t qse_sed_line_t;

typedef struct qse_sed_a_t qse_sed_a_t; /* address */
typedef struct qse_sed_l_t qse_sed_l_t; /* label */

struct qse_sed_a_t
{
	enum
	{
		QSE_SED_A_NONE, /* no address */
		QSE_SED_A_DOL,  /* $ - last line */
		QSE_SED_A_LINE, /* specified line */
		QSE_SED_A_REX   /* lines matching regular expression */
	} type;

	union 
	{
		qse_sed_line_t line;
		void*  rex;
	} u;
};

struct qse_sed_cmd_t
{
	enum
	{
		QSE_SED_CMD_QUIT       = QSE_T('q'),
		QSE_SED_CMD_QUIT_QUIET = QSE_T('Q'),

		QSE_SED_CMD_APPEND = QSE_T('a'),
		QSE_SED_CMD_INSERT = QSE_T('i'),
		QSE_SED_CMD_CHANGE = QSE_T('c'),

		QSE_SED_CMD_DELETE = QSE_T('d'),
		QSE_SED_CMD_DD     = QSE_T('D'),

		QSE_SED_CMD_PRINT_LNNUM    = QSE_T('='),
		QSE_SED_CMD_PRINT          = QSE_T('p'),
		QSE_SED_CMD_PRINT_FIRSTLN  = QSE_T('P'),
		QSE_SED_CMD_PRINT_CLEARLY  = QSE_T('l'),

		QSE_SED_CMD_HOLD           = QSE_T('h'),
		QSE_SED_CMD_HOLD_APPEND    = QSE_T('H'),
		QSE_SED_CMD_RELEASE        = QSE_T('g'),
		QSE_SED_CMD_RELEASE_APPEND = QSE_T('G'),
		QSE_SED_CMD_EXCHANGE       = QSE_T('x'), 

		QSE_SED_CMD_NEXT           = QSE_T('n'),
		QSE_SED_CMD_NEXT_APPEND    = QSE_T('N'),

		QSE_SED_CMD_BRANCH         = QSE_T('b'), 
		QSE_SED_CMD_BRANCH_COND    = QSE_T('t'),

		QSE_SED_CMD_READ_FILE      = QSE_T('r'),
		QSE_SED_CMD_READ_FILELN    = QSE_T('R'),
		QSE_SED_CMD_WRITE_FILE     = QSE_T('w'),
		QSE_SED_CMD_WRITE_FILELN   = QSE_T('W'),

		QSE_SED_CMD_SUBSTITUTE     = QSE_T('s'),
		QSE_SED_CMD_TRANSLATE      = QSE_T('y')

	} type;

	int negated;

	qse_sed_a_t a1; /* optional start address */
	qse_sed_a_t a2; /* optional end address */

	union
	{
		/* text for the a, i, c commands */
		qse_xstr_t text;  

		/* file name for r, w, R, W */
		qse_xstr_t file;

		/* data for the s command */
		struct
		{
			void* rex; /* regular expression */
			qse_xstr_t rpl;  /* replacement */

			/* flags */
			qse_xstr_t file; /* file name for w */
			unsigned short occ;
			unsigned short g: 1; /* global */
			unsigned short p: 1; /* print */
			unsigned short i: 1; /* case insensitive */
		} subst;

		/* translation set for the y command */
		qse_xstr_t transet;

		/* branch target for b and t */
		struct
		{
			qse_xstr_t label;
			qse_sed_cmd_t* target;
		} branch;
	} u;	

	struct
	{
		int a1_matched;
		int c_ready;
	} state;
};

#endif
