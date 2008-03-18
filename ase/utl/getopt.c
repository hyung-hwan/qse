/*
 * $Id: getopt.c 137 2008-03-17 12:35:02Z baconevi $
 * 
 * {License}
 */

#include <ase/utl/getopt.h>

/*
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
/* declarations to provide consistent linkage */
extern char *optarg;
extern int optind;
extern int opterr;

int     opterr = 1,             /* if error message should be printed */
        optind = 1,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
char    *optarg;                /* argument associated with option */
#endif

#define BADCH   ASE_T('?')
#define BADARG  ASE_T(':')
#define EMSG    ASE_T("")

#define optstr   opt->str
#define optarg   opt->arg
#define optind   opt->ind
#define optopt   opt->opt
#define place    opt->cur

ase_cint_t ase_getopt (int argc, ase_char_t* const* argv, ase_opt_t* opt)
{
	ase_char_t* oli;                        /* option letter list index */

	if (!*place) 
	{              
		/* update scanning pointer */
		if (optind >= argc || *(place = argv[optind]) != ASE_T('-')) 
		{
			place = EMSG;
			return ASE_CHAR_EOF;
		}
		if (place[1] && *++place == ASE_T('-'))
		{      
			/* found "--" */
			++optind;
			place = EMSG;
			return ASE_CHAR_EOF;
		}
	}                                       /* option letter okay? */

	if ((optopt = (int)*place++) == ASE_T(':') ||
	    !(oli = strchr(optstr, optopt))) 
	{
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (optopt == (int)'-') return ASE_CHAR_EOF;
		if (!*place) ++optind;
	#if 0
		if (opterr && *optstr != ASE_T(':'))
			(void)fprintf(stderr,
				"%s: illegal option -- %c\n", __FILE__, optopt);
	#endif
		return BADCH;
	}

	if (*++oli != ASE_T(':')) 
	{
		/* don't need argument */
		optarg = ASE_NULL;
		if (!*place) ++optind;
	}
	else 
	{                                  
		/* need an argument */

		if (*place) optarg = place; /* no white space */
		else if (argc <= ++optind) 
		{
			/* no arg */
			place = EMSG;
			if (*optstr == ASE_T(':')) return BADARG;
		#if 0
			if (opterr)
				(void)fprintf(stderr,
					"%s: option requires an argument -- %c\n",
					__FILE__, optopt);
		#endif
			return BADCH;
		}
		else
		{                            
			/* white space */
			optarg = argv[optind];
		}
		place = EMSG;
		++optind;
	}

	return optopt;  /* dump back option letter */
}


