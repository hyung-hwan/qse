/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_SED_SED_HPP_
#define _QSE_SED_SED_HPP_

#include <qse/Mmgr.hpp>
#include <qse/sed/sed.h>

/**@file
 * A stream editor
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Sed class implements a stream editor.
 */
class Sed: public Mmgr
{
public:
	typedef qse_sed_t sed_t;
	typedef qse_sed_io_cmd_t sed_io_cmd_t;
	typedef qse_sed_io_arg_t sed_io_arg_t;

	/**
	 * The Sed() function creates a uninitialized stream editor.
	 */
	Sed () throw (): sed (QSE_NULL) {}

	/**
	 * The open() function initializes a stream editor and makes it
	 * ready for subsequent use.
	 * @return 0 on success, -1 on failure.
	 */
	int open () throw ();

	/**
	 * The close() function finalized a stream editor.
	 */
	void close () throw ();

	/**
	 * The compile() function compiles a null-terminated string pointed
	 * to by @a sptr.
	 * @return 0 on success, -1 on failure
	 */
	int compile (
		const char_t* sptr ///< a pointer to a null-terminated string
	) throw ();

	/**
	 * The compile() function compiles a string pointed to by @a sptr
	 * and of the length @a slen.
	 * @return 0 on success, -1 on failure
	 */
	int compile (
		const char_t* sptr, ///< a pointer to a string 
		size_t slen         ///< the number of characters in the string
	) throw ();

	/**
	 * The execute() function executes compiled commands over the IO
	 * streams defined through IO handlers
	 * @return 0 on success, -1 on failure
	 */
	int execute () throw ();

	/**
	 * The getErrorMessage() function gets the description of the last 
	 * error occurred. It returns an empty string if the stream editor
	 * has not been initialized with the open() function.
	 */
	const char_t* getErrorMessage() const;

	/**
	 * The getErrorLine() function gets the number of the line where
	 * the last error occurred. It returns 0 if the stream editor has 
	 * not been initialized with the open() function.
	 */
	size_t getErrorLine () const;

	/**
	 * The getErrorNumber() function gets the number of the last 
	 * error occurred. It returns 0 if the stream editor has not been
	 * initialized with the open function.
	 */
	int getErrorNumber () const;

protected:
	/**
	 * The IO class is a base class for IO operation. It wraps around the 
	 * qse_sed_io_arg_t type and exposes relevant information to
	 * an IO handler 
	 */
	class IO
	{
	public:
		/** 
		 * The Mode enumerator defines IO operation modes.
		 */
		enum Mode
		{
			READ, ///< open a stream for reading
			WRITE ///< open a stream for writing
		};

	protected:
		IO (sed_io_arg_t* arg, Mode mode): arg(arg), mode (mode) {}

	public:
		/**
		 * The getHandle() function gets an IO handle set with the
		 * setHandle() function. Once set, it is maintained until
		 * an assoicated IO handler closes it or changes it with
		 * another call to setHandle().
		 */
		const void* getHandle () const
		{
			return arg->handle;
		}

		/**
		 * The setHandle() function sets an IO handle and is typically
		 * called in stream opening functions such as Sed::openConsole()
		 * and Sed::openFile(). You can get the handle with the 
		 * getHandle() function as needed.
		 */
		void setHandle (void* handle)
		{
			arg->handle = handle;
		}		

		/**
		 * The getMode() function gets the IO operation mode requested.
		 * A stream opening function can inspect the mode requested and
		 * open a stream properly 
		 */
		Mode getMode ()
		{
			return this->mode;
		}

	protected:
		sed_io_arg_t* arg;
		Mode mode;
	};

	/**
	 * The Console class inherits the IO class and provides functionality 
	 * for console IO operations.
	 */
	class Console: public IO
	{
	protected:
		friend class Sed;
		Console (sed_io_arg_t* arg, Mode mode): IO (arg, mode) {}
	};

	/**
	 * The File class inherits the IO class and provides functionality
	 * for file IO operations.
	 */
	class File: public IO
	{
	protected:
		friend class Sed;
		File (sed_io_arg_t* arg, Mode mode): IO (arg, mode) {}

	public:
		/**
		 * The getName() function gets the file path requested.
		 * You can call this function from the openFile() function
		 * to determine a file to open.
		 */
		const char_t* getName () const
		{
			return arg->path;
		}
	};

	/**
	 * The openConsole() function should be implemented by a subclass
	 * to open a console
	 */
	virtual int openConsole (Console& io) = 0;
	virtual int closeConsole (Console& io) = 0;
	virtual ssize_t readConsole (
		Console& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeConsole (
		Console& io, const char_t* data, size_t len) = 0;

	virtual int openFile (File& io) = 0;
	virtual int closeFile (File& io) = 0;
	virtual ssize_t readFile (
		File& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeFile (
		File& io, const char_t* data, size_t len) = 0;

protected:
	sed_t* sed;

private:
	static ssize_t xin (
		sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg) throw ();
	static ssize_t xout (
		sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg) throw ();
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
