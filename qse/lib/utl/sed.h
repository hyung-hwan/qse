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

		/* a \<\n> text - append text */
		QSE_SED_CMD_APPEND = QSE_T('a'),
		/* i \<\n> text - insert text */
		QSE_SED_CMD_INSERT = QSE_T('i'),
		/* c \<\n> text - change text */
		QSE_SED_CMD_CHANGE = QSE_T('c'),

		/* delete pattern space */
		QSE_SED_CMD_DELETE = QSE_T('d'),
		QSE_SED_CMD_DD     = QSE_T('D'),

		QSE_SED_CMD_PRINT_LNUM     = QSE_T('='),
		QSE_SED_CMD_PRINT          = QSE_T('p'),
		QSE_SED_CMD_PP             = QSE_T('P'),
		QSE_SED_CMD_PRINT_CLEARLY  = QSE_T('l'),

		QSE_SED_CMD_HOLD           = QSE_T('h'),
		QSE_SED_CMD_HOLD_APPEND    = QSE_T('H'),
		QSE_SED_CMD_RELEASE        = QSE_T('g'),
		QSE_SED_CMD_RELEASE_APPEND = QSE_T('G'),
		QSE_SED_CMD_EXCHANGE       = QSE_T('x'), 

		QSE_SED_CMD_N   = QSE_T('n'),
		QSE_SED_CMD_NN  = QSE_T('N'),

		/* branch */
		QSE_SED_CMD_BRANCH = QSE_T('b'), 
		QSE_SED_CMD_T   = QSE_T('t'),

		/* r filename - append a text from a file */
		QSE_SED_CMD_R   = QSE_T('r'),
		/* R filename - append a line from a file */
		QSE_SED_CMD_RR  = QSE_T('R'),

		/* w filename - write pattern space to a file */
		QSE_SED_CMD_W   = QSE_T('w'),
		/* W filename - write first line of pattern space to a file */
		QSE_SED_CMD_WW  = QSE_T('W'),

		/* s/regex/str/ - replace matching pattern with a new string */
		QSE_SED_CMD_S   = QSE_T('s'),
		/* y/s/d/ - translate characters in s to characters in d */
		QSE_SED_CMD_Y   = QSE_T('y')

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

		struct
		{
			qse_xstr_t label;
			qse_sed_cmd_t* target;
		} branch;

		void* rex;
	} u;	

	struct
	{
		int a1_matched;
		int c_ready;
	} state;
};

#endif
