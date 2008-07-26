/*
 * $Id: getopt.c 288 2008-07-25 15:01:27Z baconevi $
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

#if 0
ase_cint_t ase_getopt (int argc, ase_char_t* const* argv, ase_opt_t* opt)
{
	ase_char_t* oli; /* option letter list index */

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
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}

		if (opt->cur[1] != ASE_T('\0') && *++opt->cur == ASE_T('-'))
		{      
			/* found "--" */
			++opt->ind;
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}
	}   /* option letter okay? */

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
		opt->arg = ASE_NULL;
		if (*opt->cur == ASE_T('\0')) ++opt->ind;
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
		++opt->ind;
	}

	return opt->opt;  /* dump back option letter */
}
#endif

/* Code based on Unununium project (http://unununium.org/) */

ase_cint_t ase_getopt (int argc, ase_char_t* const* argv, ase_opt_t* opt)
{
	ase_char_t* oli; /* option letter list index */

	if (opt->ind == 0) opt->ind = 1;

//again:
	if (opt->ind >= argc /*|| !argv[opt->ind] */|| 
	    argv[opt->ind][0] != ASE_T('-') || 
	    argv[opt->ind][1] == ASE_T('\0')) return ASE_CHAR_EOF;

	if (argv[opt->ind][1] == ASE_T('-') && 
	    argv[opt->ind][2] == ASE_T('\0')) 
	{
		/* -- */
		++opt->ind;
		return ASE_CHAR_EOF;
	}

	// TODO: how to handle when lng is off? is this correct?
	if (opt->lng != ASE_NULL && argv[opt->ind][1] == ASE_T('-')) 
	{	
		/* a long option */

		ase_char_t* arg = argv[opt->ind] + 2;
		const ase_opt_lng_t* o;

		/* TODO: rewrite it.. */
		/*char* max=strchr(arg,'=');
		if (max == ASE_NULL) max = arg + strlen(arg);*/
		ase_char_t* max = arg;
		while (*max != ASE_T('\0') && *max != ASE_T('=')) max++;

		//for (o = longopts; o->name != ASE_NULL; o++) 
		for (o = opt->lng; o->str != ASE_NULL; o++) 
		{
			//if (!strncmp (o->name, arg, max - arg)) 
			if (ase_strxcmp (arg, max-arg, o->str) != 0) continue;

			/* match */
			opt->lngind = o - opt->lng;
			if (o->has_arg > 0) 
			{
				if (*max == ASE_T('=')) 
				{
					opt->arg = max + 1;
				}
				else 
				{
					opt->arg = argv[opt->ind+1];
					if (!opt->arg && o->has_arg == 1) 
					{	/* no argument there */
						if (*opt->str == ASE_T(':')) 
							return ASE_T(':');

						/*
						write(2,"argument required: `",20);
						write(2,arg,(size_t)(max-arg));
						write(2,"'.\n",3);
						*/
						opt->ind++;
						return ASE_T('?');
					}
					opt->ind++;
				}
			}

			opt->ind++;

			if (o->flag) *(o->flag) = o->val;
			else return o->val;

			return 0;
		}

		if (*opt->str == ASE_T(':')) return ASE_T(':');

		/*
		write(2,"invalid option `",16);
		write(2,arg,(size_t)(max-arg));
		write(2,"'.\n",3);
		*/

		opt->ind++;
		return ASE_T('?');
	}

#if 0
	if (lastidx != opt->ind) 
	{
		lastidx=opt->ind; lastofs=0;
	}

	opt->opt=argv[opt->ind][lastofs+1];
	
	tmp=ase_strchr(opt->str,opt->opt);
	if (tmp != ASE_NULL)
	{
		if (tmp[0] == ASE_T('\0')) 
		{	/* apparently, we looked for \0, i.e. end of argument */
			++opt->ind;
			goto again;
		}
		if (tmp[1] == ASE_T(':')) 
		{	/* argument expected */
			if (tmp[2]==':' || argv[opt->ind][lastofs+2]) {	/* "-foo", return "oo" as opt->arg */
				if (!*(opt->arg=argv[opt->ind]+lastofs+2)) opt->arg=0;
				goto found;
			}
			opt->arg=argv[opt->ind+1];
			if (!opt->arg) {	/* missing argument */
				++opt->ind;
				if (*opt->str==':') return ':';
				getopterror(1);
				return ':';
			}
			++opt->ind;
		} 
		else 
		{
			++lastofs;
			return opt->opt;
		}
found:
		++opt->ind;
		return optopt;
	} 
	else 
	{	/* not found */
		getopterror(0);
		++opt->ind;
		return '?';
	}
#endif

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
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}

		if (opt->cur[1] != ASE_T('\0') && *++opt->cur == ASE_T('-'))
		{      
			/* found "--" */
			++opt->ind;
			opt->cur = EMSG;
			return ASE_CHAR_EOF;
		}
	}   /* option letter okay? */

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
		opt->arg = ASE_NULL;
		if (*opt->cur == ASE_T('\0')) ++opt->ind;
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
		++opt->ind;
	}

	return opt->opt;  /* dump back option letter */
}

