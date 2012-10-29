#include <qse/awk/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include <qse/cmn/mbwc.h>
#include "../../lib/cmn/mem.h"

#if defined(HAVE_UCI_H)
#	include <uci.h>
#else
#	error this module needs uci.h
#endif

#define ERR_WRONG_HANDLE 999

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

	int errnum;
};

static uctx_node_t* new_uctx_node (qse_awk_rtx_t* rtx, uctx_list_t* list)
{
	/* create a new context node and append it to the list tail */
	uctx_node_t* node;

	node = QSE_NULL;

	if (list->free) node = list->free;
	else
	{
		node = qse_awk_rtx_callocmem (rtx, QSE_SIZEOF(*node));
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

			tmp = (uctx_node_t**) qse_awk_rtx_reallocmem (
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

static void free_uctx_node (qse_awk_rtx_t* rtx, uctx_list_t* list, uctx_node_t* node)
{
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;
	if (list->head == node) list->head = node->next;
	if (list->tail == node) list->tail = node->prev;

	list->map.tab[node->id] = QSE_NULL;

	if (node->ctx) 
	{
		uci_free_context (node->ctx);
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
		uctx_node_t* curnode;

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

static int close_byid (qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		free_uctx_node (rtx, list, list->map.tab[id]);
		x = UCI_OK;
	}
	
	return -x;
}

static int setconfdir_byid (
	qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id, const qse_char_t* path)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		x = uci_set_confdir (list->map.tab[id]->ctx, path);
	#else
		qse_mchar_t* tmp;
		qse_mmgr_t* mmgr = qse_awk_rtx_getmmgr(rtx);
		tmp = qse_wcstombsdup (path, QSE_NULL, mmgr);
		if (tmp)
		{
			x = uci_set_confdir (list->map.tab[id]->ctx, tmp);
			QSE_MMGR_FREE (mmgr, tmp);
		}
		else x = UCI_ERR_MEM;
	#endif
	}
	
	return -x;
}

static int setsavedir_byid (
	qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id, const qse_char_t* path)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		x = uci_set_savedir (list->map.tab[id]->ctx, path);
	#else
		qse_mchar_t* tmp;
		qse_mmgr_t* mmgr = qse_awk_rtx_getmmgr(rtx);
		tmp = qse_wcstombsdup (path, QSE_NULL, mmgr);
		if (tmp)
		{
			x = uci_set_savedir (list->map.tab[id]->ctx, tmp);
			QSE_MMGR_FREE (mmgr, tmp);
		}
		else x = UCI_ERR_MEM;
	#endif
	}
	
	return -x;
}

static int load_byid (
	qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id, const qse_char_t* path)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		x = uci_load (list->map.tab[id]->ctx, path, QSE_NULL);
	#else
		qse_mchar_t* tmp;
		qse_mmgr_t* mmgr = qse_awk_rtx_getmmgr(rtx);
		tmp = qse_wcstombsdup (path, QSE_NULL, mmgr);
		if (tmp)
		{
			x = uci_load (list->map.tab[id]->ctx, tmp, QSE_NULL);
			QSE_MMGR_FREE (mmgr, tmp);
		}
		else x = UCI_ERR_MEM;
	#endif
	}
	
	return -x;
}

static int unload_byid (qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		x = uci_unload (list->map.tab[id]->ctx, QSE_NULL);
		return 0;
	}
	
	return -x;
}

static int save_byid (qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		x = uci_save (list->map.tab[id]->ctx, QSE_NULL);
	}
	
	return -x;
}

#if 0
	else if (e->type == UCI_TYPE_PACKAGE)
	{
		/* anything to do? */
		[type] = "PACKAGE";
		[section.type] = "xxxxxxxxxx";
		[option.type] = "string";
		[option.type] = "list";
		[option.value] = "xxxxxxx";
		[option.value.num] = "xxxxxxx";
		[option.value.1]
		[option.value.2]
	}
