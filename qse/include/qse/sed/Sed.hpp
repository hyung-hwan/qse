/*
 * $Id: Sed.hpp 318 2009-12-18 12:34:42Z hyunghwan.chung $
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

#ifndef _QSE_SED_SED_HPP_
#define _QSE_SED_SED_HPP_

#include <qse/cmn/Mmged.hpp>
#include <qse/sed/sed.h>

/** @file
 * Stream Editor
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Sed class implements a stream editor by wrapping around #qse_sed_t.
 */
class Sed: public Mmged
{
public:
	/// The sed_t type redefines a stream editor type
	typedef qse_sed_t sed_t;
	/// The loc_t type redefines the location type	
	typedef qse_sed_loc_t loc_t;
	/// The errnum_t type redefines an error number type
	typedef qse_sed_errnum_t errnum_t; 
	/// The errstr_t type redefines an error formattering string getter type
	typedef qse_sed_errstr_t errstr_t;
	/// The io_cmd_t type redefines an IO command type
	typedef qse_sed_io_cmd_t io_cmd_t;
	/// The io_arg_t type redefines an IO data type
	typedef qse_sed_io_arg_t io_arg_t;
	/// The option_t type redefines an option type
	typedef qse_sed_option_t option_t;
	/// The depth_t type redefines an depth IDs
	typedef qse_sed_depth_t depth_t;

	///
	/// The IOStream class is a base class for I/O operation during
	/// execution.
	///
	class IOStream: public Types
	{
	public:
		enum Mode
		{
			READ,  ///< open for read
			WRITE  ///< open for write
		};

		class Data
		{
		public:
			friend class Sed;

		protected:
			Data (Sed* sed, Mode mode, io_arg_t* arg):
				sed (sed), mode (mode), arg (arg)
			{
			}

		public:
			Mode getMode() const
			{
				return mode;
			}

			void* getHandle () const
			{
				return arg->handle;
			}

			void setHandle (void* handle)
			{
				arg->handle = handle;
			}

			const char_t* getName () const
			{
				return arg->path;
			}

			operator Sed* () const
			{
				return sed;
			}

			operator sed_t* () const
			{
				return sed->sed;
			}

		protected:
			Sed* sed;
			Mode mode;
			io_arg_t* arg;
		};

		IOStream () {}
		virtual ~IOStream () {}

		virtual int open (Data& io) = 0;
		virtual int close (Data& io) = 0;
		virtual ssize_t read (Data& io, char_t* buf, size_t len) = 0;
		virtual ssize_t write (Data& io, const char_t* buf, size_t len) = 0;

	private:
		IOStream (const IOStream&);
		IOStream& operator= (const IOStream&);
	};

	///
	/// The Sed() function creates an uninitialized stream editor.
	///
	Sed (Mmgr* mmgr): Mmged (mmgr), sed (QSE_NULL), dflerrstr (QSE_NULL) {}

	///
	/// The ~Sed() function destroys a stream editor. 
	/// @note The close() function is not called by this destructor.
	///       To avoid resource leaks, You should call close() before
	///       a stream editor is destroyed if it has been initialized
	///       with open().
	///
	virtual ~Sed () {}

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
	int execute (IOStream& iostream);

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
	/// The getMaxDepth() function gets the maximum processing depth for
	/// an operation type identified by @a id.
	///
	size_t getMaxDepth (
		depth_t id ///< operation type
	) const;

	///
	/// The setMaxDepth() function gets the maximum processing depth.
	///
	void setMaxDepth (
		int    ids,  ///< 0 or a number OR'ed of depth_t values
		size_t depth ///< 0 maximum depth
	);

	///
	/// The getErrorMessage() function gets the description of the last 
	/// error occurred. It returns an empty string if the stream editor
	/// has not been initialized with the open() function.
	///
	const char_t* getErrorMessage() const;

	///
	/// The getErrorLocation() function gets the location where
	/// the last error occurred. The line and the column of the #loc_t 
	/// structure retruend are 0 if the stream editor has not been 
	/// initialized with the open() function.
	///
	loc_t getErrorLocation () const;

	///
	/// The getErrorNumber() function gets the number of the last 
	/// error occurred. It returns QSE_SED_ENOERR if the stream editor
	/// has not been initialized with the open() function.
	///
	errnum_t getErrorNumber () const;

	///
	/// The setError() function sets information on an error occurred.
	///
	void setError (
		errnum_t      num,             ///< error number
		const cstr_t* args = QSE_NULL, ///< string array for formatting
		                               ///  an error message
		const loc_t*  loc = QSE_NULL   ///< error location
	);

	///
	/// The getConsoleLine() function returns the current line
	/// number from an input console. 
	/// @return current line number
	///
	size_t getConsoleLine ();

	///
	/// The setConsoleLine() function changes the current line
	/// number from an input console. 
	///
	void setConsoleLine (
		size_t num ///< a line number
	);

	///
	/// The getMmgr() function returns the memory manager associated.
	///
	
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
	/// handle to a primitive sed object
	sed_t* sed;
	/// default error formatting string getter
	errstr_t dflerrstr; 
	/// I/O stream to read data from and write output to.
	IOStream* iostream;


private:
	static ssize_t xin (
		sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len);
	static ssize_t xout (
		sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len);
	static const char_t* xerrstr (sed_t* s, errnum_t num);

private:
	Sed (const Sed&);
	Sed& operator= (const Sed&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
