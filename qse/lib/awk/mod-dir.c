/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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


/*
BEGIN {
	x = dir::open ("/etc", dir::SORT); # dir::open ("/etc", 0)
	if (x <= -1) {
		print "cannot open";
		return -1;
	}

	while (dir::read(x, q) > 0) {
		print q;
	}
	dir::close (x);
}
*/

#include "mod-dir.h"
#include <qse/si/dir.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include "../cmn/mem-prv.h"

enum
{
	DIR_ENOERR,
	DIR_EOTHER,
	DIR_ESYSERR,
	DIR_ENOMEM,
	DIR_EINVAL,
	DIR_EACCES,
	DIR_EPERM,
	DIR_ENOENT,
	DIR_EMAPTOSCALAR
};

static int dir_err_to_errnum (qse_dir_errnum_t num)
{
	switch (num)
	{
		case QSE_DIR_ESYSERR:
			return DIR_ESYSERR;
		case QSE_DIR_ENOMEM:
			return DIR_ENOMEM;
		case QSE_DIR_EINVAL:
			return DIR_EINVAL;
		case QSE_DIR_EACCES:
			return DIR_EACCES;
		case QSE_DIR_EPERM:
			return DIR_EPERM;
		case QSE_DIR_ENOENT:
			return DIR_ENOENT;
		default:
			return DIR_EOTHER;
	}
}

static int awk_err_to_errnum (qse_awk_errnum_t num)
{
	switch (num)
	{
		case QSE_AWK_ESYSERR:
			return DIR_ESYSERR;
		case QSE_AWK_ENOMEM:
			return DIR_ENOMEM;
		case QSE_AWK_EINVAL:
			return DIR_EINVAL;
		case QSE_AWK_EACCES:
			return DIR_EACCES;
		case QSE_AWK_EPERM:
			return DIR_EPERM;
		case QSE_AWK_ENOENT:
			return DIR_ENOENT;
		case QSE_AWK_EMAPTOSCALAR:
			return DIR_EMAPTOSCALAR;
		default:
			return DIR_EOTHER;
	}
}

#define __IMAP_NODE_T_DATA qse_dir_t* ctx;
#define __IMAP_LIST_T_DATA int errnum;
#define __IMAP_LIST_T dir_list_t
#define __IMAP_NODE_T dir_node_t
#define __MAKE_IMAP_NODE __new_dir_node
#define __FREE_IMAP_NODE __free_dir_node
#include "imap-imp.h"

static dir_node_t* new_dir_node (qse_awk_rtx_t* rtx, dir_list_t* list, const qse_char_t* path, qse_awk_int_t flags)
{
	dir_node_t* node;
	qse_dir_errnum_t oe;

	node = __new_dir_node(rtx, list);
	if (!node) 
	{
		list->errnum = DIR_ENOMEM;
		return QSE_NULL;
	}

	node->ctx = qse_dir_open(qse_awk_rtx_getmmgr(rtx), 0, path, flags, &oe);
	if (!node->ctx) 
	{
		list->errnum = dir_err_to_errnum(oe);
		__free_dir_node (rtx, list, node);
		return QSE_NULL;
	}

	return node;
}

static void free_dir_node (qse_awk_rtx_t* rtx, dir_list_t* list, dir_node_t* node)
{
	if (node->ctx) 
	{
		qse_dir_close(node->ctx);
		node->ctx = QSE_NULL;
	}
	__free_dir_node (rtx, list, node);
}
/* ------------------------------------------------------------------------ */

static int close_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_awk_int_t id)
{
	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		free_dir_node (rtx, list, list->map.tab[id]);
		return 0;
	}
	else
	{
		list->errnum = DIR_EINVAL;
		return -1;
	}
}

static int reset_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_awk_int_t id, const qse_char_t* path)
{
	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		if (qse_dir_reset(list->map.tab[id]->ctx, path) <= -1)
		{
			list->errnum = dir_err_to_errnum (qse_dir_geterrnum (list->map.tab[id]->ctx));
			return -1;
		}
		return 0;
	}
	else
	{
		list->errnum = DIR_EINVAL;
		return -1;
	}
}

static int read_byid (qse_awk_rtx_t* rtx, dir_list_t* list, qse_awk_int_t id, qse_awk_val_ref_t* ref) 
{
	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		int y;
		qse_dir_ent_t ent;	
		qse_awk_val_t* tmp;

		y = qse_dir_read(list->map.tab[id]->ctx, &ent);
		if (y <= -1) 
		{
			list->errnum = dir_err_to_errnum(qse_dir_geterrnum (list->map.tab[id]->ctx));
			return -1;
		}

		if (y == 0) return 0; /* no more entry */

		tmp = qse_awk_rtx_makestrvalwithstr(rtx, ent.name);
		if (!tmp)
		{
			list->errnum = awk_err_to_errnum(qse_awk_rtx_geterrnum (rtx));
			return -1;
		}
		else
		{
			int n;
			qse_awk_rtx_refupval (rtx, tmp);
			n = qse_awk_rtx_setrefval (rtx, ref, tmp);
			qse_awk_rtx_refdownval (rtx, tmp);
			if (n <= -1) return -9999;
		}

		return 1; /* has entry */
	}
	else
	{
		list->errnum = DIR_EINVAL;
		return -1;
	}
}

/* ------------------------------------------------------------------------ */

