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

#undef __IMAP_NODE_T_DATA
#undef __IMAP_LIST_T_DATA
#undef __IMAP_LIST_T
#undef __IMAP_NODE_T
#undef __MAKE_IMAP_NODE
#undef __FREE_IMAP_NODE

#define __IMAP_NODE_T_DATA  MYSQL_RES* res; unsigned int num_fields;
#define __IMAP_LIST_T_DATA  int errnum;
#define __IMAP_LIST_T res_list_t
#define __IMAP_NODE_T res_node_t
#define __MAKE_IMAP_NODE __new_res_node
#define __FREE_IMAP_NODE __free_res_node
#include "../lib/awk/imap-imp.h"

struct rtx_data_t
{
	sql_list_t sql_list;
	res_list_t res_list;
};
typedef struct rtx_data_t rtx_data_t;

static sql_node_t* new_sql_node (qse_awk_rtx_t* rtx, sql_list_t* sql_list)
{
	sql_node_t* sql_node;

	sql_node = __new_sql_node(rtx, sql_list);
	if (!sql_node) return QSE_NULL;

	sql_node->mysql = mysql_init(QSE_NULL);
	if (!sql_node->mysql)
	{
		qse_awk_rtx_seterrfmt (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_T("unable to allocate a mysql object"));
		return QSE_NULL;
	}

	return sql_node;
}

static void free_sql_node (qse_awk_rtx_t* rtx, sql_list_t* sql_list, sql_node_t* sql_node)
{
	mysql_close (sql_node->mysql);
	sql_node->mysql = QSE_NULL;
	__free_sql_node (rtx, sql_list, sql_node);
}


static res_node_t* new_res_node (qse_awk_rtx_t* rtx, res_list_t* res_list, MYSQL_RES* res)
{
	res_node_t* res_node;

	res_node = __new_res_node(rtx, res_list);
	if (!res_node) return QSE_NULL;

	res_node->res = res;
	res_node->num_fields = mysql_num_fields(res);

	return res_node;
}

static void free_res_node (qse_awk_rtx_t* rtx, res_list_t* res_list, res_node_t* res_node)
{
	mysql_free_result (res_node->res);
	res_node->res = QSE_NULL;
	__free_res_node (rtx, res_list, res_node);
}

/* ------------------------------------------------------------------------ */

