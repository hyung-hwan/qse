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



#include <qse/xli/SkvEnv.hpp>
#include <qse/xli/stdxli.h>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"

#define SCTN_KEY_SPLITTER QSE_T('*')

QSE_BEGIN_NAMESPACE(QSE)

static qse_char_t splitter[2] = { SCTN_KEY_SPLITTER, QSE_T('\0') };

SkvEnv::SkvEnv (Mmgr* mmgr): Mmged(mmgr), xli(QSE_NULL)
{
}

SkvEnv::~SkvEnv ()
{
	if (this->xli) qse_xli_close (this->xli);
}

int SkvEnv::addItem (const qse_char_t* name, const qse_char_t* dval, ProbeProc probe)
{
	qse_char_t*  sctn, * key;
	qse_size_t sctn_len, key_len;
	ItemList::Node* np;

	if (this->split_name(name, &sctn, &sctn_len, &key, &key_len) == -1) return -1;

	for (np = this->item_list.getHeadNode(); np; np = np->getNextNode()) 
	{
		Item& item = np->getValue();
		if (qse_strxcmp(sctn, sctn_len, item.sctn) == 0 &&
		    qse_strxcmp(key, key_len, item.key)  == 0) return -1;
	}

	try { np = this->item_list.append (Item()); }
	catch (...) { return -1; }

	Item& item = np->getValue();
	qse_strxncpy (item.sctn, QSE_COUNTOF(item.sctn), sctn, sctn_len);
	qse_strxncpy (item.key,  QSE_COUNTOF(item.key),  key,  key_len);
	
	qse_strxjoin (
		item.name, QSE_COUNTOF(item.name), 
		item.sctn, splitter, item.key, QSE_NULL);
	qse_strxcpy (item.dval, QSE_COUNTOF(item.dval), dval);
	item.probe = probe;

	// set its default value
	if (this->setValue(name, item.dval) <= -1) 
	{
		this->item_list.remove (np);
		return -1;
	}
	return 0;
}

int SkvEnv::removeItem (const qse_char_t* name)
{
	qse_char_t*  sctn, * key;
	qse_size_t sctn_len, key_len;

	if (this->split_name (name, &sctn, &sctn_len, &key, &key_len) == -1) return -1;

	for (ItemList::Node* np = item_list.getHeadNode(); np; np = np->getNextNode()) 
	{
		Item& item = np->value;
		if (qse_strxcmp(sctn, sctn_len, item.sctn) == 0 &&
		    qse_strxcmp(key, key_len, item.key)  == 0) 
		{
			this->item_list.remove (np);
			return 0;
		}
	}

	return -1;
}

const qse_char_t* SkvEnv::getValue (const qse_char_t* name) const
{
	if (!this->xli) return QSE_NULL;

	qse_xli_pair_t* pair = qse_xli_findpair(this->xli, QSE_NULL, name);
	if (!pair) return QSE_NULL;

	QSE_ASSERT (pair->val != QSE_NULL);
	QSE_ASSERT (pair->val->type == QSE_XLI_STR);
	
	return (((qse_xli_str_t*)pair->val))->ptr;
}

int SkvEnv::setValue (const qse_char_t* name, const qse_char_t* value)
{
	qse_char_t* sctn, * key;
	qse_size_t sctn_len, key_len;

	if (this->split_name(name, &sctn, &sctn_len, &key, &key_len) <= -1) return -1;

	if (!this->xli)
	{
		this->xli = qse_xli_openstdwithmmgr(this->getMmgr(), 0, 0, QSE_NULL);
		if (!this->xli) return -1;
		qse_xli_setopt (this->xli, QSE_XLI_KEYSPLITTER, splitter);
	}

	// find if the name is a registered item name.
	for (ItemList::Node* np = this->item_list.getHeadNode(); np; np = np->getNextNode()) 
	{
		Item& item = np->value;
		if (qse_strxcmp(sctn, sctn_len, item.sctn) == 0 &&
		    qse_strxcmp(key, key_len, item.key)  == 0) 
		{
			// if it's the registered item name, change the value.
			return this->set_value_with_item (item, value);
		}
	}

	return -1;
}

