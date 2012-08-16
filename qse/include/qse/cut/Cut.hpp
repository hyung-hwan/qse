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

#ifndef _QSE_CUT_CUT_HPP_
#define _QSE_CUT_CUT_HPP_

#include <qse/cmn/Mmged.hpp>
#include <qse/cut/cut.h>

/** @file
 * Stream Editor
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Cut class implements a stream editor by wrapping around #qse_cut_t.
 */
class Cut: public Mmged
{
public:
	/// The cut_t type redefines a stream editor type
	typedef qse_cut_t cut_t;
	/// The errnum_t type redefines an error number type
	typedef qse_cut_errnum_t errnum_t; 
	/// The errstr_t type redefines an error formattering string getter type
	typedef qse_cut_errstr_t errstr_t;
	/// The io_cmd_t type redefines an IO command type
	typedef qse_cut_io_cmd_t io_cmd_t;
	/// The io_arg_t type redefines an IO data type
	typedef qse_cut_io_arg_t io_arg_t;
	/// The option_t type redefines an option type
	typedef qse_cut_option_t option_t;

	///
	/// The Stream class is a base class for I/O operation during
	/// execution.
	///
	class Stream: public Types
	{
	public:
		/// The Mode type defines I/O operation mode.
		enum Mode
		{
			READ,  ///< open for read
			WRITE  ///< open for write
		};

		class Data
		{
		public:
			friend class Cut;

		protected:
			Data (Cut* cut, Mode mode, io_arg_t* arg):
				cut (cut), mode (mode), arg (arg) {}

		public:
			Mode getMode() const { return mode; }
			void* getHandle () const { return arg->handle; }
			void setHandle (void* handle) { arg->handle = handle; }
			operator Cut* () const { return cut; }
			operator cut_t* () const { return cut->cut; }

		protected:
			Cut* cut;
			Mode mode;
			io_arg_t* arg;
		};

		Stream () {}
		virtual ~Stream () {}

		virtual int open (Data& io) = 0;
		virtual int close (Data& io) = 0;
		virtual ssize_t read (Data& io, char_t* buf, size_t len) = 0;
		virtual ssize_t write (Data& io, const char_t* buf, size_t len) = 0;

	private:
		Stream (const Stream&);
		Stream& operator= (const Stream&);
	};

	///
	/// The Cut() function creates an uninitialized stream editor.
	///
	Cut (Mmgr* mmgr): Mmged (mmgr), cut (QSE_NULL), dflerrstr (QSE_NULL) 
	{
	}

	///
	/// The ~Cut() function destroys a stream editor. 
	/// @note The close() function is not called by this destructor.
	///       To avoid resource leaks, You should call close() before
	///       a stream editor is destroyed if it has been initialized
	///       with open().
	///
	virtual ~Cut () {}

	///
	/// The open() function initializes a stream editor and makes it
	/// ready for subsequent use.
	/// @return 0 on success, -1 on failure.
	///
	int open ();

	///
	/// The close() function finalizes a stream editor.
	///
	void close ();

	///
	/// The compile() function compiles a null-terminated string pointed
	/// to by @a sptr.
	/// @return 0 on success, -1 on failure
	///
	int compile (
		const char_t* sptr ///< a pointer to a null-terminated string
	);

	///
	/// The compile() function compiles a string pointed to by @a sptr
	/// and of the length @a slen.
	///  @return 0 on success, -1 on failure
	///
	int compile (
		const char_t* sptr, ///< a pointer to a string 
		size_t slen         ///< the number of characters in the string
	);

	///
	/// The execute() function executes compiled commands over the I/O
	/// streams defined through I/O handlers
	/// @return 0 on success, -1 on failure
	///
	int execute (Stream& iostream);

	///
	/// The getOption() function gets the current options.
	/// @return 0 or current options ORed of #option_t enumerators.
	///
	int getOption () const;

	///
	/// The setOption() function sets options for a stream editor.
	/// The option code @a opt is 0 or OR'ed of #option_t enumerators.
	///
	void setOption (
		int opt ///< option code
	);

	///
	/// The getErrorMessage() function gets the description of the last 
	/// error occurred. It returns an empty string if the stream editor
	/// has not been initialized with the open() function.
	///
	const char_t* getErrorMessage() const;

	///
	/// The getErrorNumber() function gets the number of the last 
	/// error occurred. It returns QSE_CUT_ENOERR if the stream editor
	/// has not been initialized with the open() function.
	///
	errnum_t getErrorNumber () const;

	///
	/// The setError() function sets information on an error occurred.
	///
	void setError (
		errnum_t      num,             ///< error number
		const cstr_t* args = QSE_NULL  ///< string array for formatting
		                               ///  an error message
	);

protected:
	///
	/// The getErrorString() function returns an error formatting string
	/// for the error number @a num. A subclass wishing to customize
	/// an error formatting string may override this function.
	/// 
	virtual const char_t* getErrorString (
		errnum_t num ///< an error number
	) const;

protected:
	/// handle to a primitive cut object
	cut_t* cut;
	/// default error formatting string getter
	errstr_t dflerrstr; 
	/// I/O stream to read data from and write output to.
	Stream* iostream;

private:
	static ssize_t xin (
		cut_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len);
	static ssize_t xout (
		cut_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len);
	static const char_t* xerrstr (cut_t* s, errnum_t num);

private:
	Cut (const Cut&);
	Cut& operator= (const Cut&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
