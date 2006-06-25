/*
 * $Id: extio.c,v 1.10 2006-06-25 15:26:57 bacon Exp $
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
	int ioopt;

	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

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

		if (type == XP_AWK_EXTIO_PIPE) 
			ioopt = XP_AWK_IO_PIPE_READ;
		else if (type == XP_AWK_EXTIO_FILE) 
			ioopt = XP_AWK_IO_FILE_READ;
		else ioopt = 0; /* TODO: how to handle this??? */

		if (handler (XP_AWK_IO_OPEN, ioopt, p, XP_NULL, 0) == -1)
		{
			xp_free (p->name);
			xp_free (p);
				
			/* TODO: set ERRNO */
			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		if (p->handle == XP_NULL)
		{
			xp_free (p->name);
			xp_free (p);

			/* wrong implementation of user io handler.
			 * the correct io handler should set p->handle to
			 * non XP_NULL when it returns 0. */
			*errnum = XP_AWK_EIOIMPL;
			return -1;
		}

		/* chain it */
		p->next = run->extio;
		run->extio = p;
	}

	{
xp_char_t buf[1024];

	if (handler (XP_AWK_IO_READ, 0, p, buf, xp_countof(buf)) == 0)
	{
		/* no more data. end of data stream */
		return 0;
	}

xp_printf(XP_TEXT("%s"), buf);
	}

	return 1;
}

int xp_awk_writeextio (
	xp_awk_run_t* run, int type, 
	const xp_char_t* name, xp_awk_val_t* v, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_io_t handler = run->awk->extio[type];
	int ioopt;

	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

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

		if (type == XP_AWK_EXTIO_PIPE) 
			ioopt = XP_AWK_IO_PIPE_READ;
		else if (type == XP_AWK_EXTIO_FILE) 
			ioopt = XP_AWK_IO_FILE_READ;
		else if (type == XP_AWK_EXTIO_CONSOLE)
			ioopt = XP_AWK_IO_CONSOLE_READ;
		else ioopt = 0; /* TODO: how to handle this??? */

		if (handler (XP_AWK_IO_OPEN, ioopt, p, XP_NULL, 0) == -1)
		{
			xp_free (p->name);
			xp_free (p);
				
			/* TODO: set ERRNO */
			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		if (p->handle == XP_NULL)
		{
			xp_free (p->name);
			xp_free (p);

			/* wrong implementation of user io handler.
			 * the correct io handler should set p->handle to
			 * non XP_NULL when it returns 0. */
			*errnum = XP_AWK_EIOIMPL;
			return -1;
		}

		/* chain it */
		p->next = run->extio;
		run->extio = p;
	}

	{
		xp_awk_printval (v);
#if 0
xp_char_t buf[1024] = XP_T("temp...");;

	if (handler (XP_AWK_IO_WRITE, 0, p, buf, xp_countof(buf)) == 0)
	{
		/* no more data. end of data stream */
		return 0;
	}

xp_printf(XP_TEXT("%s"), buf);
#endif
	}

	return 1;
}

int xp_awk_closeextio (xp_awk_run_t* run, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio, * px = XP_NULL;

	while (p != XP_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (xp_strcmp(p->name,name) == 0) 
		{
			xp_awk_io_t handler = run->awk->extio[p->type];

			if (handler != NULL)
			{
	/* TODO: io command should not be XP_AWK_IO_CLOSE 
	 *       it should be more generic form than this... */
				if (handler (XP_AWK_IO_CLOSE, 0, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					*errnum = XP_AWK_ENOERR;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	*errnum = XP_AWK_ENOERR;
	return -1;
}

void xp_awk_clearextio (xp_awk_run_t* run)
{
	xp_awk_extio_t* next;
	xp_awk_io_t handler;
	int n;

	while (run->extio != XP_NULL)
	{
		handler = run->awk->extio[run->extio->type];
		next = run->extio->next;

		if (handler != XP_NULL)
		{
	/* TODO: io command should not be XP_AWK_INPUT_CLOSE 
	 *       it should be more generic form than this... */
			n = handler (XP_AWK_IO_CLOSE, 0, run->extio, XP_NULL, 0);
			if (n == -1)
			{
				/* TODO: 
				 * some warning actions need to be taken */
			}
		}

		xp_free (run->extio->name);
		xp_free (run->extio);

		run->extio = next;
	}
}
