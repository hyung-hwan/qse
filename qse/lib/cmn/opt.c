/*
 * $Id: opt.c 550 2011-08-14 15:59:55Z hyunghwan.chung $
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

#include <qse/cmn/opt.h>
#include <qse/cmn/str.h>

/* 
 * qse_getopt is based on BSD getopt.
 * --------------------------------------------------------------------------
 *
 * Copyright (c) 1987-2002 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * A. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * B. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * C. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * --------------------------------------------------------------------------
 */

#define BADCH   QSE_T('?')
#define BADARG  QSE_T(':')
#define EMSG    QSE_T("")

qse_cint_t qse_getopt (int argc, qse_char_t* const* argv, qse_opt_t* opt)
{
	qse_char_t* oli; /* option letter list index */
	int dbldash = 0;

	opt->arg = QSE_NULL;
	opt->lngopt = QSE_NULL;

	if (opt->cur == QSE_NULL) 
	{
		opt->cur = EMSG;
		opt->ind = 1;
	}

	if (*opt->cur == QSE_T('\0')) 
	{              
		/* update scanning pointer */
		if (opt->ind >= argc || *(opt->cur = argv[opt->ind]) != QSE_T('-')) 
		{
			/* All arguments have been processed or the current 
			 * argument doesn't start with a dash */
			opt->cur = EMSG;
			return QSE_CHAR_EOF;
		}

		opt->cur++;

	#if 0
		if (*opt->cur == QSE_T('\0'))
		{
			/* - */
			opt->ind++;
			opt->cur = EMSG;
			return QSE_CHAR_EOF;
		}
	#endif

		if (*opt->cur == QSE_T('-'))
		{
			if (*++opt->cur == QSE_T('\0'))
			{
				/* -- */
				opt->ind++;
				opt->cur = EMSG;
				return QSE_CHAR_EOF;
			}
			else
			{
				dbldash = 1;
			}
		}
	}   

	if (dbldash && opt->lng != QSE_NULL)
	{	
		const qse_opt_lng_t* o;
		qse_char_t* end = opt->cur;

		while (*end != QSE_T('\0') && *end != QSE_T('=')) end++;

		for (o = opt->lng; o->str; o++) 
		{
			const qse_char_t* str = o->str;

			if (*str == QSE_T(':')) str++;

			if (qse_strxcmp (opt->cur, end-opt->cur, str) != 0) continue;
	
			/* match */
			opt->cur = EMSG;
			opt->lngopt = o->str;

			/* for a long matching option, remove the leading colon */
			if (opt->lngopt[0] == QSE_T(':')) opt->lngopt++;

			if (*end == QSE_T('=')) opt->arg = end + 1;

			if (*o->str != QSE_T(':'))
			{
				/* should not have an option argument */
				if (opt->arg != QSE_NULL) return BADARG;
			}
			else if (opt->arg == QSE_NULL)
			{
				/* check if it has a remaining argument 
				 * available */
				if (argc <= ++opt->ind) return BADARG; 
				/* If so, the next available argument is 
				 * taken to be an option argument */
				opt->arg = argv[opt->ind];
			}

			opt->ind++;
			return o->val;
		}

		/*if (*end == QSE_T('=')) *end = QSE_T('\0');*/
		opt->lngopt = opt->cur; 
		return BADCH;
	}

	if ((opt->opt = *opt->cur++) == QSE_T(':') ||
	    (oli = qse_strchr(opt->str, opt->opt)) == QSE_NULL) 
	{
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (opt->opt == (int)'-') return QSE_CHAR_EOF;
		if (*opt->cur == QSE_T('\0')) ++opt->ind;
		return BADCH;
	}

	if (*++oli != QSE_T(':')) 
	{
		/* don't need argument */
		if (*opt->cur == QSE_T('\0')) opt->ind++;
	}
	else 
	{                                  
		/* need an argument */

		if (*opt->cur != QSE_T('\0')) 
		{
			/* no white space */
			opt->arg = opt->cur;
		}
		else if (argc <= ++opt->ind) 
		{
			/* no arg */
			opt->cur = EMSG;
			/*if (*opt->str == QSE_T(':'))*/ return BADARG;
			/*return BADCH;*/
		}
		else
		{                            
			/* white space */
			opt->arg = argv[opt->ind];
		}

		opt->cur = EMSG;
		opt->ind++;
	}

	return opt->opt;  /* dump back option letter */
}

