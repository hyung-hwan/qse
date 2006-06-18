/*
 * $Id: extio.c,v 1.3 2006-06-18 13:43:28 bacon Exp $
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
		xp_awk_io_t pipe_io = run->awk.extio.pipe;

		//p->handle = (void*) _tpopen (p->name, XP_T("r"));
		p->handle = pipe_io (XP_AWK_INPUT_OPEN, p->name, XP_NULL, 0);
		if (p->handle == NULL) 
		{
			/* this is treated as pipe open error.
			 * the return value of getline should be -1
			 * set ERRNO as well....
			 */
			return -1;
		}
	}

	{
	xp_char_t buf[1024];
	if (_fgetts (buf, xp_countof(buf), p->handle) == XP_NULL) return 0;
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
			fclose ((FILE*)p->handle);
			p->handle = XP_NULL;

			//if (opt_remove_closed_cmd)
			//{
				if (px != XP_NULL) px->next = p->next;
				else run->extio.incmd = p->next;

				xp_free (p->name);
				xp_free (p);
			//}
		}

		px = p;
		p = p->next;
	}

	return -1;
}