#endif

static int getsection_byid (
	qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id,
	const qse_char_t* tuple, qse_awk_val_t** retv)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		qse_mmgr_t* mmgr;
		qse_mchar_t* mtuple;
		struct uci_ptr ptr;

		mmgr = qse_awk_rtx_getmmgr (rtx);

	#if defined(QSE_CHAR_IS_MCHAR)
		mtuple = qse_mbsdup (tuple, mmgr);
	#else
		mtuple = qse_wcstombsdup (tuple, QSE_NULL, mmgr);
	#endif
		if (mtuple)
		{
			x = uci_lookup_ptr (list->map.tab[id]->ctx, &ptr, mtuple, 1);
			if (x == UCI_OK)
			{
				if (ptr.flags & UCI_LOOKUP_COMPLETE)
				{
					struct uci_element* e;

					e = ptr.last;	
					if (e->type == UCI_TYPE_SECTION)
					{
						qse_awk_val_map_data_t md[4];
						qse_long_t lv;
						qse_awk_val_t* tmp;

						QSE_MEMSET (md, 0, QSE_SIZEOF(md));
		
						md[0].key.ptr = QSE_T("type");
						md[0].key.len = 4;
						md[0].type = QSE_AWK_VAL_MAP_DATA_MBS;
						md[0].vptr = ptr.s->type;

						md[1].key.ptr = QSE_T("name");
						md[1].key.len = 4;
						md[1].type = QSE_AWK_VAL_MAP_DATA_MBS;
						md[1].vptr = ptr.s->e.name; /* e->name == ptr.s->e.name */

						md[2].key.ptr = QSE_T("anon");
						md[2].key.len = 4;
						md[2].vptr = QSE_AWK_VAL_MAP_DATA_INT;
						lv = ptr.s->anonymous;
						md[2].vptr = &lv;

						tmp = qse_awk_rtx_makemapvalwithdata (rtx, md);
						if (tmp) *retv = tmp;
						else x = UCI_ERR_MEM;
					}
					else x = UCI_ERR_NOTFOUND;
					
				}
				else x = UCI_ERR_NOTFOUND;
			}

			QSE_MMGR_FREE (mmgr, mtuple);
		}
		else x = UCI_ERR_MEM;
	}

	return -x;
}

