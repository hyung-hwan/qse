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
	/// Type sed_t type redefines a stream editor type
	typedef qse_sed_t sed_t;
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

	/**
	 * The Sed() function creates an uninitialized stream editor.
	 */
	Sed () throw (): sed (QSE_NULL), dflerrstr (QSE_NULL) {}

	/**
	 * The ~Sed() function destroys a stream editor. 
	 * @note The close() function is not called by this destructor.
	 *       To avoid resource leaks, You should call close() before
	 *       a stream editor is destroyed if it has been initialized
	 *       with open().
	 */
	~Sed () {}

	/**
	 * The open() function initializes a stream editor and makes it
	 * ready for subsequent use.
	 * @return 0 on success, -1 on failure.
	 */
	int open () throw ();

	/**
	 * The close() function finalizes a stream editor.
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
	 * The getOption() function gets the current options.
	 * @return current option code
	 */
	int getOption () const throw ();

	/**
	 * The setOption() function sets options for a stream editor.
	 * The option code @a opt is 0 or OR'ed of #option_t enumerators.
	 */
	void setOption (
		int opt ///< option code
	) throw ();

	/**
	 * The getMaxDepth() function gets the maximum processing depth.
	 */
	size_t getMaxDepth (depth_t id) const throw ();

	/**
	 * The setMaxDepth() function gets the maximum processing depth.
	 */
	void setMaxDepth (
		int    ids,  ///< 0 or a number OR'ed of depth_t values
		size_t depth ///< 0 maximum depth
	) throw ();

	/**
	 * The getErrorMessage() function gets the description of the last 
	 * error occurred. It returns an empty string if the stream editor
	 * has not been initialized with the open() function.
	 */
	const char_t* getErrorMessage() const throw ();

	/**
	 * The getErrorLine() function gets the number of the line where
	 * the last error occurred. It returns 0 if the stream editor has 
	 * not been initialized with the open() function.
	 */
	size_t getErrorLine () const throw ();

	/**
	 * The getErrorNumber() function gets the number of the last 
	 * error occurred. It returns QSE_SED_ENOERR if the stream editor
	 * has not been initialized with the open() function.
	 */
	errnum_t getErrorNumber () const throw ();

	/**
	 * The getConsoleLine() function returns the current line
	 * number from an input console. 
	 * @return current line number
	 */
	size_t getConsoleLine () throw ();

	/**
	 * The setError() function sets information on an error occurred.
	 */
	void setError (
		errnum_t      err,            ///< an error number
		size_t        lin = 0,        ///< a line number
		const cstr_t* args = QSE_NULL ///< strings for formatting an error message
	);

	/**
	 * The setConsoleLine() function changes the current line
	 * number from an input console. 
	 */
	void setConsoleLine (
		size_t num ///< a line number
	) throw ();

