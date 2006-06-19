/*
 * $Id: extio.c,v 1.4 2006-06-19 04:38:51 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

/* TODO: a lot of todo's here... */
#include <tchar.h>
#include <stdio.h>

int xp_awk_readcmd (xp_awk_run_t* run, const xp_char_t* cmd, int* errnum)
{
	xp_awk_cmd_t* p = run->extio.incmd;

	while (p != XP_NULL)
	{
		if (xp_strcmp(p->name,cmd) == 0) break;
		p = p->next;
	}

	if (p == XP_NULL)
	{
		p = (xp_awk_cmd_t*) xp_malloc (xp_sizeof(xp_awk_cmd_t));
		if (p == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (cmd);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->handle = XP_NULL;
		p->next = run->extio.incmd;
		run->extio.incmd = p;
	}

	if (p->handle == XP_NULL)
	{
		xp_awk_io_t pipe_io = run->awk->extio.pipe;

		if (pipe_io (XP_AWK_INPUT_OPEN, p, XP_NULL, 0) == -1)
		{
			/* this is treated as pipe open error.
			 * the return value of getline should be -1
			 * set ERRNO as well....
			 */
			return -1;
		}

		if (p->handle == XP_NULL)
		{
			/* TODO: break the chain ... */


			/* *errnum = XP_AWK_EEXTIO; external io handler error */
			*errnum = XP_AWK_EINTERNAL;
			return -1;
		}
	}

	{
xp_char_t buf[1024];
xp_awk_io_t pipe_io = run->awk->extio.pipe;

	if (pipe_io (XP_AWK_INPUT_DATA, p, buf, xp_countof(buf)) == 0)
	{
		return 0;
	}

xp_printf(XP_TEXT("%s"), buf);
	}

	return 1;
}

int xp_awk_closecmd (xp_awk_run_t* run, const xp_char_t* cmd, int* errnum)
{
	xp_awk_cmd_t* p = run->extio.incmd, * px = XP_NULL;

	while (p != XP_NULL)
	{
		if (xp_strcmp(p->name,cmd) == 0) 
		{
			xp_awk_io_t pipe_io = run->awk->extio.pipe;

			if (pipe_io (XP_AWK_INPUT_CLOSE, p, XP_NULL, 0) == -1)
			{
				/* TODO: how to handle this... */
				p->handle = XP_NULL;
				return -1;
			}

			p->handle = XP_NULL;
			//if (opt_remove_closed_cmd)
			//{
				if (px != XP_NULL) px->next = p->next;
				else run->extio.incmd = p->next;

				xp_free (p->name);
				xp_free (p);
			//}
			return 0;
		}

		px = p;
		p = p->next;
	}

	return -1;
}
