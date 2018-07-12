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


#include <qsep/xli/SkvEnv.hpp>
#include <qse/cmn/str.h>

#define SCTN_KEY_SPLITTER QSE_T('*')

QSE_BEGIN_NAMESPACE(QSE)

const qse_char_t* SkvEnv::setItemValue (const qse_char_t* name, const qse_char_t* val)
{
	qse_char_t* sctn, * key;
	qse_size_t sctn_len, key_len;

	if (this->split_name(name, &sctn, &sctn_len, &key, &key_len) == -1) return QSE_NULL;

	for (ItemList::Node* np = item_list.head(); np; np = np->forward()) 
	{
		Item& item = np->value;
		if (qse_strxcmp(sctn, sctn_len, item.sctn) == 0 &&
		    qse_strxcmp(key, key_len, item.key)  == 0) 
		{
			if ((this->*item.probe)(val) == -1) return QSE_NULL;
			this->setValue (item.sctn, item.key, val);
			return val;
		}
	}

	return QSE_NULL;
}



int SkvEnv::split_name (const qse_char_t* name, qse_char_t** sctn, qse_size_t* sctn_len, qse_char_t** key, qse_size_t* key_len)
{
	QSE_ASSERT (name     != QSE_NULL);
	QSE_ASSERT (sctn     != QSE_NULL);
	QSE_ASSERT (sctn_len != QSE_NULL);
	QSE_ASSERT (key      != QSE_NULL);
	QSE_ASSERT (key_len  != QSE_NULL);

	qse_char_t* s, * k, * p;
	qse_size_t sl, kl;

	qse_char_t spr[] = { SCTN_KEY_SPLITTER, QSE_CHAR('\0') };
	p = qse_strtok(name, spr, &s, &sl);
	if (!p || sl == 0) return -1;
	qse_strtok(p, QSE_NULL, &k, &kl);
	if (kl == 0) return -1;

	*sctn     = s;
	*sctn_len = sl;
	*key      = k;
	*key_len  = kl;

	return 0;
}

QSE_END_NAMESPACE(QSE)
