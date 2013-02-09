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

	xli->tok.name = qse_str_open (mmgr, 0, 128);
	if (xli->tok.name == QSE_NULL) goto oops;

	xli->sio_names = qse_htb_open (
		mmgr, QSE_SIZEOF(xli), 128, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (xli->sio_names == QSE_NULL) goto oops;
	*(qse_xli_t**)QSE_XTN(xli->sio_names) = xli;
	qse_htb_setmancbs (xli->sio_names, 
		qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_KEY_COPIER)
	);
	
	return 0;

oops:
	qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
	if (xli->sio_names) qse_htb_close (xli->sio_names);
	if (xli->tok.name) qse_str_close (xli->tok.name);
	return -1;
}

void qse_xli_fini (qse_xli_t* xli)
{
	qse_xli_clear (xli);
	qse_htb_close (xli->sio_names);
	qse_str_close (xli->tok.name);
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
}

qse_xli_pair_t* qse_xli_insertpair (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* name, qse_xli_val_t* value)
{
	qse_xli_pair_t* pair;
	qse_size_t klen, nlen;
	qse_char_t* kptr, * nptr;

	klen = qse_strlen (key);
	nlen = name? qse_strlen (name): 0;

	pair = qse_xli_callocmem (xli, 
		QSE_SIZEOF(*pair) + 
		((klen + 1) * QSE_SIZEOF(*key)) + 
		((nlen + 1) * QSE_SIZEOF(*name)));
	if (pair == QSE_NULL) return QSE_NULL;

	kptr = (qse_char_t*)(pair + 1);
	nptr = kptr + klen + 1;
	qse_strcpy (kptr, key);
	if (name) qse_strcpy (nptr, name);

	pair->type = QSE_XLI_PAIR;
	pair->key = kptr;
	pair->name = nptr;
	pair->val = value;  /* this assumes it points to a dynamically allocated atom  */

	insert_atom (xli, parent, peer, (qse_xli_atom_t*)pair);
	return pair;
}

qse_xli_pair_t* qse_xli_insertpairwithemptylist (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* name)
{
	qse_xli_list_t* val;
	qse_xli_pair_t* tmp;

	val = qse_xli_callocmem (xli, QSE_SIZEOF(*val));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_XLI_LIST;
	tmp = qse_xli_insertpair (xli, parent, peer, key, name, (qse_xli_val_t*)val);	
	if (tmp == QSE_NULL) qse_xli_freemem (xli, val);
	return tmp;
}

qse_xli_pair_t* qse_xli_insertpairwithstr (
	qse_xli_t* xli, qse_xli_list_t* parent, qse_xli_atom_t* peer,
	const qse_char_t* key, const qse_char_t* name, const qse_char_t* value, int verbatim)
{
	qse_xli_str_t* val;
	qse_xli_pair_t* tmp;
	qse_size_t vlen;

	vlen = qse_strlen (value);
	val = qse_xli_callocmem (xli, QSE_SIZEOF(*val) + ((vlen  + 1) * QSE_SIZEOF(*value)));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_XLI_STR;
	val->ptr = (const qse_char_t*)(val + 1);
	val->len = vlen;
	val->verbatim = verbatim;
	tmp = qse_xli_insertpair (xli, parent, peer, key, name, (qse_xli_val_t*)val);	
	if (tmp == QSE_NULL) qse_xli_freemem (xli, val);
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
	text->len = slen;

	insert_atom (xli, parent, peer, (qse_xli_atom_t*)text);

	return text;
}

/* ------------------------------------------------------ */

static void free_list (qse_xli_t* xli, qse_xli_list_t* list);

static void free_atom (qse_xli_t* xli, qse_xli_atom_t* atom)
{
	if (atom->type == QSE_XLI_PAIR)
	{
		qse_xli_pair_t* pair = (qse_xli_pair_t*)atom;

		if (pair->val->type == QSE_XLI_LIST)
			free_list (xli, (qse_xli_list_t*)pair->val);

		QSE_MMGR_FREE (xli->mmgr, pair->val);
	}
	
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
	/* TODO: free data under xli->root */
	free_list (xli, &xli->root);
}
