/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/awk/awk.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include <qse/cmn/dir.h>
#include "../../lib/cmn/mem.h"

typedef struct dir_list_t dir_list_t;
typedef struct dir_node_t dir_node_t;

enum
{
	DIR_ENOERR,
	DIR_ENOMEM,
	DIR_EINVAL,
	DIR_ENOENT
};

struct dir_node_t
{
	int id;
	qse_dir_t* ctx;
	dir_node_t* prev;
	dir_node_t* next;
};

struct dir_list_t
{
	dir_node_t* head;
	dir_node_t* tail;
	dir_node_t* free;
	
	/* mapping table to map 'id' to 'node' */
	struct
	{
		dir_node_t** tab;
		int capa;
		int high;
	} map;

	int errnum;
};

static dir_node_t* new_dir_node (qse_awk_rtx_t* rtx, dir_list_t* list, const qse_char_t* path)
{
	/* create a new context node and append it to the list tail */
	dir_node_t* node;

	node = QSE_NULL;

	if (list->free) node = list->free;
	else
	{
		node = qse_awk_rtx_callocmem (rtx, QSE_SIZEOF(*node));
		if (!node) goto oops;
	}

	node->ctx = qse_dir_open (qse_awk_rtx_getmmgr(rtx), 0, path, 0);
	if (!node->ctx) goto oops;

	if (node == list->free) list->free = node->next;
	else
	{
		if (list->map.high <= list->map.capa)
		{
			int newcapa;
			dir_node_t** tmp;

			newcapa = list->map.capa + 64;
			if (newcapa < list->map.capa) goto oops; /* overflow */

			tmp = (dir_node_t**) qse_awk_rtx_reallocmem (
				rtx, list->map.tab, QSE_SIZEOF(*tmp) * newcapa);
			if (!tmp) goto oops;

			QSE_MEMSET (&tmp[list->map.capa], 0, 
				QSE_SIZEOF(*tmp) * (newcapa - list->map.capa));

			list->map.tab = tmp;
			list->map.capa = newcapa;
		}

		node->id = list->map.high;
		QSE_ASSERT (list->map.tab[node->id] == QSE_NULL);
		list->map.tab[node->id] = node;
		list->map.high++;
	}

	/* append it to the tail */
	node->next = QSE_NULL;
	node->prev = list->tail;
	if (list->tail) list->tail->next = node;
	else list->head = node;
	list->tail = node;

	return node;

oops:
	if (node) qse_awk_rtx_freemem (rtx, node);
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return QSE_NULL;
}

static void free_dir_node (qse_awk_rtx_t* rtx, dir_list_t* list, dir_node_t* node)
{
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;
	if (list->head == node) list->head = node->next;
	if (list->tail == node) list->tail = node->prev;

	list->map.tab[node->id] = QSE_NULL;

	if (node->ctx) 
	{
		qse_dir_close (node->ctx);
	}

	if (list->map.high == node->id + 1)
	{
		/* destroy the actual node if the node to be freed has the
		 * highest id */
		QSE_MMGR_FREE (qse_awk_rtx_getmmgr(rtx), node);
		list->map.high--;
	}
	else
	{
		/* otherwise, chain the node to the free list */
		node->ctx = QSE_NULL;
		node->next = list->free;
		list->free = node;
	}

	/* however, i destroy the whole free list when all the nodes are
	 * chanined to the free list */
	if (list->head == QSE_NULL) 
	{
		qse_mmgr_t* mmgr;
		dir_node_t* curnode;

		mmgr = qse_awk_rtx_getmmgr(rtx);

		while (list->free)
		{
			curnode = list->free;
			list->free = list->free->next;
			QSE_ASSERT (curnode->ctx == QSE_NULL);
			QSE_MMGR_FREE (mmgr, curnode);
		}

		QSE_MMGR_FREE (mmgr, list->map.tab);
		list->map.high = 0;
		list->map.capa = 0;
		list->map.tab = QSE_NULL;
	}
}

/* ------------------------------------------------------------------------ */

static int close_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_long_t id)
{
	int x = DIR_EINVAL;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		free_dir_node (rtx, list, list->map.tab[id]);
		x = DIR_ENOERR;
	}
	
	return -x;
}

static int reset_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_long_t id, const qse_char_t* path)
{
	int x = DIR_EINVAL;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		if (qse_dir_reset (list->map.tab[id]->ctx, path) >= 0) x = DIR_ENOERR;
	}
	
	return -x;
}

