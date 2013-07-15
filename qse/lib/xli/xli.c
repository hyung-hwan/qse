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

#include "xli.h"
#include <qse/cmn/chr.h>

static void free_val (qse_xli_t* xli, qse_xli_val_t* val);
static void free_list (qse_xli_t* xli, qse_xli_list_t* list);
static void free_atom (qse_xli_t* xli, qse_xli_atom_t* atom);

qse_xli_t* qse_xli_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_xli_t* xli;

	xli = (qse_xli_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_xli_t) + xtnsize);
	if (xli)
	{
		if (qse_xli_init (xli, mmgr) <= -1)
		{
			QSE_MMGR_FREE (xli->mmgr, xli);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(xli), 0, xtnsize);
	}

	return xli;
}

void qse_xli_close (qse_xli_t* xli)
{
	qse_xli_ecb_t* ecb;

	for (ecb = xli->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (xli);

	qse_xli_fini (xli);
	QSE_MMGR_FREE (xli->mmgr, xli);
}

int qse_xli_init (qse_xli_t* xli, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (xli, 0, QSE_SIZEOF(*xli));
	xli->mmgr = mmgr;
	xli->errstr = qse_xli_dflerrstr;

	xli->dotted_curkey = qse_str_open (mmgr, 0, 128);
	if (xli->dotted_curkey == QSE_NULL) goto oops;

	xli->tok.name = qse_str_open (mmgr, 0, 128);
	if (xli->tok.name == QSE_NULL) goto oops;

	xli->schema = qse_rbt_open (mmgr, 0, QSE_SIZEOF(qse_char_t), 1);
	if (xli->schema == QSE_NULL) goto oops;
	qse_rbt_setstyle (xli->schema, qse_getrbtstyle(QSE_RBT_STYLE_INLINE_COPIERS));

	xli->root.type = QSE_XLI_LIST;
	xli->xnil.type = QSE_XLI_NIL;

	return 0;

oops:
	qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
	if (xli->schema) qse_rbt_close (xli->schema);
	if (xli->tok.name) qse_str_close (xli->tok.name);
	if (xli->dotted_curkey) qse_str_close (xli->dotted_curkey);
	return -1;
}

void qse_xli_fini (qse_xli_t* xli)
{
	qse_xli_clear (xli);

	qse_rbt_close (xli->schema);
	qse_str_close (xli->tok.name);
	qse_str_close (xli->dotted_curkey);
}

qse_mmgr_t* qse_xli_getmmgr (qse_xli_t* xli)
{
	return xli->mmgr;
}

void* qse_xli_getxtn (qse_xli_t* xli)
{
	return QSE_XTN (xli);
}

int qse_xli_setopt (qse_xli_t* xli, qse_xli_opt_t id, const void* value)
{
	switch (id)
	{
		case QSE_XLI_TRAIT:
			xli->opt.trait = *(const int*)value;
			return 0;
	}

	qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
	return -1;
}

int qse_xli_getopt (qse_xli_t* xli, qse_xli_opt_t  id, void* value)
{
	switch  (id)
	{
		case QSE_XLI_TRAIT:
			*(int*)value = xli->opt.trait;
			return 0;
	};

	qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
	return -1;
}

qse_xli_ecb_t* qse_xli_popecb (qse_xli_t* xli)
{
	qse_xli_ecb_t* top = xli->ecb;
	if (top) xli->ecb = top->next;
	return top;
}

void qse_xli_pushecb (qse_xli_t* xli, qse_xli_ecb_t* ecb)
{
	ecb->next = xli->ecb;
	xli->ecb = ecb;
}

/* ------------------------------------------------------ */

void* qse_xli_allocmem (qse_xli_t* xli, qse_size_t size)
{
	void* ptr;

	ptr = QSE_MMGR_ALLOC (xli->mmgr, size);
	if (ptr == QSE_NULL) 
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_xli_callocmem (qse_xli_t* xli, qse_size_t size)
{
	void* ptr;

	ptr = QSE_MMGR_ALLOC (xli->mmgr, size);
	if (ptr == QSE_NULL)
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
	else QSE_MEMSET (ptr, 0, size);
	return ptr;
}

void qse_xli_freemem (qse_xli_t* xli, void* ptr)
{
	QSE_MMGR_FREE (xli->mmgr, ptr);
}
/* ------------------------------------------------------ */

static void insert_atom (
	qse_xli_t* xli, qse_xli_list_t* parent, 
	qse_xli_atom_t* peer, qse_xli_atom_t* atom)
{
	if (parent == QSE_NULL) parent = &xli->root;

	if (peer == QSE_NULL)	
	{
		/* insert it to the tail */
		atom->prev = parent->tail;
		if (parent->tail) parent->tail->next = atom;
		parent->tail = atom;
		if (parent->head == QSE_NULL) parent->head = atom;
	}
	else
	{
		/* insert it in front of peer */
		QSE_ASSERT (parent->head != QSE_NULL);
		QSE_ASSERT (parent->tail != QSE_NULL);

		atom->prev = peer->prev;
		if (peer->prev) peer->prev->next = atom;
		else
		{
			QSE_ASSERT (peer = parent->head);
			parent->head = atom;
		}
		atom->next = peer;
		peer->prev = atom;
	}
	atom->super = parent;
}

static qse_xli_pair_t* insert_pair (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_cstr_t* key, const qse_cstr_t* alias, qse_xli_val_t* value)
{
	qse_xli_pair_t* pair;
	qse_size_t alen;
	qse_char_t* kptr, * nptr;

	alen = alias? alias->len: 0;

	pair = qse_xli_callocmem (xli, 
		QSE_SIZEOF(*pair) + 
		((key->len + 1) * QSE_SIZEOF(*key->ptr)) + 
		((alen + 1) * QSE_SIZEOF(*alias->ptr)));
	if (pair == QSE_NULL) return QSE_NULL;

	kptr = (qse_char_t*)(pair + 1);
	qse_strcpy (kptr, key->ptr);

	pair->type = QSE_XLI_PAIR;
	pair->key = kptr;
	if (alias) 
	{
		nptr = kptr + key->len + 1;
		qse_strcpy (nptr, alias->ptr);
		pair->alias = nptr;
	}
	pair->val = value; /* take note of no duplication here */

	insert_atom (xli, parent, peer, (qse_xli_atom_t*)pair);
	return pair;
}

qse_xli_pair_t* qse_xli_insertpair (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* alias, qse_xli_val_t* value)
{
	qse_cstr_t k;

	k.ptr = key;
	k.len = qse_strlen (key);

	if (alias)
	{
		qse_cstr_t a;

		a.ptr = alias;
		a.len = qse_strlen (alias);

		return insert_pair (xli, parent, peer, &k, &a, value);
	}
	else
	{
		return insert_pair (xli, parent, peer, &k, QSE_NULL, value);
	}
}

qse_xli_pair_t* qse_xli_insertpairwithemptylist (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* alias)
{
	qse_xli_list_t* val;
	qse_xli_pair_t* tmp;

	val = qse_xli_callocmem (xli, QSE_SIZEOF(*val));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_XLI_LIST;
	tmp = qse_xli_insertpair (xli, parent, peer, key, alias, (qse_xli_val_t*)val);	
	if (tmp == QSE_NULL) qse_xli_freemem (xli, val);
	return tmp;
}

qse_xli_pair_t* qse_xli_insertpairwithstr (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* alias, const qse_cstr_t* value)
{
	qse_xli_str_t* val;
	qse_xli_pair_t* tmp;

	val = qse_xli_callocmem (xli, QSE_SIZEOF(*val) + ((value->len  + 1) * QSE_SIZEOF(*value->ptr)));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_XLI_STR;

	qse_strncpy ((qse_char_t*)(val + 1), value->ptr, value->len);
	val->ptr = (const qse_char_t*)(val + 1);
	val->len = value->len;

	tmp = qse_xli_insertpair (xli, parent, peer, key, alias, (qse_xli_val_t*)val);	
	if (tmp == QSE_NULL) qse_xli_freemem (xli, val);
	return tmp;
}

qse_xli_pair_t* qse_xli_insertpairwithstrs (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* alias, 
	const qse_cstr_t value[], qse_size_t count)
{
	qse_xli_pair_t* tmp;
	qse_xli_str_t* str;
	qse_size_t i;

	if (count <= 0) 
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	tmp = qse_xli_insertpairwithstr (xli, parent, peer, key, alias, &value[0]);
	if (tmp == QSE_NULL) return QSE_NULL;

	str = (qse_xli_str_t*)tmp->val;
	for (i = 1; i < count; i++)
	{
		str = qse_xli_addsegtostr (xli, str, &value[i]);			
		if (str == QSE_NULL)
		{
			free_atom (xli, (qse_xli_atom_t*)tmp);
			return QSE_NULL;
		}
	}

	return tmp;
}

qse_xli_text_t* qse_xli_inserttext (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer, const qse_char_t* str)
{
	qse_xli_text_t* text;
	qse_size_t slen;

	slen = qse_strlen (str);

	text = qse_xli_callocmem (xli, QSE_SIZEOF(*text) + ((slen + 1) * QSE_SIZEOF(*str)));
	if (text == QSE_NULL) return QSE_NULL;

	text->type = QSE_XLI_TEXT;
	text->ptr = (const qse_char_t*)(text + 1);

	qse_strcpy ((qse_char_t*)(text + 1), str);

	insert_atom (xli, parent, peer, (qse_xli_atom_t*)text);

	return text;
}

qse_xli_file_t* qse_xli_insertfile (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer, const qse_char_t* path)
{
	qse_xli_file_t* file;
	qse_size_t plen;

	plen = qse_strlen (path);

	file = qse_xli_callocmem (xli, QSE_SIZEOF(*file) + ((plen + 1) * QSE_SIZEOF(*path)));
	if (file == QSE_NULL) return QSE_NULL;

	file->type = QSE_XLI_FILE;
	file->path = (const qse_char_t*)(file + 1);

	qse_strcpy ((qse_char_t*)(file + 1), path);

	insert_atom (xli, parent, peer, (qse_xli_atom_t*)file);

	return file;
}

qse_xli_eof_t* qse_xli_inserteof (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer)
{
	qse_xli_eof_t* eof;

	eof = qse_xli_callocmem (xli, QSE_SIZEOF(*eof));
	if (eof == QSE_NULL) return QSE_NULL;

	eof->type = QSE_XLI_EOF;
	insert_atom (xli, parent, peer, (qse_xli_atom_t*)eof);

	return eof;
}

/* ------------------------------------------------------ */

static void free_val (qse_xli_t* xli, qse_xli_val_t* val)
{
	if ((qse_xli_nil_t*)val != &xli->xnil)
	{
		if (val->type == QSE_XLI_LIST)
			free_list (xli, (qse_xli_list_t*)val);
		else if (val->type == QSE_XLI_STR)
		{
			qse_xli_str_t* cur, * next; 

			cur = ((qse_xli_str_t*)val)->next;
			while (cur)
			{
				next = cur->next;
				QSE_MMGR_FREE (xli->mmgr, cur);
				cur = next;
			}
		}

		QSE_MMGR_FREE (xli->mmgr, val);
	}
}

static void free_atom (qse_xli_t* xli, qse_xli_atom_t* atom)
{
	if (atom->type == QSE_XLI_PAIR) free_val (xli, ((qse_xli_pair_t*)atom)->val);
	QSE_MMGR_FREE (xli->mmgr, atom);
}

static void free_list (qse_xli_t* xli, qse_xli_list_t* list)
{
	qse_xli_atom_t* p, * n;

	p = list->head;
	while (p)
	{
		n = p->next;
		free_atom (xli, p);	
		p = n;
	}

	list->head = QSE_NULL;
	list->tail = QSE_NULL;
}

void qse_xli_clear (qse_xli_t* xli)
{
	free_list (xli, &xli->root);
	qse_rbt_clear (xli->schema);

	qse_xli_seterrnum (xli, QSE_XLI_ENOERR, QSE_NULL);
	qse_xli_clearrionames (xli);
	qse_xli_clearwionames (xli);
}

qse_xli_list_t* qse_xli_getroot (qse_xli_t* xli)
{
	return &xli->root;
}

void qse_xli_clearroot (qse_xli_t* xli)
{
	free_list (xli, &xli->root);
}


/* ------------------------------------------------------ */

static qse_size_t count_pairs_by_key (
	qse_xli_t* xli, const qse_xli_list_t* list, const qse_cstr_t* key)
{
	qse_xli_atom_t* p;
	qse_size_t count = 0;

	/* TODO: speed up. no linear search */
	p = list->head;
	while (p)
	{
		if (p->type == QSE_XLI_PAIR)
		{
			qse_xli_pair_t* pair = (qse_xli_pair_t*)p;
			if (qse_strxcmp (key->ptr, key->len, pair->key) == 0) count++;
		}

		p = p->next;
	}

	return count;
}

static qse_xli_pair_t* find_pair_by_key_and_alias (
	qse_xli_t* xli, const qse_xli_list_t* list, 
	const qse_cstr_t* key, const qse_cstr_t* alias)
{
	qse_xli_atom_t* p;

	/* TODO: speed up. no linear search */
	p = list->head;
	while (p)
	{
		if (p->type == QSE_XLI_PAIR)
		{
			qse_xli_pair_t* pair = (qse_xli_pair_t*)p;
			if (qse_strxcmp (key->ptr, key->len, pair->key) == 0) 
			{
				if (alias == QSE_NULL || 
				    qse_strxcmp (alias->ptr, alias->len, pair->alias) == 0) return pair;
			}
		}

		p = p->next;
	}

	return QSE_NULL;
}

static qse_xli_pair_t* find_pair_by_key_and_index (
	qse_xli_t* xli, const qse_xli_list_t* list, 
	const qse_cstr_t* key, qse_size_t index)
{
	qse_xli_atom_t* p;
	qse_size_t count = 0;

	/* TODO: speed up. no linear search */
	p = list->head;
	while (p)
	{
		if (p->type == QSE_XLI_PAIR)
		{
			qse_xli_pair_t* pair = (qse_xli_pair_t*)p;
			if (qse_strxcmp (key->ptr, key->len, pair->key) == 0) 
			{
				if (index == count) return pair;
				count++;
			}
		}

		p = p->next;
	}

	return QSE_NULL;
}


struct fqpn_seg_t
{
	qse_cstr_t ki; /* key + index */
	qse_cstr_t key;
	enum
	{
		FQPN_SEG_IDX_NONE,
		FQPN_SEG_IDX_NUMBER,
		FQPN_SEG_IDX_ALIAS
	} idxtype; 
	
	union
	{
		qse_size_t number;
		qse_cstr_t alias;	
	} idx;
};

typedef struct fqpn_seg_t fqpn_seg_t;

const qse_char_t* get_next_fqpn_segment (qse_xli_t* xli, const qse_char_t* fqpn, fqpn_seg_t* seg)
{
	const qse_char_t* ptr;

	seg->key.ptr = ptr = fqpn;
	while (*ptr != QSE_T('\0') && *ptr != QSE_T('.') && *ptr != QSE_T('[') && *ptr != QSE_T('{')) ptr++;
	if (ptr == seg->key.ptr) goto inval; /* no key part */
	seg->key.len = ptr - seg->key.ptr;

	if (*ptr == QSE_T('['))
	{
		/*  index is specified */
		ptr++; /* skip [ */

		if (QSE_ISDIGIT(*ptr))
		{
			/* numeric index */
			qse_size_t index = 0, count = 0;
			do 
			{
				index = index * 10 + (*ptr++ - QSE_T('0')); 
				count++;
			}
			while (QSE_ISDIGIT(*ptr));

			if (*ptr != QSE_T(']')) goto inval;

			seg->idxtype = FQPN_SEG_IDX_NUMBER;
			seg->idx.number = index;

			seg->ki.ptr = seg->key.ptr;
			seg->ki.len = seg->key.len + count + 2;
		}
		else goto inval;

		ptr++;  /* skip ] */
		if (*ptr != QSE_T('\0') && *ptr != QSE_T('.')) goto inval;
	}
	else if (*ptr == QSE_T('{'))
	{
		/* word index - alias */
		ptr++; /* skip { */

		/* no escaping is supported for the alias inside {}.
		 * so if your alias contains these characters (in a quoted string), 
		 * you can't reference it using a dotted key name. */
		seg->idxtype = FQPN_SEG_IDX_ALIAS;
		seg->idx.alias.ptr = ptr;
		while (*ptr != QSE_T('}') && *ptr != QSE_T('\0')) ptr++;
		seg->idx.alias.len = ptr - seg->idx.alias.ptr;

		seg->ki.ptr = seg->key.ptr;
		seg->ki.len = seg->key.len + seg->idx.alias.len + 2;

		if (*ptr != QSE_T('}') || seg->idx.alias.len == 0) goto inval;

		ptr++;  /* skip } */
		if (*ptr != QSE_T('\0') && *ptr != QSE_T('.')) goto inval;
	}
	else
	{
		seg->idxtype = FQPN_SEG_IDX_NONE;
		seg->ki = seg->key;
	}

	return ptr;

inval:
	qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
	return QSE_NULL;
}

qse_xli_pair_t* qse_xli_findpair (qse_xli_t* xli, const qse_xli_list_t* list, const qse_char_t* fqpn)
{
	const qse_char_t* ptr;
	const qse_xli_list_t* curlist;
	fqpn_seg_t seg;

	curlist = list? list: &xli->root;

	ptr = fqpn;
	while (1)
	{
		qse_xli_pair_t* pair;

		ptr = get_next_fqpn_segment (xli, ptr, &seg);
		if (ptr == QSE_NULL) return QSE_NULL;

		if (curlist->type != QSE_XLI_LIST) 
		{
			/* check the type of curlist. this check is needed
			 * because of the unconditional switching at the bottom of the 
			 * this loop. this implementation strategy has been chosen
			 * to provide the segment name easily when setting the error. */
			goto noent;
		}

		switch (seg.idxtype)
		{
			case FQPN_SEG_IDX_NONE: 
				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, QSE_NULL);
				break;

			case FQPN_SEG_IDX_NUMBER:
				pair = find_pair_by_key_and_index (xli, curlist, &seg.key, seg.idx.number);
				break;

			default: /*case FQPN_SEG_IDX_ALIAS:*/
				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, &seg.idx.alias);
				break;
		}

		if (pair == QSE_NULL) goto noent;
		if (*ptr == QSE_T('\0')) return pair; /* no more segments */

		/* more segments to handle */
		QSE_ASSERT (*ptr == QSE_T('.'));
		ptr++; /* skip . */

		/* switch to the value regardless of its type.
		 * check if it is a list in the beginning of the loop
		 * just after having gotten the next segment alias */
		curlist = (qse_xli_list_t*)pair->val;
	}

	/* this part must never be reached */
	qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
	return QSE_NULL;

noent:
	qse_xli_seterrnum (xli, QSE_XLI_ENOENT, &seg.ki);
	return QSE_NULL;
}

