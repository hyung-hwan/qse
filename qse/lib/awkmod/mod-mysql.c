/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mod-mysql.h"
#include <mysql/mysql.h>

#define __IMAP_NODE_T_DATA  MYSQL ctx;
#define __IMAP_LIST_T_DATA /* nothing */
#define __IMAP_LIST_T sql_list_t
#define __IMAP_NODE_T sql_node_t
#define __MAKE_IMAP_NODE __new_sql_node
#define __FREE_IMAP_NODE __free_sql_node
#include "../lib/awk/imap-imp.h"

static sql_node_t* new_sql_node (qse_awk_rtx_t* rtx, sql_list_t* list)
{
	sql_node_t* node;

	node = __new_sql_node(rtx, list);
	if (!node) return QSE_NULL;

	if (mysql_init(&node->ctx) == QSE_NULL)
	{
		__free_sql_node (rtx, list, node);
		return QSE_NULL;
	}

	return node;
}

static void free_sql_node (qse_awk_rtx_t* rtx, sql_list_t* list, sql_node_t* node)
{
	mysqL_close (&node->ctx);
	__free_sql_node (rtx, list, node);
}

static int fnc_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_node_t* node;
	return -1;
}

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return -1;
}

static int fnc_connect (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return -1;
}

static int fnc_query (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return -1;
}


typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

#define A_MAX QSE_TYPE_MAX(int)

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("close"),        { { 1, 1, QSE_NULL },   fnc_close,     0 } },
	{ QSE_T("connect"),      { { 1, 1, QSE_NULL },   fnc_connect,   0 } },
	{ QSE_T("open"),         { { 1, 1, QSE_NULL },   fnc_open,      0 } },
	{ QSE_T("query"),        { { 2, 3, QSE_NULL },   fnc_query,     0 } },
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (fnctab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

#if 0
	left = 0; right = QSE_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (inttab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
     }
#endif

	ea.ptr = (qse_char_t*)name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	mysql_library_init (0, QSE_NULL, QSE_NULL);
	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
	mysql_library_end ();
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int qse_awk_mod_mysql (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	/*
	mod->ctx...
	 */

	

	return 0;
}


int qse_awk_mod_mysql_init (int argc, qse_achar_t* argv[])
{
	if (mysql_library_init(argc, argv, QSE_NULL) != 0) return -1;
	return 0;
}

void qse_awk_mod_mysql_fini (void)
{
	mysql_library_end ();
}