static int read_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_long_t id, qse_awk_val_t** retv)
{
	int x = DIR_EINVAL;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		int y;
		qse_dir_ent_t ent;	
		y = qse_dir_read (list->map.tab[id]->ctx, &ent);
		if (y == 0) x = DIR_ENOENT;
		else if (y >= 1)
		{
			qse_awk_val_t* tmp;
			tmp = qse_awk_rtx_makestrvalwithstr (rtx, ent.name);	
			if (tmp) 
			{
				*retv = tmp;
				x = DIR_ENOERR;
			}
			else x = DIR_ENOMEM;
		}
	}
	
	return -x;
}

/* ------------------------------------------------------------------------ */

static QSE_INLINE dir_list_t* rtx_to_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	pair = qse_rbt_search ((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (dir_list_t*)QSE_RBT_VPTR(pair);
}

static int fnc_dir_errno (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;

	list = rtx_to_list (rtx, fi);

	retv = qse_awk_rtx_makeintval (rtx, list->errnum);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static qse_char_t* errmsg[] =
{
	QSE_T("no error"),
	QSE_T("out of memory"),
	QSE_T("invalid data"),
	QSE_T("no entry"),
	QSE_T("unknown error")
};

static int fnc_dir_errstr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t errnum;

	list = rtx_to_list (rtx, fi);

	if (qse_awk_rtx_getnargs (rtx) <= 0 ||
	    qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &errnum) <= -1)
	{
		errnum = list->errnum;
	}

	if (errnum < 0 || errnum >= QSE_COUNTOF(errmsg)) errnum = QSE_COUNTOF(errmsg) - 1;

	retv = qse_awk_rtx_makestrvalwithstr (rtx, errmsg[errnum]);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	dir_node_t* node;
	qse_long_t ret;
	qse_awk_val_t* retv;
	qse_char_t* path;

	list = rtx_to_list (rtx, fi);

	path = qse_awk_rtx_valtostrdup (rtx, qse_awk_rtx_getarg (rtx, 0), QSE_NULL);
	if (path == QSE_NULL)
	{
		list->errnum = DIR_ENOMEM;
		ret = -DIR_ENOMEM;
	}
	else
	{
		node = new_dir_node (rtx, list, path);
		ret = node? node->id: -DIR_ENOMEM;
		qse_awk_rtx_freemem (rtx, path);
	}
	
	if (ret <= -1) 
	{
		list->errnum = -ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -DIR_EINVAL;
	else ret = close_byid (rtx, list, id);

	if (ret <= -1) 
	{
		list->errnum = -ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_reset (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -DIR_EINVAL;
	else
	{
		qse_char_t* path;

		path = qse_awk_rtx_valtostrdup (rtx, qse_awk_rtx_getarg(rtx, 1), QSE_NULL);
		if (path)
		{
			ret = reset_byid (rtx, list, id, path);
			qse_awk_rtx_freemem (rtx, path);
		}
		else ret = -DIR_ENOMEM;
	}

	if (ret <= -1) 
	{
		list->errnum = -ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_read  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -DIR_EINVAL;
	else ret = read_byid (rtx, list, id, &retv);

	if (ret <= -1) 
	{
		list->errnum = -ret;
	}
	else
	{
		qse_awk_rtx_setretval (rtx, retv);
	}

	return 0;
}

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	{ QSE_T("close"),       { { 1, 1, QSE_NULL }, fnc_dir_close,      0 } },
	{ QSE_T("errno"),       { { 0, 0, QSE_NULL }, fnc_dir_errno,      0 } },
	{ QSE_T("errstr"),      { { 0, 1, QSE_NULL }, fnc_dir_errstr,     0 } },
	{ QSE_T("open"),        { { 1, 1, QSE_NULL }, fnc_dir_open,       0 } },
	{ QSE_T("read"),        { { 1, 1, QSE_NULL }, fnc_dir_read,       0 } },
	{ QSE_T("reset"),       { { 2, 2, QSE_NULL }, fnc_dir_reset,      0 } },
};

/* ------------------------------------------------------------------------ */

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
     int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

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

	ea.ptr = name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	qse_rbt_t* rbt;
	dir_list_t list;

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
	pair = qse_rbt_search  (rbt, &rtx, QSE_SIZEOF(rtx));
	if (pair)
	{
		dir_list_t* list;
		dir_node_t* node, * next;

		list = QSE_RBT_VPTR(pair);
		node = list->head;
		while (node)
		{
			next = node->next;
			free_dir_node (rtx, list, node);
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

QSE_EXPORT int load (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_rbt_t* rbt;

	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;

	rbt = qse_rbt_open (qse_awk_getmmgr(awk), 0, 1, 1);
	if (rbt == QSE_NULL) 
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	qse_rbt_setmancbs (rbt, qse_getrbtmancbs(QSE_RBT_MANCBS_INLINE_COPIERS));

	mod->ctx = rbt;
	return 0;
}

#if defined(__DOS__)
/* kind of DllMain() for Causeway DLL */
int main (int eax) { return 0; }
#endif
       