qse_size_t qse_xli_countpairs (qse_xli_t* xli, const qse_xli_list_t* list, const qse_char_t* fqpn)
{

	const qse_char_t* ptr;
	const qse_xli_list_t* curlist;
	fqpn_seg_t seg;

	curlist = list? list: &xli->root;

	ptr = fqpn;
	while (1)
	{
		qse_xli_pair_t* pair;

		ptr = get_next_fqpn_segment (xli, ptr, &seg);
		if (ptr == QSE_NULL) return 0;

		if (curlist->type != QSE_XLI_LIST) 
		{
			/* check the type of curlist. this check is needed
			 * because of the unconditional switching at the bottom of the 
			 * this loop. this implementation strategy has been chosen
			 * to provide the segment name easily when setting the error. */
			goto noent;
		}

		switch (seg.idxtype)
		{
			case FQPN_SEG_IDX_NONE:
				if (*ptr == QSE_T('\0')) 
				{
					/* last segment */
					return count_pairs_by_key (xli, curlist, &seg.key);
				}

				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, QSE_NULL);
				if (pair == QSE_NULL) goto noent;
				break;

			case FQPN_SEG_IDX_NUMBER:
				pair = find_pair_by_key_and_index (xli, curlist, &seg.key, seg.idx.number);
				if (pair == QSE_NULL) goto noent;
				if (*ptr == QSE_T('\0')) return 1;
				break;

			default: /*case FQPN_SEG_IDX_ALIAS:*/
				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, &seg.idx.alias);
				if (pair == QSE_NULL) goto noent;
				if (*ptr == QSE_T('\0')) return 1;
				break;
		}

		/* more segments to handle */
		QSE_ASSERT (*ptr == QSE_T('.'));
		ptr++; /* skip . */

		/* switch to the value regardless of its type.
		 * check if it is a list in the beginning of the loop
		 * just after having gotten the next segment alias */
		curlist = (qse_xli_list_t*)pair->val;
	}

	/* this part must never be reached */
	qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
	return 0;