static int getoption_byid (
	qse_awk_rtx_t* rtx, uctx_list_t* list, qse_long_t id,
	const qse_char_t* tuple, qse_awk_val_t** retv)
{
	int x = ERR_WRONG_HANDLE;

	if (id >= 0 && id < list->map.high && list->map.tab[id]) 
	{
		qse_mmgr_t* mmgr;
		qse_mchar_t* mtuple;
		struct uci_ptr ptr;

		mmgr = qse_awk_rtx_getmmgr (rtx);

	#if defined(QSE_CHAR_IS_MCHAR)
		mtuple = qse_mbsdup (tuple, mmgr);
	#else
		mtuple = qse_wcstombsdup (tuple, QSE_NULL, mmgr);
	#endif
		if (mtuple)
		{
			x = uci_lookup_ptr (list->map.tab[id]->ctx, &ptr, mtuple, 1);
			if (x == UCI_OK)
			{
				if (ptr.flags & UCI_LOOKUP_COMPLETE)
				{
					struct uci_element* e;

					e = ptr.last;	
					if (e->type == UCI_TYPE_OPTION)
					{
						struct uci_option* uo = ptr.o;

						if (uo->type == UCI_TYPE_STRING)
						{
							qse_awk_val_map_data_t md[3];
							qse_awk_val_t* tmp;

							QSE_MEMSET (md, 0, QSE_SIZEOF(md));

							md[0].key.ptr = QSE_T("type");
							md[0].key.len = 4;
							md[0].type = QSE_AWK_VAL_MAP_DATA_STR;
							md[0].vptr = QSE_T("string");

							md[1].key.ptr = QSE_T("value");
							md[1].key.len = 5;
							md[1].type = QSE_AWK_VAL_MAP_DATA_MBS;
							md[1].vptr = uo->v.string;

							tmp = qse_awk_rtx_makemapvalwithdata (rtx, md);
							if (tmp) *retv = tmp;
							else x = UCI_ERR_MEM;
						}
						else if (uo->type == UCI_TYPE_LIST)
						{
							struct uci_element* tmp;
							qse_size_t count = 0;
							qse_awk_val_map_data_t* md;

							uci_foreach_element(&uo->v.list, tmp) { count++; }

							md = qse_awk_rtx_callocmem (rtx, (count + 2) * QSE_SIZEOF(*md));
							if (md == QSE_NULL) x = UCI_ERR_MEM;
							else
							{
								md[0].key.ptr = QSE_T("type");
								md[0].key.len = 4;
								md[0].type = QSE_AWK_VAL_MAP_DATA_STR;
								md[0].vptr = QSE_T("list");

								count = 1;
								uci_foreach_element(&uo->v.list, tmp)
								{	
									md[count].key.ptr = QSE_T("value.idx");
									md[count].key.len = 7;
									md[count].type = QSE_AWK_VAL_MAP_DATA_MBS;
									md[count].vptr = tmp->name;
									count++;
								}

								tmp = qse_awk_rtx_makemapvalwithdata (rtx, md);
								qse_awk_rtx_freemem (rtx, md);
								if (tmp) *retv = tmp;
								else x = UCI_ERR_MEM;
							}
						}
					}
					else x = UCI_ERR_NOTFOUND;
					
				}
				else x = UCI_ERR_NOTFOUND;
			}

			QSE_MMGR_FREE (mmgr, mtuple);
		}
		else x = UCI_ERR_MEM;
	}

	return -x;
}
/* ------------------------------------------------------------------------ */

static QSE_INLINE uctx_list_t* rtx_to_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_rbt_pair_t* pair;
	pair = qse_rbt_search ((qse_rbt_t*)fi->mod->ctx, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (uctx_list_t*)QSE_RBT_VPTR(pair);
}

static int fnc_uci_errno (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;

	list = rtx_to_list (rtx, fi);

	retv = qse_awk_rtx_makeintval (rtx, list->errnum);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	uctx_node_t* node;
	qse_long_t ret;
	qse_awk_val_t* retv;

	list = rtx_to_list (rtx, fi);
	node = new_uctx_node (rtx, list);
	ret = node? node->id: -UCI_ERR_MEM;
	
	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

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
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else ret = close_byid (rtx, list, id);

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_load  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else
	{
		qse_awk_val_t* v;

		v = qse_awk_rtx_getarg(rtx, 1);
		if (v->type == QSE_AWK_VAL_STR)
		{
			ret = load_byid (rtx, list, id, ((qse_awk_val_str_t*)v)->val.ptr);
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			ret = qse_awk_rtx_valtostr (rtx, qse_awk_rtx_getarg (rtx, 1), &out);
			if (ret <= -1) ret = -UCI_ERR_MEM;
			else
			{
				ret = load_byid (rtx, list, id, out.u.cpldup.ptr);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			}
		}
	}

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_unload  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else ret = unload_byid (rtx, list, id);

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_save  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else ret = save_byid (rtx, list, id);

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_setconfdir  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else
	{
		qse_awk_val_t* v;

		v = qse_awk_rtx_getarg(rtx, 1);
		if (v->type == QSE_AWK_VAL_STR)
		{
			ret = setconfdir_byid (rtx, list, id, ((qse_awk_val_str_t*)v)->val.ptr);
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			ret = qse_awk_rtx_valtostr (rtx, qse_awk_rtx_getarg (rtx, 1), &out);
			if (ret <= -1) ret = -UCI_ERR_MEM;
			else
			{
				ret = setconfdir_byid (rtx, list, id, out.u.cpldup.ptr);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			}
		}
	}

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}


