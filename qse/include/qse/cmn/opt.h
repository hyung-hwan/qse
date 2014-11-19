/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#if defined(__cplusplus)
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
QSE_EXPORT qse_cint_t qse_getopt (
	int                argc, /* argument count */ 
	qse_char_t* const* argv, /* argument array */
	qse_opt_t*         opt   /* option configuration */
);

#if defined(__cplusplus)
}
#endif

#endif
