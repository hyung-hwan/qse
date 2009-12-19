/**
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <qse/cmn/sio.h>

#include <string>
#include <iostream>

#ifdef QSE_CHAR_IS_MCHAR
#	define xcout std::cout
#else
#	define xcout std::wcout
#endif

//
// The StringStream class implements a data I/O stream over strings.
//
class StringStream: public QSE::StdSed::IOStream
{
public:
	StringStream (const char_t* in) { this->in.ptr = in; }
	
	int open (Data& io)
	{
		const char_t* ioname = io.getName ();

		if (ioname == QSE_NULL)
		{
			// open a main data stream
			if (io.getMode() == READ) 
			{
				in.cur = in.ptr;
				io.setHandle ((void*)in.ptr);
			}
			else
			{
				out.erase ();
				io.setHandle (&out);
			}
		}
		else
		{
			// open files for a r or w command
			qse_sio_t* sio;
			int mode = (io.getMode() == READ)?
				QSE_SIO_READ: 
				(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE);

			sio = qse_sio_open (((QSE::Sed*)io)->getMmgr(), 0, ioname, mode);
			if (sio == QSE_NULL) return -1;

			io.setHandle (sio);
		}

		return 1;
	}

	int close (Data& io)
	{
		const void* handle = io.getHandle();
		if (handle != in.ptr && handle != &out)
			qse_sio_close ((qse_sio_t*)handle);
		return 0;
	}

	ssize_t read (Data& io, char_t* buf, size_t len)
	{
		const void* handle = io.getHandle();
		if (handle == in.ptr)
		{
			ssize_t n = qse_strxcpy (buf, len, in.cur);
			in.cur += n; return n;
		}
		else
		{
			QSE_ASSERT (handle != &out);
			return qse_sio_getsn ((qse_sio_t*)handle, buf, len);
		}
	}

	ssize_t write (Data& io, const char_t* buf, size_t len)
	{
		const void* handle = io.getHandle();

		if (handle == &out)
		{
			try 
			{
				out.append (buf, len);
				return len; 
			}
			catch (...) 
			{ 
				((QSE::Sed*)io)->setError (QSE_SED_ENOMEM); 
				throw;
			}
		}
		else
		{
			QSE_ASSERT (handle != in.ptr);
			return qse_sio_putsn ((qse_sio_t*)handle, buf, len);
		}
	}

	const char_t* getInput () const { return in.ptr; }
	const char_t* getOutput () const { return out.c_str (); }

protected:
	struct
	{
		const char_t* ptr; 
		const char_t* cur;
	} in;

	std::basic_string<char_t> out;
};

//
// The MySed class simplifies QSE::StdSed by utilizing exception handling.
//
class MySed: protected QSE::StdSed
{
public:
	
	class Error
	{ 
	public:
		Error (const char_t* msg) throw (): msg (msg) {}
		const char_t* getMessage() const throw() { return msg; }
	protected:
		const char_t* msg;
	};

	MySed () { if (open() <= -1) throw Error (QSE_T("cannot open")); }
	~MySed () { close (); }

	void compile (const char_t* sptr)
	{
		if (QSE::StdSed::compile (sptr) <= -1)
			throw Error (getErrorMessage());
	}

	void execute (IOStream& stream)
	{
		if (QSE::StdSed::execute (stream) <= -1)
			throw Error (getErrorMessage());
	}
};

int sed_main (int argc, qse_char_t* argv[])
{
	try
	{
		MySed sed;

		sed.compile (QSE_T("y/ABC/abc/;s/abc/def/g"));

		StringStream stream (QSE_T("ABCDEFabcdef"));
		sed.execute (stream);

		xcout << QSE_T("INPUT: ") << stream.getInput() << std::endl;
		xcout << QSE_T("OUTPUT: ") << stream.getOutput() << std::endl;
	}
	catch (MySed::Error& err)
	{
		xcout << QSE_T("ERROR: ") << err.getMessage() << std::endl;
		return -1;
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