static int fnc_uci_setsavedir  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else
	{
		qse_awk_val_t* v;

		v = qse_awk_rtx_getarg(rtx, 1);
		if (v->type == QSE_AWK_VAL_STR)
		{
			ret = setsavedir_byid (rtx, list, id, ((qse_awk_val_str_t*)v)->val.ptr);
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			ret = qse_awk_rtx_valtostr (rtx, qse_awk_rtx_getarg (rtx, 1), &out);
			if (ret <= -1) ret = -UCI_ERR_MEM;
			else
			{
				ret = setsavedir_byid (rtx, list, id, out.u.cpldup.ptr);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			}
		}

	}

	if (ret <= -1) 
	{
		list->errnum = ret;
		ret = -1;
	}

	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_uci_getoption (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) ret = -UCI_ERR_INVAL;
	else
	{
		qse_awk_val_t* v;

		v = qse_awk_rtx_getarg(rtx, 1);
		if (v->type == QSE_AWK_VAL_STR)
		{
			ret = getoption_byid (rtx, list, id, ((qse_awk_val_str_t*)v)->val.ptr, &retv);
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			ret = qse_awk_rtx_valtostr (rtx, qse_awk_rtx_getarg (rtx, 1), &out);
			if (ret <= -1) ret = -UCI_ERR_MEM;
			else
			{
				ret = getoption_byid (rtx, list, id, out.u.cpldup.ptr, &retv);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			}
		}
	}

	if (ret <= -1) 
	{
		list->errnum = ret;
	}
	else
	{
		qse_awk_rtx_setretval (rtx, retv);
	}
	return 0;
}

static int fnc_uci_getsection (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;
	qse_long_t id;
	int ret;
	
	list = rtx_to_list (rtx, fi);

	ret = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret >= 0)
	{
		qse_awk_val_t* v;

		v = qse_awk_rtx_getarg(rtx, 1);
		if (v->type == QSE_AWK_VAL_STR)
		{
			ret = getsection_byid (rtx, list, id, ((qse_awk_val_str_t*)v)->val.ptr, &retv);
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			ret = qse_awk_rtx_valtostr (rtx, qse_awk_rtx_getarg (rtx, 1), &out);
			if (ret <= -1) ret = -UCI_ERR_MEM;
			else
			{
				ret = getsection_byid (rtx, list, id, out.u.cpldup.ptr, &retv);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			}
		}
	}

	if (ret <= -1)
	{
		list->errnum = ret;
	}
	else 
	{
		qse_awk_rtx_setretval (rtx, retv);
	}
	return 0;
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

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	{ QSE_T("close"),       { { 1, 1 }, fnc_uci_close      } },
	{ QSE_T("commit"),      { { 1, 1 }, fnc_uci_commit     } },
	{ QSE_T("errno"),       { { 0, 0 }, fnc_uci_errno      } },
	{ QSE_T("getoption"),   { { 2, 2 }, fnc_uci_getoption  } },
	{ QSE_T("getsection"),  { { 2, 2 }, fnc_uci_getsection } },
	{ QSE_T("load"),        { { 2, 2 }, fnc_uci_load       } },
	{ QSE_T("open"),        { { 0, 0 }, fnc_uci_open       } },
	{ QSE_T("revert"),      { { 1, 1 }, fnc_uci_revert     } },
	{ QSE_T("save"),        { { 1, 1 }, fnc_uci_save       } },
	{ QSE_T("setconfdir"),  { { 2, 2 }, fnc_uci_setconfdir } }, 
	{ QSE_T("setsavedir"),  { { 2, 2 }, fnc_uci_setsavedir } }, 
	{ QSE_T("unload"),      { { 1, 1 }, fnc_uci_unload     } }
};

/* ------------------------------------------------------------------------ */

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

	mod->ctx = rbt;
	return 0;
}

