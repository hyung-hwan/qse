/*
 * $Id: extio.c,v 1.5 2006-06-19 09:08:50 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

int xp_awk_readextio (xp_awk_run_t* run, 
	xp_awk_extio_t** extio, xp_awk_io_t handler,
	const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = *extio;
	xp_bool_t extio_created = xp_false;

	while (p != XP_NULL)
	{
		if (xp_strcmp(p->name,name) == 0) break;
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
		p->next = *extio;
		*extio = p;
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

int xp_awk_closeextio (xp_awk_run_t* run, 
	xp_awk_extio_t** extio, xp_awk_io_t handler,
	const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = *extio, * px = XP_NULL;

	while (p != XP_NULL)
	{
		if (xp_strcmp(p->name,name) == 0) 
		{
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
				else *extio = p->next;

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