static QSE_INLINE dir_list_t* rtx_to_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	pair = qse_rbt_search((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (dir_list_t*)QSE_RBT_VPTR(pair);
}

static int fnc_dir_errno (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;

	list = rtx_to_list(rtx, fi);

	retv = qse_awk_rtx_makeintval (rtx, list->errnum);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static qse_char_t* errmsg[] =
{
	QSE_T("no error"),
	QSE_T("other error"),
	QSE_T("system error"),
	QSE_T("insufficient memory"),
	QSE_T("invalid data"),
	QSE_T("access denied"),
	QSE_T("operation not permitted"),
	QSE_T("no entry"),
	QSE_T("cannot change a map to a scalar"),
	QSE_T("unknown error")
};

static int fnc_dir_errstr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_val_t* retv;
	qse_awk_int_t errnum;

	list = rtx_to_list(rtx, fi);

	if (qse_awk_rtx_getnargs(rtx) <= 0 ||
	    qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg (rtx, 0), &errnum) <= -1)
	{
		errnum = list->errnum;
	}

	if (errnum < 0 || errnum >= QSE_COUNTOF(errmsg)) errnum = QSE_COUNTOF(errmsg) - 1;

	retv = qse_awk_rtx_makestrvalwithstr(rtx, errmsg[errnum]);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	dir_node_t* node = QSE_NULL;
	qse_awk_int_t ret;
	qse_char_t* path;
	qse_awk_val_t* retv;
	qse_awk_val_t* a0;
	qse_awk_int_t flags = 0;

	list = rtx_to_list(rtx, fi);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	path = qse_awk_rtx_getvalstr(rtx, a0, QSE_NULL);
	if (path)
	{
		if (qse_awk_rtx_getnargs(rtx) >= 2 && 
		    qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &flags) <= -1)
		{
			qse_awk_rtx_freevalstr (rtx, a0, path);
			goto oops;
		}

		node = new_dir_node(rtx, list, path, flags);
		if (node) ret = node->id;
		else ret = -1;
		qse_awk_rtx_freevalstr (rtx, a0, path);
	}
	else
	{
	oops:
		list->errnum = awk_err_to_errnum(qse_awk_rtx_geterrnum(rtx));
		ret = -1;
	}

	/* ret may not be a statically managed number. 
	 * error checking is required */
	retv = qse_awk_rtx_makeintval(rtx, ret);
	if (retv == QSE_NULL)
	{
		if (node) free_dir_node (rtx, list, node);
		return -1;
	}

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_dir_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_int_t id;
	int ret;

	list = rtx_to_list(rtx, fi);

	ret = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &id);
	if (ret <= -1)
	{
		list->errnum = awk_err_to_errnum(qse_awk_rtx_geterrnum(rtx));
		ret = -1;
	}
	else
	{
		ret = close_byid(rtx, list, id);
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_dir_reset (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_int_t id;
	int ret;
	qse_char_t* path;
	
	list = rtx_to_list(rtx, fi);

	ret = qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1)
	{
		list->errnum = awk_err_to_errnum (qse_awk_rtx_geterrnum (rtx));
	}
	else
	{
		qse_awk_val_t* a1;

		a1 = qse_awk_rtx_getarg (rtx, 1);
		path = qse_awk_rtx_getvalstr (rtx, a1, QSE_NULL);
		if (path)
		{
			ret = reset_byid (rtx, list, id, path);
			qse_awk_rtx_freevalstr (rtx, a1, path);
		}
		else
		{
			list->errnum = awk_err_to_errnum (qse_awk_rtx_geterrnum (rtx));
			ret = -1;
		}
	}

	/* no error check for qse_awk_rtx_makeintval() here since ret 
	 * is 0 or -1. it will never fail for those numbers */
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval (rtx, ret));
	return 0;
}

static int fnc_dir_read  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	dir_list_t* list;
	qse_awk_int_t id;
	int ret;

	list = rtx_to_list(rtx, fi);

	ret = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) 
	{
		list->errnum = awk_err_to_errnum(qse_awk_rtx_geterrnum (rtx));
	}
	else
	{
		ret = read_byid(rtx, list, id, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 1));
		if (ret == -9999) return -1;
	}

	/* no error check for qse_awk_rtx_makeintval() here since ret 
	 * is 0, 1, -1. it will never fail for those numbers */
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_int_t info;
};


static fnctab_t fnctab[] =
{
	{ QSE_T("close"),       { { 1, 1, QSE_NULL    }, fnc_dir_close,      0 } },
	{ QSE_T("errno"),       { { 0, 0, QSE_NULL    }, fnc_dir_errno,      0 } },
	{ QSE_T("errstr"),      { { 0, 1, QSE_NULL    }, fnc_dir_errstr,     0 } },
	{ QSE_T("open"),        { { 1, 2, QSE_NULL    }, fnc_dir_open,       0 } },
	{ QSE_T("read"),        { { 2, 2, QSE_T("vr") }, fnc_dir_read,       0 } },
	{ QSE_T("reset"),       { { 2, 2, QSE_NULL    }, fnc_dir_reset,      0 } },
};

static inttab_t inttab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("SORT"), { QSE_DIR_SORT } }
};

/* ------------------------------------------------------------------------ */

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

	ea.ptr = (qse_char_t*)name;
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
	pair = qse_rbt_search(rbt, &rtx, QSE_SIZEOF(rtx));
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

int qse_awk_mod_dir (qse_awk_mod_t* mod, qse_awk_t* awk) 
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

       