static QSE_INLINE sql_list_t* rtx_to_sql_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	rtx_data_t* data;
	pair = qse_rbt_search((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	data = (rtx_data_t*)QSE_RBT_VPTR(pair);
	return &data->sql_list;
}

static QSE_INLINE sql_node_t* get_sql_list_node (sql_list_t* sql_list, qse_awk_int_t id)
{
	if (id < 0 || id >= sql_list->map.high || !sql_list->map.tab[id]) return QSE_NULL;
	return sql_list->map.tab[id];
}

static QSE_INLINE res_list_t* rtx_to_res_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	rtx_data_t* data;
	pair = qse_rbt_search((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	data = (rtx_data_t*)QSE_RBT_VPTR(pair);
	return &data->res_list;
}

static QSE_INLINE res_node_t* get_res_list_node (res_list_t* res_list, qse_awk_int_t id)
{
	if (id < 0 || id >= res_list->map.high || !res_list->map.tab[id]) return QSE_NULL;
	return res_list->map.tab[id];
}

/* ------------------------------------------------------------------------ */

static int fnc_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* sql_list;
	sql_node_t* sql_node = QSE_NULL;
	qse_awk_int_t ret;
	qse_awk_val_t* retv;

	sql_list = rtx_to_sql_list(rtx, fi);

	sql_node = new_sql_node(rtx, sql_list);
	if (sql_node) ret = sql_node->id;
	else ret = -1;

	/* ret may not be a statically managed number. 
	 * error checking is required */
	retv = qse_awk_rtx_makeintval(rtx, ret);
	if (retv == QSE_NULL)
	{
		if (sql_node) free_sql_node (rtx, sql_list, sql_node);
		return -1;
	}

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* sql_list;
	sql_node_t* sql_node;
	qse_awk_int_t id;
	int ret;

	sql_list = rtx_to_sql_list(rtx, fi);

	ret = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id);
	if (ret <= -1)
	{
		sql_list->errnum = qse_awk_rtx_geterrnum(rtx);
		ret = -1;
	}
	else if (!(sql_node = get_sql_list_node(sql_list, id)))
	{
/* TODO: enhance error */
		sql_list->errnum = QSE_AWK_EINVAL;
		ret = -1;
	}
	else
	{
		free_sql_node (rtx, sql_list, sql_node);
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_connect (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* sql_list;
	sql_node_t* sql_node;
	qse_awk_int_t id;
	int ret = -1;

	qse_awk_val_t* a1, * a2, * a3;
	qse_mchar_t* host = QSE_NULL;
	qse_mchar_t* user = QSE_NULL;
	qse_mchar_t* pass = QSE_NULL;

	sql_list = rtx_to_sql_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id) <= -1)
	{
		sql_list->errnum = qse_awk_rtx_geterrnum(rtx);
	}
	else if (!(sql_node = get_sql_list_node(sql_list, id)))
	{
/* TODO: enhance error */
		sql_list->errnum = QSE_AWK_EINVAL;
	}
	else
	{
		a1 = qse_awk_rtx_getarg(rtx, 1);
		a2 = qse_awk_rtx_getarg(rtx, 2);
		a3 = qse_awk_rtx_getarg(rtx, 3);

		host = qse_awk_rtx_getvalmbs(rtx, a1, QSE_NULL);
		if (!host) goto done; /* TODO: set sql_list->errnum ... */
		user = qse_awk_rtx_getvalmbs(rtx, a2, QSE_NULL);
		if (!user) goto done;
		pass = qse_awk_rtx_getvalmbs(rtx, a3, QSE_NULL);
		if (!pass) goto done;

		if (!mysql_real_connect(sql_node->mysql, host, user, pass, QSE_NULL, 0, QSE_NULL, 0))
		{
/* TODO: capture error message... */
			sql_list->errnum = QSE_AWK_ESYSERR;
			goto done;
		}

		ret = 0;
	}

done:
	if (pass) qse_awk_rtx_freevalmbs (rtx, a3, pass);
	if (user) qse_awk_rtx_freevalmbs (rtx, a2, user);
	if (host) qse_awk_rtx_freevalmbs (rtx, a1, host);
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_query (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* sql_list;
	sql_node_t* sql_node;
	qse_awk_int_t id;
	int ret = -1;

	qse_awk_val_t* a1;
	qse_mchar_t* qstr = QSE_NULL;

	sql_list = rtx_to_sql_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id) <= -1)
	{
		sql_list->errnum = qse_awk_rtx_geterrnum(rtx);
	}
	else if (!(sql_node = get_sql_list_node(sql_list, id)))
	{
/* TODO: enhance error */
		sql_list->errnum = QSE_AWK_EINVAL;
	}
	else
	{
		qse_size_t qlen;
		a1 = qse_awk_rtx_getarg(rtx, 1);

		qstr = qse_awk_rtx_getvalmbs(rtx, a1, &qlen);
		if (!qstr) goto done; /* TODO: set sql_list->errnum ... */

		if (mysql_real_query(sql_node->mysql, qstr, qlen) != 0)
		{
/* TODO: capture error message... */
			sql_list->errnum = QSE_AWK_ESYSERR;
			goto done;
		}

		ret = 0;
	}

done:
	if (qstr) qse_awk_rtx_freevalmbs (rtx, a1, qstr);
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_store_result (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sql_list_t* sql_list;
	sql_node_t* sql_node;
	qse_awk_int_t id;
	int ret = -1;

	sql_list = rtx_to_sql_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id) <= -1)
	{
		sql_list->errnum = qse_awk_rtx_geterrnum(rtx);
	}
	else if (!(sql_node = get_sql_list_node(sql_list, id)))
	{
/* TODO: enhance error */
		sql_list->errnum = QSE_AWK_EINVAL;
	}
	else
	{
		res_list_t* res_list;
		res_node_t* res_node;
		MYSQL_RES* res;

		res_list = rtx_to_res_list(rtx, fi);

		res = mysql_store_result(sql_node->mysql);
		if (!res)
		{
/* TODO: capture error message... */
			sql_list->errnum = QSE_AWK_ESYSERR;
			goto done;
		}

		res_node = new_res_node(rtx, res_list, res);
		if (!res_node)	
		{
			mysql_free_result (res);
			goto done;
		}

		ret = res_node->id;
	}