int SkvEnv::split_name (const qse_char_t* name, qse_char_t** sctn, qse_size_t* sctn_len, qse_char_t** key, qse_size_t* key_len)
{
	QSE_ASSERT (name     != QSE_NULL);
	QSE_ASSERT (sctn     != QSE_NULL);
	QSE_ASSERT (sctn_len != QSE_NULL);
	QSE_ASSERT (key      != QSE_NULL);
	QSE_ASSERT (key_len  != QSE_NULL);

	qse_char_t* p;
	qse_cstr_t s, k;

	p = qse_strtok(name, splitter, &s);
	if (!p || s.len == 0) return -1;
	qse_strtok(p, QSE_NULL, &k);
	if (k.len == 0) return -1;

	*sctn     = s.ptr;
	*sctn_len = s.len;
	*key      = k.ptr;
	*key_len  = k.len;

	return 0;
}

int SkvEnv::set_value_with_item (Item& item, const qse_char_t* value)
{
	if (item.probe && (this->*item.probe)(value) <= -1) return -1;

	qse_cstr_t v = { (qse_char_t*)value, qse_strlen(value) };

	qse_xli_pair_t* pair;
	pair = qse_xli_setpairwithstr(this->xli, QSE_NULL, item.name, &v, QSE_NULL);
	if (!pair)
	{
		if (qse_xli_geterrnum(this->xli) != QSE_XLI_ENOENT) return -1;

		pair = qse_xli_findpair(this->xli, QSE_NULL, item.sctn);
		if (!pair)
		{
			pair = qse_xli_insertpairwithemptylist(this->xli, QSE_NULL, QSE_NULL, item.sctn, QSE_NULL, QSE_NULL);
			if (!pair) return -1;
		}

		QSE_ASSERT (pair->val != QSE_NULL);
		QSE_ASSERT (pair->val->type == QSE_XLI_LIST);

		if (!qse_xli_insertpairwithstr(this->xli, (qse_xli_list_t*)pair->val, QSE_NULL, item.key, QSE_NULL, QSE_NULL, &v, QSE_NULL)) return -1;
	}

	return 0;
}

int SkvEnv::load (const qse_char_t* path)
{
	qse_xli_t* xli;

	if (!this->xli)
	{
		// this means that no items have been registered with
		// this->addItem().
		return -1;
	}

	xli = qse_xli_openstdwithmmgr (this->getMmgr(), 0, 0, QSE_NULL);
	if (!xli) return -1;

	qse_xli_setopt (xli, QSE_XLI_KEYSPLITTER, splitter);

	qse_xli_iostd_t in;
	QSE_MEMSET (&in, 0, QSE_SIZEOF(in));
	in.type = QSE_XLI_IOSTD_FILE;
	in.u.file.path = path;

	if (qse_xli_readinistd(xli, &in) <= -1)
	{
		qse_xli_close (xli);
		return -1;
	}

	for (ItemList::Node* np = this->item_list.getHeadNode(); np; np = np->getNextNode())
	{
		Item& item = np->value;
		qse_xli_pair_t* pair;

		pair = qse_xli_findpair(xli, QSE_NULL, item.name);
		if (pair)
		{
			qse_xli_str_t* strv = (qse_xli_str_t*)pair->val;
			this->set_value_with_item (item, strv->ptr);
			// ignore failure.
		}
	}

	qse_xli_close(xli);
	return 0;
}

int SkvEnv::store (const qse_char_t* path)
{
	if (!this->xli) return -1;

	qse_xli_iostd_t out;

	QSE_MEMSET (&out, 0, QSE_SIZEOF(out));
	out.type = QSE_XLI_IOSTD_FILE;
	out.u.file.path = path;

	return qse_xli_writeinistd(this->xli, QSE_NULL, &out);
}

QSE_END_NAMESPACE(QSE)
