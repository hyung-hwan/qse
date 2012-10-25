#include <qse/awk/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include "../../lib/cmn/mem.h"

#if defined(HAVE_UCI_H)
#	include <uci.h>
#else
#	error this module needs uci.h
#endif

typedef struct uctx_list_t uctx_list_t;
typedef struct uctx_node_t uctx_node_t;

struct uctx_node_t
{
	int id;
	struct uci_context* ctx;	
	uctx_node_t* prev;
	uctx_node_t* next;
};

struct uctx_list_t
{
	uctx_node_t* head;
	uctx_node_t* tail;
	uctx_node_t* free;
	
	/* mapping table to map 'id' to 'node' */
	struct
	{
		uctx_node_t** tab;
		int capa;
		int high;
	} map;
};

static uctx_node_t* new_uctx_node (qse_awk_rtx_t* rtx, uctx_list_t* list)
{
	/* create a new context node and append it to the list tail */
	uctx_node_t* node;
	qse_mmgr_t* mmgr;

	node = QSE_NULL;
	mmgr = qse_awk_rtx_getmmgr(rtx);

	if (list->free) node = list->free;
	else
	{
		node = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*node));
		if (!node) goto oops;
	}

	node->ctx = uci_alloc_context();
	if (!node->ctx) goto oops;

	if (node == list->free) list->free = node->next;
	else
	{
		if (list->map.high <= list->map.capa)
		{
			int newcapa;
			uctx_node_t** tmp;

			newcapa = list->map.capa + 64;
			if (newcapa < list->map.capa) goto oops; /* overflow */

			tmp = (uctx_node_t**) QSE_MMGR_REALLOC (
				mmgr, list->map.tab, QSE_SIZEOF(*tmp) * newcapa);
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
	if (node) QSE_MMGR_FREE (mmgr, node);
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return QSE_NULL;
}

static void free_uctx_node (qse_awk_rtx_t* rtx, uctx_list_t* list, uctx_node_t* node)
{
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;
	if (list->head == node) list->head = node->next;
	if (list->tail == node) list->tail = node->prev;

	list->map.tab[node->id] = QSE_NULL;

	if (node->ctx) uci_free_context (node->ctx);

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
		uctx_node_t* curnode;

		mmgr = qse_awk_rtx_getmmgr(rtx);

		while (list->free)
		{
			curnode = list->free;
			list->free = list->free->next;
			QSE_ASSERT (curnode->ctx == QSE_NULL);
			QSE_MMGR_FREE (mmgr, curnode);
		}

qse_printf (QSE_T("freeing map...\n"));
		QSE_MMGR_FREE (mmgr, list->map.tab);
		list->map.high = 0;
		list->map.capa = 0;
		list->map.tab = QSE_NULL;
	}
}

static int free_uctx_node_byid (qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id)
{
	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		free_uctx_node (rtx, list, list->map.tab[id]);
		return 0;
	}
	
	return -1;
}

static QSE_INLINE uctx_list_t* rtx_to_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	pair = qse_rbt_search ((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (uctx_list_t*)QSE_RBT_VPTR(pair);
}

static int fnc_uci_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	uctx_node_t* node;
	qse_long_t ret = -1;
	qse_awk_val_t* retv;

	list = rtx_to_list (rtx, fi);
	node = new_uctx_node (rtx, list);
	if (node) ret = node->id;
	
	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;

}

static int fnc_uci_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int n;
	
	list = rtx_to_list (rtx, fi);

	n = qse_awk_rtx_valtolong (
		rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (n <= -1) return -1;

	retv = qse_awk_rtx_makeintval (
		rtx, free_uctx_node_byid (rtx, list, id));
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_load  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return 0 ;
}

static int fnc_uci_save  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return 0 ;
}

static int fnc_uci_commit (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	//uci_commit ();
	return 0 ;
}

static int fnc_uci_revert (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	//uci_revert ();
	return 0;
}


typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	{ QSE_T("close"),   { { 1, 1 }, fnc_uci_close } },
	{ QSE_T("commit"),  { { 0, 0 }, fnc_uci_commit  } },
	{ QSE_T("load"),    { { 1, 1 }, fnc_uci_load  } },
	{ QSE_T("open"),    { { 0, 0 }, fnc_uci_open  } },
	{ QSE_T("revert"),  { { 0, 0 }, fnc_uci_revert  } },
	{ QSE_T("save"),    { { 0, 0 }, fnc_uci_save  } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int i;

/* TODO: binary search */
	for (i = 0; i < QSE_COUNTOF(fnctab); i++)
	{
		if (qse_strcmp (fnctab[i].name, name) == 0) 
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[i].info;
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
	uctx_list_t list;

	rbt = (qse_rbt_t*)mod->ctx;

	QSE_MEMSET (&list, 0, QSE_SIZEOF(list));
	if (qse_rbt_insert (rbt, &rtx, QSE_SIZEOF(rtx), &list, QSE_SIZEOF(list)) == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
qse_printf (QSE_T("initialized ... module... %p %p\n"), mod, mod->ctx);

	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	qse_rbt_t* rbt;
	qse_rbt_pair_t* pair;
	
qse_printf (QSE_T("fini ... module...%p\n"), mod);
	rbt = (qse_rbt_t*)mod->ctx;

	/* garbage clean-up */
	pair = qse_rbt_search  (rbt, &rtx, QSE_SIZEOF(rtx));
	if (pair)
	{
		uctx_list_t* list;
		uctx_node_t* node, * next;

		list = QSE_RBT_VPTR(pair);
		node = list->head;
		while (node)
		{
			next = node->next;
			free_uctx_node (rtx, list, node);
			node = next;
		}

		qse_rbt_delete (rbt, &rtx, QSE_SIZEOF(rtx));
	}
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_rbt_t* rbt;

	rbt = (qse_rbt_t*)mod->ctx;

qse_printf (QSE_T("unloaded ... module...%p\n"), mod);
	QSE_ASSERT (QSE_RBT_SIZE(rbt) == 0);
	qse_rbt_close (rbt);
}

int load (qse_awk_mod_t* mod, qse_awk_t* awk)
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

qse_printf (QSE_T("loaded ... module...%p\n"), mod);
	mod->ctx = rbt;
	return 0;
}