protected:
	/**
	 * The IOBase class is a base class for IO operation. It wraps around
	 * the primitive Sed::io_arg_t type and exposes relevant information to
	 * an IO handler 
	 */
	class IOBase
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
		IOBase (io_arg_t* arg, Mode mode) throw (): 
			arg(arg), mode (mode) {}

	public:
		/**
		 * The getHandle() function gets an IO handle set with the
		 * setHandle() function. Once set, it is maintained until
		 * an assoicated IO handler closes it or changes it with
		 * another call to setHandle().
		 */
		const void* getHandle () const throw ()
		{
			return arg->handle;
		}

		/**
		 * The setHandle() function sets an IO handle and is typically
		 * called in stream opening functions such as Sed::openConsole()
		 * and Sed::openFile(). You can get the handle with the 
		 * getHandle() function as needed.
		 */
		void setHandle (void* handle) throw ()
		{
			arg->handle = handle;
		}		

		/**
		 * The getMode() function gets the IO operation mode requested.
		 * A stream opening function can inspect the mode requested and
		 * open a stream properly 
		 */
		Mode getMode () throw ()
		{
			return this->mode;
		}

	protected:
		Sed*      sed;
		io_arg_t* arg;
		Mode      mode;
	};

	/**
	 * The Console class inherits the IO class and provides functionality 
	 * for console IO operations.
	 */
	class Console: public IOBase
	{
	protected:
		friend class Sed;
		Console (io_arg_t* arg, Mode mode) throw ():
			IOBase (arg, mode) {}
	};

	/**
	 * The File class inherits the IO class and provides functionality
	 * for file IO operations.
	 */
	class File: public IOBase
	{
	protected:
		friend class Sed;
		File (io_arg_t* arg, Mode mode) throw ():
			IOBase (arg, mode) {}

	public:
		/**
		 * The getName() function gets the file path requested.
		 * You can call this function from the openFile() function
		 * to determine a file to open.
		 */
		const char_t* getName () const throw ()
		{
			return arg->path;
		}
	};

	/**
	 * The openConsole() function should be implemented by a subclass
	 * to open a console. It can get the mode requested by invoking
	 * the Console::getMode() function over the console object @a io.
	 *
	 * When it comes to the meaning of the return value, 0 may look
	 * a bit tricky. Easygoers can just return 1 on success and never
	 * return 0 from openConsole().
	 * - If 0 is returned for a Console::READ console, the execute()
	 * function returns success after having calle closeConsole() as it 
	 * has opened a console but has reached EOF.
	 * - If 0 is returned for a Console::WRITE console and there are any
	 * following writeConsole() requests, the execute() function
	 * returns failure after having called closeConsole() as it cannot
	 * write further on EOF.
	 *
	 * @return -1 on failure, 1 on success, 0 on success but reached EOF.
	 */
	virtual int openConsole (
		Console& io ///< a console object
	) = 0;

	/**
	 * The closeConsole() function should be implemented by a subclass
	 * to close a console.
	 */
	virtual int closeConsole (
		Console& io ///< a console object
	) = 0;

	/**
	 * The readConsole() function should be implemented by a subclass
	 * to read from a console. It should fill the memory area pointed to
	 * by @a buf, but at most \a len characters.
	 * @return the number of characters read on success, 
	 *         0 on EOF, -1 on failure
	 */
	virtual ssize_t readConsole (
		Console& io,  ///< a console object
		char_t*  buf, ///< a buffer pointer 
		size_t   len  ///< the size of a buffer
	) = 0;

	/**
	 * The writeConsole() function should be implemented by a subclass
	 * to write to a console. It should write up to @a len characters
	 * from the memory are pointed to by @a data.
	 * @return the number of characters written on success
	 *         0 on EOF, -1 on failure
	 * @note The number of characters written may be less than @a len.
	 *       But the return value 0 causes execute() to fail as
	 *       writeConsole() is called when there are data to write and
	 *       it has indicated EOF.
	 */
	virtual ssize_t writeConsole (
		Console&      io,    ///< a console object
		const char_t* data,  ///< a pointer to data to write
		size_t        len    ///< the length of data
	) = 0;

	/**
	 * The openFile() function should be implemented by a subclass
	 * to open a file. It can get the mode requested by invoking
	 * the File::getMode() function over the file object @a io.
	 * @return -1 on failure, 1 on success, 0 on success but reached EOF.
	 */
	virtual int openFile (
		File& io    ///< a file object
	) = 0;

	/**
	 * The closeFile() function should be implemented by a subclass
	 * to close a file.
	 */
	virtual int closeFile (
		File& io    ///< a file object
	) = 0;

	/**
	 * The readFile() function should be implemented by a subclass
	 * to read from a file. It should fill the memory area pointed to
	 * by @a buf, but at most \a len characters.
	 * @return the number of characters read on success, 
	 *         0 on EOF, -1 on failure
	 */
	virtual ssize_t readFile (
		File& io,     ///< a file object
		char_t*  buf, ///< a buffer pointer 
		size_t   len  ///< the size of a buffer
	) = 0;

	/**
	 * The writeFile() function should be implemented by a subclass
	 * to write to a file. It should write up to @a len characters
	 * from the memory are pointed to by @a data.
	 * @return the number of characters written on success
	 *         0 on EOF, -1 on failure
	 * @note The number of characters written may be less than @a len.
	 *       But the return value 0 causes execute() to fail as
	 *       writeFile() is called when there are data to write and
	 *       it has indicated EOF.
	 */
	virtual ssize_t writeFile (
		File& io,            ///< a file object
		const char_t* data,  ///< a pointer to data to write
		size_t        len    ///< the length of data
	) = 0;

	/**
	 * The getErrorString() function returns an error formatting string
	 * for the error number @a num. A subclass wishing to customize
	 * an error formatting string may override this function.
	 */ 
	virtual const char_t* getErrorString (
		errnum_t num ///< an error number
	);

protected:
	/// handle to a primitive sed object
	sed_t* sed;
	/// default error formatting string getter
	errstr_t dflerrstr; 

private:
	static ssize_t xin (sed_t* s, io_cmd_t cmd, io_arg_t* arg) throw ();
	static ssize_t xout (sed_t* s, io_cmd_t cmd, io_arg_t* arg) throw ();
	static const char_t* xerrstr (sed_t* s, errnum_t num) throw ();
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
