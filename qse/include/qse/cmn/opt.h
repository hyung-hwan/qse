/*
 * $Id: opt.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_OPT_H_
#define _QSE_CMN_OPT_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines functions and data structures to process 
 * command-line arguments. 
 */

typedef struct qse_opt_t qse_opt_t;
typedef struct qse_opt_lng_t qse_opt_lng_t;

struct qse_opt_lng_t
{
	const qse_char_t* str;
	qse_cint_t        val;
};

struct qse_opt_t
{
	/* input */
	const qse_char_t* str; /* option string  */
	qse_opt_lng_t*    lng; /* long options */

	/* output */
	qse_cint_t        opt; /* character checked for validity */
	qse_char_t*       arg; /* argument associated with an option */

	/* output */
	const qse_char_t* lngopt; 

	/* input + output */
	int               ind; /* index into parent argv vector */

	/* input + output - internal*/
	qse_char_t*        cur;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_getopt() function processes the @a argc command-line arguments
 * pointed to by @a argv as configured in @a opt. It can process two 
 * different option styles: a single character starting with '-', and a 
 * long name starting with '--'. 
 *
 * A character in @a opt.str is treated as a single character option. Should
 * it require a parameter, specify ':' after it.
 *
 * Two special returning option characters indicate special error conditions. 
 * - @b ? indicates a bad option stored in the @a opt->opt field.
 * - @b : indicates a bad parameter for an option stored in the 
 *      @a opt->opt field.
 *
 * @return an option character on success, QSE_CHAR_EOF on no more options.
 */
qse_cint_t qse_getopt (
	int                argc, /* argument count */ 
	qse_char_t* const* argv, /* argument array */
	qse_opt_t*         opt   /* option configuration */
);

#ifdef __cplusplus
}
#endif

#endif
