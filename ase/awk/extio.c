/*
 * $Id: extio.c,v 1.6 2006-06-21 13:52:15 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

int xp_awk_readextio (
	xp_awk_run_t* run, int type, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_io_t handler = run->awk->extio[type];
	xp_bool_t extio_created = xp_false;

	while (p != XP_NULL)
	{
		if (p->type == type &&
		    xp_strcmp(p->name,name) == 0) break;
		p = p->next;
	}

	if (p == XP_NULL)
	{
		p = (xp_awk_extio_t*) xp_malloc (xp_sizeof(xp_awk_extio_t));
		if (p == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (name);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->type = type;
		p->handle = XP_NULL;
		p->next = XP_NULL;
		extio_created = xp_true;
	}

	if (p->handle == XP_NULL)
	{
		if (handler (XP_AWK_INPUT_OPEN, p, XP_NULL, 0) == -1)
		{
			if (extio_created)
			{
				xp_free (p->name);
				xp_free (p);
			}
				
			/* TODO: set ERRNO */
			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		if (p->handle == XP_NULL)
		{
			if (extio_created)
			{
				xp_free (p->name);
				xp_free (p);
			}

			/* wrong implementation of user io handler.
			 * the correct io handler should set p->handle to
			 * non XP_NULL when it returns 0. */
			*errnum = XP_AWK_EIOIMPL;
			return -1;
		}
	}

	/* link it to the extio chain */
	if (extio_created)
	{
		p->next = run->extio;
		run->extio = p;
	}

	{
xp_char_t buf[1024];

	if (handler (XP_AWK_INPUT_DATA, p, buf, xp_countof(buf)) == 0)
	{
		/* no more data. end of data stream */
		return 0;
	}

xp_printf(XP_TEXT("%s"), buf);
	}

	return 1;
}

int xp_awk_closeextio (xp_awk_run_t* run, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio, * px = XP_NULL;

	while (p != XP_NULL)
	{
		 /* it handles the first name that matches 
		  * regardless of the extio type */
		if (xp_strcmp(p->name,name) == 0) 
		{
			xp_awk_io_t handler = run->awk->extio[p->type];

	/* TODO: io command should not be XP_AWK_INPUT_CLOSE 
	 *       it should have more generic form than this... */
			if (handler (XP_AWK_INPUT_CLOSE, p, XP_NULL, 0) == -1)
			{
				/* this is not a run-time error.*/
				*errnum = XP_AWK_ENOERR;
				return -1;
			}

			p->handle = XP_NULL;
			//if (opt_remove_closed_extio) // TODO:...
			//{
				if (px != XP_NULL) px->next = p->next;
				else run->extio = p->next;

				xp_free (p->name);
				xp_free (p);
			//}
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	*errnum = XP_AWK_ENOERR;
	return -1;
}

int xp_awk_clearextio (xp_awk_run_t* run, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_extio_t* next;
	xp_awk_io_t handler;

	while (p != XP_NULL)
	{
		handler = run->awk->extio[p->type];
		next = p->next;

	/* TODO: io command should not be XP_AWK_INPUT_CLOSE 
	 *       it should have more generic form than this... */
		if (handler (XP_AWK_INPUT_CLOSE, p, XP_NULL, 0) == -1)
		{
/* TODO: should it be removed from the chain???? */
			/* this is not a run-time error.*/
			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		p->handle = XP_NULL;
		//if (opt_remove_closed_extio) // TODO:...
		//{
			if (px != XP_NULL) px->next = p->next;
			else run->extio = p->next;

			xp_free (p->name);
			xp_free (p);
		//}

		p = next;
	}

	return 0;
}