done:
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_free_result (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	res_list_t* res_list;
	res_node_t* res_node;
	qse_awk_int_t id;
	int ret = -1;

	res_list = rtx_to_res_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id) <= -1)
	{
		res_list->errnum = qse_awk_rtx_geterrnum(rtx);
	}
	else if (!(res_node = get_res_list_node(res_list, id)))
	{
/* TODO: enhance error */
		res_list->errnum = QSE_AWK_EINVAL;
	}
	else
	{
		free_res_node (rtx, res_list, res_node);
		ret = 0;
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_fetch_row (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	res_list_t* res_list;
	res_node_t* res_node;
	qse_awk_int_t id;
	int ret = -1;

	res_list = rtx_to_res_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id) <= -1)
	{
		res_list->errnum = qse_awk_rtx_geterrnum(rtx);
	}
	else if (!(res_node = get_res_list_node(res_list, id)))
	{
/* TODO: enhance error */
		res_list->errnum = QSE_AWK_EINVAL;
	}
	else
	{
		MYSQL_ROW row;
		unsigned int i;
		qse_awk_val_t* row_map, * row_val;
		int x;

		row = mysql_fetch_row(res_node->res);
		if (!row)
		{
			ret = 0;
			goto done;
		}

		row_map = qse_awk_rtx_makemapval(rtx);
		if (!row_map) 
		{
			/* TODO set erro rmessage */
			goto done;
		}

		qse_awk_rtx_refupval (rtx, row_map);
		x = qse_awk_rtx_setrefval(rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 1), row_map);
		qse_awk_rtx_refdownval (rtx, row_map);
		if (x <= -1)
		{
			/* TODO set erro rmessage */
			goto done;
		}

		for (i = 0; i < res_node->num_fields; )
		{
			qse_char_t key_buf[QSE_SIZEOF(qse_awk_int_t) * 8 + 2];
			qse_size_t key_len;

			if (row[i])
			{
/* TODO: consider using make multi byte string - qse_awk_rtx_makembsstr */
				row_val = qse_awk_rtx_makestrvalwithmbs(rtx, row[i]);
				if (!row_val)
				{
					/* TODO set erro rmessage */
					goto done;
				}
			}
			else
			{
				row_val = qse_awk_rtx_makenilval(rtx);
			}

			++i;

			/* put it into the map */
			key_len = qse_awk_inttostr(qse_awk_rtx_getawk(rtx), i, 10, QSE_NULL, key_buf, QSE_COUNTOF(key_buf));
			QSE_ASSERT (key_len != (qse_size_t)-1);

			if (qse_awk_rtx_setmapvalfld(rtx, row_map, key_buf, key_len, row_val) == QSE_NULL)
			{
				qse_awk_rtx_refupval (rtx, row_val);
				qse_awk_rtx_refdownval (rtx, row_val);
			/* TODO: set error message */
				goto done;
			}
		}

		ret = 1;
	}

done:
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
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
	{ QSE_T("close"),        { { 1, 1, QSE_NULL },     fnc_close,          0 } },
	{ QSE_T("connect"),      { { 4, 8, QSE_NULL },     fnc_connect,        0 } },
	{ QSE_T("fetch_row"),    { { 2, 2, QSE_T("vr") },  fnc_fetch_row,      0 } },
	{ QSE_T("free_result"),  { { 1, 1, QSE_NULL },     fnc_free_result,    0 } },
	{ QSE_T("open"),         { { 0, 0, QSE_NULL },     fnc_open,           0 } },
	{ QSE_T("query"),        { { 2, 2, QSE_NULL },     fnc_query,          0 } },
	{ QSE_T("store_result"), { { 1, 1, QSE_NULL },     fnc_store_result,   0 } }
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
	rtx_data_t data;

	rbt = (qse_rbt_t*)mod->ctx;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	if (qse_rbt_insert (rbt, &rtx, QSE_SIZEOF(rtx), &data, QSE_SIZEOF(data)) == QSE_NULL) 
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
		rtx_data_t* data;
		sql_node_t* sql_node, * sql_next;
		res_node_t* res_node, * res_next;

		data = (rtx_data_t*)QSE_RBT_VPTR(pair);

		res_node = data->res_list.head;
		while (sql_node)
		{
			res_next = res_node->next;
			free_res_node (rtx, &data->res_list, res_node);
			res_node = res_next;
		}

		sql_node = data->sql_list.head;
		while (sql_node)
		{
			sql_next = sql_node->next;
			free_sql_node (rtx, &data->sql_list, sql_node);
			sql_node = sql_next;
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
