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

struct qse_sed_c_t
{
	qse_sed_a_t a1; /* optional start address */
	qse_sed_a_t a2; /* optional end address */

	union
	{
		qse_str_t* text;  
		void* rex;
		qse_sed_c_t* label; /* branch destination */
	} u;	

	qse_char_t* rhs; /* right-hand side of sustitution */

	enum
	{
		QSE_SED_CMD_B   = QSE_T('b'), /* branch */
		/* print current line number */
		QSE_SED_CMD_EQ  = QSE_T('='), /* print current line number */

		/* a \<\n> text - append text */
		QSE_SED_CMD_A   = QSE_T('a'),
		/* i \<\n> text - insert text */
		QSE_SED_CMD_I   = QSE_T('i'),
		/* c \<\n> text - change text */
		QSE_SED_CMD_C   = QSE_T('c'),

		QSE_SED_CMD_D   = QSE_T('d'), /* delete pattern space */
		QSE_SED_CMD_DD  = QSE_T('D'),

		QSE_SED_CMD_H   = QSE_T('h'),
		QSE_SED_CMD_HH  = QSE_T('H'),
		QSE_SED_CMD_G   = QSE_T('g'),
		QSE_SED_CMD_GG  = QSE_T('G'),
		/* list out current line */
		QSE_SED_CMD_L   = QSE_T('l'),
		QSE_SED_CMD_N   = QSE_T('n'),
		QSE_SED_CMD_NN  = QSE_T('N'),
		QSE_SED_CMD_P   = QSE_T('p'),
		QSE_SED_CMD_PP  = QSE_T('P'),
		/* exchange hold space and pattern space */
		QSE_SED_CMD_X   = QSE_T('x'), 

		/* r filename - append a text from a file */
		QSE_SED_CMD_R   = QSE_T('r'),
		/* R filename - append a line from a file */
		QSE_SED_CMD_RR  = QSE_T('R'),

		/* w filename - write pattern space to a file */
		QSE_SED_CMD_W   = QSE_T('r'),
		/* W filename - write first line of pattern space to a file */
		QSE_SED_CMD_WW  = QSE_T('R'),

		QSE_SED_CMD_Q   = QSE_T('q'),
		QSE_SED_CMD_QQ  = QSE_T('Q'),

		QSE_SED_CMD_S   = QSE_T('s'),
		QSE_SED_CMD_Y   = QSE_T('y')

	} type;

	/* TODO: change the data type to a shorter one to save space */
	int negfl;
};

struct qse_sed_l_t
{
	qse_char_t name[9];   /* label name */
	qse_sed_c_t* chain;   
	qse_sed_c_t* address; /* command associated with label */

};

#endif