noent:
	qse_xli_seterrnum (xli, QSE_XLI_ENOENT, &seg.ki);
	return 0;
}


qse_xli_str_t* qse_xli_addsegtostr (
	qse_xli_t* xli, qse_xli_str_t* str, const qse_cstr_t* value)
{
	qse_xli_str_t* val;

	val = qse_xli_callocmem (xli, QSE_SIZEOF(*val) + ((value->len  + 1) * QSE_SIZEOF(*value->ptr)));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_XLI_STR;
	qse_strncpy ((qse_char_t*)(val + 1), value->ptr, value->len);
	val->ptr = (const qse_char_t*)(val + 1);
	val->len = value->len;
		
	val->next = str->next;
	str->next = val;
	return str->next;
}

qse_char_t* qse_xli_dupflatstr (qse_xli_t* xli, qse_xli_str_t* str, qse_size_t* len, qse_size_t* nsegs)
{
	qse_char_t* tmp;
	qse_xli_str_t* cur;
	qse_size_t x, y;

	for (x = 0, cur = str; cur; cur = cur->next) x += (cur->len + 1);

	tmp = qse_xli_allocmem (xli, (x + 1) * QSE_SIZEOF(*tmp));
	if (tmp == QSE_NULL) return QSE_NULL;

	for (x = 0, y = 0, cur = str; cur; cur = cur->next, y++) 
	{
		qse_strncpy (&tmp[x], cur->ptr, cur->len);
		x += (cur->len + 1);	
	}
	tmp[x] = QSE_T('\0'); 

	if (len) *len = x; /* the length of the flattened string */
	if (nsegs) *nsegs = y; /* the number of string segments used for flattening */

	return tmp;
}

