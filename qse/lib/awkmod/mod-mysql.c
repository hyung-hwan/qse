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
#include "../cmn/mem-prv.h"
#include <qse/cmn/main.h>
#include <qse/cmn/rbt.h>

#define __IMAP_NODE_T_DATA  MYSQL* mysql;
#define __IMAP_LIST_T_DATA  int errnum;
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

	node->mysql = mysql_init(QSE_NULL);
	if (!node->mysql)
	{
		qse_awk_rtx_seterrfmt (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_T("unable to allocate a mysql object"));
		return QSE_NULL;
	}

	return node;
}

static void free_sql_node (qse_awk_rtx_t* rtx, sql_list_t* list, sql_node_t* node)
{
	mysql_close (node->mysql);
	node->mysql = QSE_NULL;
	__free_sql_node (rtx, list, node);
}

/* ------------------------------------------------------------------------ */

static QSE_INLINE sql_list_t* rtx_to_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	pair = qse_rbt_search((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (sql_list_t*)QSE_RBT_VPTR(pair);
}

static QSE_INLINE sql_node_t* get_list_node (sql_list_t* list, qse_awk_int_t id)
{
	if (id < 0 || id >= list->map.high || !list->map.tab[id]) return QSE_NULL;
	return list->map.tab[id];
}

static int fnc_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* list;
	sql_node_t* node = QSE_NULL;
	qse_awk_int_t ret;
	qse_awk_val_t* retv;

	list = rtx_to_list(rtx, fi);

	node = new_sql_node(rtx, list);
	if (node) ret = node->id;
	else ret = -1;

	/* ret may not be a statically managed number. 
	 * error checking is required */
	retv = qse_awk_rtx_makeintval(rtx, ret);
	if (retv == QSE_NULL)
	{
		if (node) free_sql_node (rtx, list, node);
		return -1;
	}

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* list;
	sql_node_t* node;
	qse_awk_int_t id;
	int ret;

	list = rtx_to_list(rtx, fi);

	ret = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id);
	if (ret <= -1)
	{
		list->errnum = qse_awk_rtx_geterrnum(rtx);
		ret = -1;
	}
	else if (!(node = get_list_node(list, id)))
	{
/* TODO: enhance error */
		list->errnum = QSE_AWK_EINVAL;
		ret = -1;
	}
	else
	{
		free_sql_node (rtx, list, node);
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_connect (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* list;
	sql_node_t* node;
	qse_awk_int_t id;
	int ret;

	list = rtx_to_list(rtx, fi);

	ret = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id);
	if (ret <= -1)
	{
		list->errnum = qse_awk_rtx_geterrnum(rtx);
		ret = -1;
	}
	else if (!(node = get_list_node(list, id)))
	{
/* TODO: enhance error */
		list->errnum = QSE_AWK_EINVAL;
		ret = -1;
	}
	else
	{
		if (!mysql_real_connect(node->mysql, QSE_NULL, QSE_NULL, QSE_NULL, QSE_NULL, 0, QSE_NULL, 0))
		{
/* TODO: capture error message... */
			list->errnum = QSE_AWK_ESYSERR;
			ret = -1;
		}
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
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
	{ QSE_T("connect"),      { { 4, 1, QSE_NULL },   fnc_connect,   0 } },
	{ QSE_T("open"),         { { 0, 0, QSE_NULL },   fnc_open,      0 } },
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
	qse_rbt_t* rbt;
	sql_list_t list;

	rbt = (qse_rbt_t*)mod->ctx;

	QSE_MEMSET (&list, 0, QSE_SIZEOF(list));
	if (qse_rbt_insert (rbt, &rtx, QSE_SIZEOF(rtx), &list, QSE_SIZEOF(list)) == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}

	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	qse_rbt_t* rbt;
	qse_rbt_pair_t* pair;
	
	rbt = (qse_rbt_t*)mod->ctx;

	/* garbage clean-up */
	pair = qse_rbt_search(rbt, &rtx, QSE_SIZEOF(rtx));
	if (pair)
	{
		sql_list_t* list;
		sql_node_t* node, * next;

		list = QSE_RBT_VPTR(pair);
		node = list->head;
		while (node)
		{
			next = node->next;
			free_sql_node (rtx, list, node);
			node = next;
		}

		qse_rbt_delete (rbt, &rtx, QSE_SIZEOF(rtx));
	}
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_rbt_t* rbt;

	rbt = (qse_rbt_t*)mod->ctx;

	QSE_ASSERT (QSE_RBT_SIZE(rbt) == 0);
	qse_rbt_close (rbt);
}

int qse_awk_mod_mysql (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_rbt_t* rbt;

	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;

	rbt = qse_rbt_open(qse_awk_getmmgr(awk), 0, 1, 1);
	if (rbt == QSE_NULL) 
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	qse_rbt_setstyle (rbt, qse_getrbtstyle(QSE_RBT_STYLE_INLINE_COPIERS));

	mod->ctx = rbt;
	return 0;
}


QSE_EXPORT int qse_awk_mod_mysql_init (int argc, qse_achar_t* argv[])
{
	if (mysql_library_init(argc, argv, QSE_NULL) != 0) return -1;
	return 0;
}

QSE_EXPORT void qse_awk_mod_mysql_fini (void)
{
	mysql_library_end ();
}
