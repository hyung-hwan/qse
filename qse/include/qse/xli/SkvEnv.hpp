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

#ifndef _QSE_LIB_XLI_SKVENV_CLASS_
#define _QSE_LIB_XLI_SKVENV_CLASS_

#include <qse/cmn/LinkedList.hpp>
#include <qse/xli/xli.h>

QSE_BEGIN_NAMESPACE(QSE)

class SkvEnv: public Uncopyable, public Mmged
{
public:
	enum 
	{
		MAX_SCTN_LEN = 64,
		MAX_KEY_LEN  = 64,
		// 64 for sctn, 64 for key, 1 for splitter
		MAX_NAME_LEN = 129,
		// the default value can't be longer than this
		MAX_DVAL_LEN = 256
	};

	typedef int (SkvEnv::*ProbeProc) (const qse_char_t* val);
	struct Item 
	{
		qse_char_t sctn[MAX_SCTN_LEN + 1];
		qse_char_t key [MAX_KEY_LEN  + 1];
		qse_char_t name[MAX_NAME_LEN + 1]; // sctn*key
		qse_char_t dval[MAX_DVAL_LEN + 1]; // default value
		ProbeProc probe;
	};
	typedef QSE::LinkedList<Item> ItemList;

	SkvEnv (Mmgr* mmgr = QSE_NULL);
	virtual ~SkvEnv ();

	int addItem (const qse_char_t* name, const qse_char_t* dval, ProbeProc probe = QSE_NULL);
	int removeItem (const qse_char_t* name);
	const ItemList& getItemList () const { return this->item_list; }

	const qse_char_t* getValue (const qse_char_t* name) const;
	int setValue (const qse_char_t* name, const qse_char_t* value);

	int load (const qse_char_t* file);
	int store (const qse_char_t* file);

protected:
	ItemList item_list;
	qse_xli_t* xli;

	int split_name (const qse_char_t* name, qse_char_t** sctn, qse_size_t* sctn_len, qse_char_t** key, qse_size_t* key_len);
	int set_value_with_item (Item& item, const qse_char_t* value);

	virtual int probe_item_value (const Item& item, const qse_char_t* value) 
	{
		if (item.probe && (this->*item.probe)(value) <= -1) return -1;
		return 0;
	}

	int call_probe_item_value (const Item& item, const qse_char_t* value) 
	{
		try { return this->probe_item_value(item, value); }
		catch (...) { return -1; }
	}
};

QSE_END_NAMESPACE(QSE)

#endif
