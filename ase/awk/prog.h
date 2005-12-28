/*
 * $Id: prog.h,v 1.1 2005-12-28 15:57:27 bacon Exp $
 */

#ifndef _XP_AWK_PROG_H_
#define _XP_AWK_PROG_H_


struct 
{
	/* line number */
	xp_size_t linenum;

	union 
	{
		const xp_char_t* func_name;
	} ptn;

	union
	{
		struct
		{
			unsigned short nargs;
			xp_awk_body_t body;
		} func;
	} act;
};

#endif