/* ------------------------------------------------------ */

int qse_xli_definepair (qse_xli_t* xli, const qse_char_t* fqpn, const qse_xli_scm_t* scm)
{
	int tmp;

	/* the fully qualified pair name for this function must not contain an index or 
	 * an alias. it is so because the definition contains information on key duplicability 
	 * and whether to allow an alias */

	tmp = scm->flags & (QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_VALSTR | QSE_XLI_SCM_VALNIL);
	if (tmp != QSE_XLI_SCM_VALLIST && tmp != QSE_XLI_SCM_VALSTR && tmp != QSE_XLI_SCM_VALNIL)
	{
		/* VAL_LIST, VAL_STR, VAL_NIL can't co-exist */
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}

	if (qse_rbt_upsert (xli->schema, fqpn, qse_strlen(fqpn), scm, QSE_SIZEOF(*scm)) == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
		return -1;
	}

	return 0;
}

int qse_xli_undefinepair (qse_xli_t* xli, const qse_char_t* fqpn)
{
	if (qse_rbt_delete (xli->schema, fqpn, qse_strlen(fqpn)) <= -1)
	{
		qse_cstr_t ea;
		ea.ptr = fqpn;
		ea.len = qse_strlen (ea.ptr);
		qse_xli_seterrnum (xli, QSE_XLI_ENOENT, &ea);
		return -1;
	}

	return 0;
}

