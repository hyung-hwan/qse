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

#ifndef _QSE_SED_STDSED_HPP_
#define _QSE_SED_STDSED_HPP_

#include <qse/sed/Sed.hpp>
#include <qse/cmn/StdMmgr.hpp>
#include <qse/cmn/str.h>

///
/// @file
/// This file defines easier-to-use stream editor classes providing standard
/// memory management and I/O handling.
///

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The StdSed class inherits the Sed class, implements a standard
/// I/O stream class, and sets the default memory manager.
///
class QSE_EXPORT StdSed: public Sed
{
public:
	StdSed (Mmgr* mmgr = StdMmgr::getDFL()): Sed (mmgr) {}

	///
	/// The FileStream class implements a stream over input
	/// and output files.
	///
	class QSE_EXPORT FileStream: public Stream
	{
	public:
		FileStream (const char_t* infile = QSE_NULL,
		            const char_t* outfile = QSE_NULL,
		            qse_cmgr_t* cmgr = QSE_NULL): 
			infile(infile), outfile(outfile), cmgr(cmgr) 
		{
		}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* infile;
		const char_t* outfile;
		qse_cmgr_t*   cmgr;
	};

	///
	/// The StringStream class implements a stream over a string
	///
	class QSE_EXPORT StringStream: public Stream
	{
	public:
		StringStream (const char_t* in);
		StringStream (const char_t* in, size_t len);
		~StringStream ();

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

		const char_t* getInput (size_t* len = QSE_NULL) const;
		const char_t* getOutput (size_t* len = QSE_NULL) const;

	protected:
		struct
		{
			const char_t* ptr; 
			const char_t* end; 
			const char_t* cur;
		} in;

		struct
		{
			bool inited;
			qse_str_t buf;
		} out;
	};
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
