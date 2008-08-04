/*
 * $Id: getopt.c 313 2008-08-03 14:06:43Z baconevi $
 * 
 * {License}
 */

#include <ase/utl/getopt.h>
#include <ase/cmn/str.h>

/* 
 * ase_getopt is based on BSD getopt.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BADCH   ASE_T('?')
#define BADARG  ASE_T(':')
#define EMSG    ASE_T("")

ase_cint_t ase_getopt (int argc, ase_char_t* const* argv, ase_opt_t* opt)
{
	ase_char_t* oli; /* option letter list index */
	int dbldash = 0;

	opt->arg = ASE_NULL;
	opt->lngopt = ASE_NULL;

	if (opt->cur == ASE_NULL) 
	{
		opt->cur = EMSG;
		opt->ind = 1;
	}

	if (*opt->cur == ASE_T('\0')) 
	{              
		/* update scanning pointer */
		if (opt->ind >= argc || *(opt->cur = argv[opt->ind]) != ASE_T('-')) 
		{
			/* All arguments have been processed or the current 
			 * argument doesn't start with a dash */
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}

		opt->cur++;

	#if 0
		if (*opt->cur == ASE_T('\0'))
		{
			/* - */
			opt->ind++;
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}
	#endif

		if (*opt->cur == ASE_T('-'))
		{
			if (*++opt->cur == ASE_T('\0'))
			{
				/* -- */
				opt->ind++;
				opt->cur = EMSG;
				return ASE_CHAR_EOF;
			}
			else
			{
				dbldash = 1;
			}
		}
	}   

	if (dbldash && opt->lng != ASE_NULL)
	{	
		const ase_opt_lng_t* o;
		ase_char_t* end = opt->cur;

		while (*end != ASE_T('\0') && *end != ASE_T('=')) end++;

		for (o = opt->lng; o->str != ASE_NULL; o++) 
		{
			const ase_char_t* str = o->str;
			if (*str == ASE_T(':')) str++;

			if (ase_strxcmp (opt->cur, end-opt->cur, str) != 0) continue;
	
			/* match */
			opt->cur = EMSG;
			opt->lngopt = o->str;
			if (*end == ASE_T('=')) opt->arg = end + 1;

			if (*o->str != ASE_T(':'))
			{
				/* should not have an option argument */
				if (opt->arg != ASE_NULL) return BADARG;
			}
			else if (opt->arg == ASE_NULL)
			{
				/* Check if it has a remaining argument 
				 * available */
				if (argc <= ++opt->ind) return BADARG; 
				/* If so, the next available argument is 
				 * taken to be an option argument */
				opt->arg = argv[opt->ind];
			}

			opt->ind++;
			return o->val;
		}

		/*if (*end == ASE_T('=')) *end = ASE_T('\0');*/
		opt->lngopt = opt->cur;
		return BADCH;
	}

	if ((opt->opt = *opt->cur++) == ASE_T(':') ||
	    (oli = ase_strchr(opt->str, opt->opt)) == ASE_NULL) 
	{
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (opt->opt == (int)'-') return ASE_CHAR_EOF;
		if (*opt->cur == ASE_T('\0')) ++opt->ind;
		return BADCH;
	}

	if (*++oli != ASE_T(':')) 
	{
		/* don't need argument */
		if (*opt->cur == ASE_T('\0')) opt->ind++;
	}
	else 
	{                                  
		/* need an argument */

		if (*opt->cur != ASE_T('\0')) 
		{
			/* no white space */
			opt->arg = opt->cur;
		}
		else if (argc <= ++opt->ind) 
		{
			/* no arg */
			opt->cur = EMSG;
			/*if (*opt->str == ASE_T(':'))*/ return BADARG;
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