void qse_xli_undefinepairs (qse_xli_t* xli)
{
	qse_rbt_clear (xli->schema);
}


#if 0
qse_xli_pair_t* qse_xli_getpair (qse_xli_t* xli, const qse_char_t* fqpn)
{
	return qse_xli_findpair (xli, &xli->root, fqpn);
}

qse_xli_pair_t* qse_xli_setpair (qse_xli_t* xli, const qse_char_t* fqpn, const qse_xli_val_t* val) -> str, val, nil
{
	const qse_char_t* ptr;
	const qse_xli_list_t* curlist;
	fqpn_seg_t seg;

	curlist = list? list: &xli->root;

	ptr = fqpn;
	while (1)
	{
		qse_xli_pair_t* pair;

		ptr = get_next_fqpn_segment (xli, ptr, &seg);
		if (ptr == QSE_NULL) return QSE_NULL;

		if (curlist->type != QSE_XLI_LIST) 
		{
			/* for example, the second segment y in a FQPN "x.y.z" may be found.
			 * however the value for it is not a list. i can't force insert a new
			 * pair 'z' under a non-list atom */
			goto noent; 
		}

		switch (seg.idxtype)
		{
			case FQPN_SEG_IDX_NONE: 
				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, QSE_NULL);
				break;

			case FQPN_SEG_IDX_NUMBER:
				pair = find_pair_by_key_and_index (xli, curlist, &seg.key, seg.idx.number);
				break;

			case FQPN_SEG_IDX_ALIAS:
				pair = find_pair_by_key_and_alias (xli, curlist, &seg.key, &seg.idx.alias);
				break;
		}

		if (pair == QSE_NULL) 
		{
			/* insert a new item..... */
			if (*ptr == QSE_T('\0'))
			{
				/* append according to the value */
				pair = insert_pair (xli, curlist, QSE_NULL, &seg.key, ((seg.idxtype == FQPN_SEG_IDX_ALIAS)? &seg.idx.alias: QSE_NULL), val);
			}
			else
			{
				/* this is not the last segment. insert an empty list */
/* seg.key, seg.alias */
				pair = insert_pair_with_empty_list (xli, curlist, QSE_NULL, key, alias);
			}

			if (pair == QSE_NULL) return QSE_NULL;
		}

		if (*ptr == QSE_T('\0')) 
		{
			/* no more segment. and the final match is found.
			 * update the pair with a new value */
		}

		/* more segments to handle */
		QSE_ASSERT (*ptr == QSE_T('.'));
		ptr++; /* skip . */

		/* switch to the value regardless of its type.
		 * check if it is a list in the beginning of the loop
		 * just after having gotten the next segment alias */
		curlist = (qse_xli_list_t*)pair->val;
	}

	/* this part must never be reached */
	qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
	return QSE_NULL;

noent:
	qse_xli_seterrnum (xli, QSE_XLI_ENOENT, &seg.ki);
	return QSE_NULL;
}
#endif
