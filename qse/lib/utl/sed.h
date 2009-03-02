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
		void* rex;        /* regular expression */
		qse_char_t* text; /* added text or file name */
		qse_sed_c_t* lbl; /* destination command of branch */

	} u;	

	qse_char_t* rhs; /* right-hand side of sustitution */

	enum
	{
		QSE_SED_C_JMP,

		QSE_SED_C_A, /* append text */
		QSE_SED_C_B,
		QSE_SED_C_C,
		QSE_SED_C_CD,
		QSE_SED_C_CN,
		QSE_SED_C_CO,
		QSE_SED_C_CP,
		QSE_SED_C_D,
		QSE_SED_C_E,
		QSE_SED_C_EQ, /* print the current line number */
		QSE_SED_C_F,
		QSE_SED_C_G,
		QSE_SED_C_CG,
		QSE_SED_C_H,
		QSE_SED_C_CH,
		QSE_SED_C_I,
		QSE_SED_C_L,
		QSE_SED_C_N,
		QSE_SED_C_P,
		QSE_SED_C_Q,
		QSE_SED_C_R,
		QSE_SED_C_S,
		QSE_SED_C_T,
		QSE_SED_C_W,
		QSE_SED_C_CW,
		QSE_SED_C_Y,
		QSE_SED_C_X
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
