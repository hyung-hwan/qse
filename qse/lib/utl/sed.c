/*
 * $Id$
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "sed.h"
#include "../cmn/mem.h"
#include <qse/cmn/rex.h>

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

qse_sed_t* qse_sed_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_sed_t* sed;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	sed = (qse_sed_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sed_t) + xtn);
	if (sed == QSE_NULL) return QSE_NULL;

	if (qse_sed_init (sed, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (sed->mmgr, sed);
		return QSE_NULL;
	}

	return sed;
}

void qse_sed_close (qse_sed_t* sed)
{
	qse_sed_fini (sed);
	QSE_MMGR_FREE (sed->mmgr, sed);
}

qse_sed_t* qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (sed, 0, sizeof(*sed));
	sed->mmgr = mmgr;

	if (qse_str_init (&sed->rexbuf, mmgr, 0) == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOMEM;
		return QSE_NULL;	
	}	

	return sed;
}

void qse_sed_fini (qse_sed_t* sed)
{
	qse_str_fini (&sed->rexbuf);
}

/* get the current character without advancing the pointer */
#define CC(ptr,end) ((ptr < end)? *ptr: QSE_CHAR_EOF)
/* get the current character advancing the pointer */
#define NC(ptr,end) ((ptr < end)? *ptr++: QSE_CHAR_EOF)

static const void* compile (
	qse_sed_t* sed, const qse_char_t* ptr, 
	const qse_char_t* end, qse_char_t seof)
{
	void* code;
	qse_cint_t c;

	qse_str_clear (&sed->rexbuf);

	for (;;)
	{
		c = NC (ptr, end);
		if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
		{
			sed->errnum = QSE_SED_ETMTXT;
			return QSE_NULL;
		}

		if (c == seof) break;

		if (c == QSE_T('\\'))
		{
			c = NC (ptr, end);
			if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
			{
				sed->errnum = QSE_SED_ETMTXT;
				return QSE_NULL;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			// TODO: support more escaped characters??
		}

		if (qse_str_ccat (&sed->rexbuf, c) == (qse_size_t)-1)
		{
			sed->errnum = QSE_SED_ENOMEM;
			return QSE_NULL;
		}
	} 

	/* TODO: maximum depth - optionize the second  parameter */
	code = qse_buildrex (
		sed->mmgr, 0, 
		QSE_STR_PTR(&sed->rexbuf), 
		QSE_STR_LEN(&sed->rexbuf), 
		QSE_NULL);
	if (code == QSE_NULL)
	{
		sed->errnum = QSE_SED_EREXBL;
		return QSE_NULL;
	}

	sed->lastrex = code;
	return ptr;
}

static const qse_char_t* address (
	qse_sed_t* sed, const qse_char_t* ptr, 
	const qse_char_t* end, qse_sed_a_t* a)
{
	qse_cint_t c;

	c = NC (ptr, end);
	if ((c = *ptr) == QSE_T('$'))
	{
		a->type = QSE_SED_A_DOL;
		ptr++;
	}
	else if (c == QSE_T('/'))
	{
		ptr++;
		if (compile (sed, ptr, end, c) == QSE_NULL) 
			return QSE_NULL;

		a->u.rex = sed->lastrex;
		a->type = QSE_SED_A_REX;
	}
	else if (c >= QSE_T('0') && c <= QSE_T('9'))
	{
		qse_sed_line_t lno = 0;
		do
		{
			lno = lno * 10 + c - QSE_T('0');
			ptr++;
		}
		while ((c = *ptr) >= QSE_T('0') && c <= QSE_T('9'));

		/* line number 0 is illegal */
		if (lno == 0) return QSE_NULL;

		a->type = QSE_SED_A_LINE;
		a->u.line = lno;
	}
	else if (c == QSE_T('\\'))
	{
		/* TODO */
	}
	else
	{
		a->type = QSE_SED_A_NONE;
	}

	return ptr;
}

static const qse_char_t* command (
	qse_sed_t* sed, const qse_char_t* ptr, const qse_char_t* end)
{
	qse_cint_t c;

	c = CC (ptr, end);
	
	switch (c)
	{
		default:
			sed->errnum = QSE_SED_ECMDNR;
			return QSE_NULL;

#if 0
		case QSE_T('{'):
			/* insert a negaited branch command at the beginning 
			 * of a group. this way, all the commands in a group
			 * can be skipped. the branch target is set once a
			 * corresponding } is met. */
			cmd.type = QSE_SED_C_BRANCH;
			cmd.negfl = !cmd.negfl;
			break;

		case QSE_T('}'):
			break;

		case QSE_T('='):
			cmd = QSE_SED_C_EQ;
			if (ad2.type != QSE_SED_A_NONE)
			{
				sed->errnum = QSE_SED_EA2NNC;
				return QSE_NULL;
			}
			break;
#endif

		case QSE_T(':'):
			break;

		case QSE_T('a'):
			break;

		case QSE_T('c'):
			break;

		case QSE_T('i'):
			break;

		case QSE_T('g'):
			break;

		case QSE_T('G'):
			break;

		case QSE_T('h'):
			break;

		case QSE_T('H'):
			break;

		case QSE_T('t'):
			break;

		case QSE_T('b'):
			break;

		case QSE_T('n'):
			break;

		case QSE_T('N'):
			break;

		case QSE_T('p'):
			break;

		case QSE_T('P'):
			break;

		case QSE_T('r'):
			break;

		case QSE_T('d'):
			break;

		case QSE_T('D'):
			break;

		case QSE_T('q'):
			break;

		case QSE_T('l'):
			break;

		case QSE_T('s'):
			break;

		case QSE_T('w'):
			break;

		case QSE_T('x'):
			break;

		case QSE_T('y'):
			break;
	}

	return ptr;
}

static const qse_char_t* fcomp (
	qse_sed_t* sed, const qse_char_t* ptr, qse_size_t len)
{
	qse_cint_t c;
	const qse_char_t* end = ptr + len;
	qse_sed_a_t a1, a2;

	while (1)
	{
		c = CC (ptr, end);

		/* skip white spaces */
		while (c == QSE_T(' ') || c == QSE_T('\t')) 
		{
			ptr++;
			c = CC (ptr, end);
		}

		/* check if it has reached the end or is commented */
		if (c == QSE_CHAR_EOF || c == QSE_T('#')) break;

		if (c == QSE_T(';')) 
		{
			ptr++;
			continue;
		}

		/* process address */
		ptr = address (sed, ptr, end, &a1);
		if (ptr == QSE_NULL) return QSE_NULL;

		c = CC (ptr, end);
		if (a1.type != QSE_SED_A_NONE)
		{
			/* if (a1.type == QSE_SED_A_LAST)
			{
				 // TODO: ????
			} */
			if (c == QSE_T(',') || c == QSE_T(';'))
			{
				ptr++;
				ptr = address (sed, ptr, end, &a2);
				if (ptr == QSE_NULL) return QSE_NULL;
				c = CC (ptr, end);
			}
			else a2.type = QSE_SED_A_NONE;
		}

		/* skip white spaces */
		while (c == QSE_T(' ') || c == QSE_T('\t')) 
		{
			ptr++;
			c = CC (ptr, end);
		}


		if (c == QSE_T('!'))
		{
			/* negate */
		}

		ptr = command (sed, ptr, end);
		if (ptr == QSE_NULL) return QSE_NULL;
	}

	return ptr;
}
