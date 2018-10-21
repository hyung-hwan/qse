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

#ifndef _QSE_STTP_STTPCMD_CLASS_
#define _QSE_STTP_STTPCMD_CLASS_

#include <qse/cmn/String.hpp>
#include <qse/cmn/Array.hpp>

QSE_BEGIN_NAMESPACE(QSE)

class SttpCmd: public QSE::Array<QSE::String>
{
public:
	SttpCmd (const qse_char_t* n = QSE_T(""), QSE::Mmgr* mmgr = QSE_NULL): 
		QSE::Array<QSE::String>(mmgr, 20), name (n) {}

	SttpCmd (const QSE::String& n, QSE::Mmgr* mmgr = QSE_NULL): 
		QSE::Array<QSE::String>(mmgr, 20), name (n) {}

	qse_size_t getArgCount () const
	{
		return this->getSize ();
	}

	const qse_char_t* getArgAt (qse_size_t i) const
	{
		return this->getValueAt(i).getBuffer();
	}

	qse_size_t getArgLenAt (qse_size_t i) const
	{
		return this->getValueAt(i).getSize();
	}

	void addArg (const qse_char_t* n, qse_size_t size)
	{
		this->insertLast (QSE::String(n, size));
	}

	void setName (const qse_char_t* n, qse_size_t size)
	{
		this->name.truncate (0);
		this->name.append (n, size);
	}

	const QSE::String& getName () const
	{
		return this->name;
	}

	bool isNullCmd () const
	{
		return this->name.getSize() == 0;
	}

	QSE::String name;
};

QSE_END_NAMESPACE(QSE)

#endif
