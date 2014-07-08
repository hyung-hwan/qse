/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

/// \file
/// This file defines C++ classes that you can use when you create a stream
/// editor. The C++ classes encapsulates the C data types and functions in 
/// a more object-oriented manner.
///
/// \todo support sed tracer
///

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The Sed class implements a stream editor by wrapping around #qse_sed_t.
///
class QSE_EXPORT Sed: public Mmged
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
	/// The trait_t type redefines an trait type
	typedef qse_sed_trait_t trait_t;
#if 0
	/// The depth_t type redefines an depth IDs
	typedef qse_sed_depth_t depth_t;
#endif

	///
	/// The Stream class is a abstract class for I/O operation during
	/// execution.
	///
	class QSE_EXPORT Stream: public Types
	{
	public:
		/// The Mode type defines I/O modes.
		enum Mode
		{
			READ,  ///< open for read
			WRITE  ///< open for write
		};

		/// The Data class conveys information need for I/O operations. 
		class QSE_EXPORT Data
		{
		public:
			friend class Sed;

		protected:
			Data (Sed* sed, Mode mode, io_arg_t* arg):
				sed (sed), mode (mode), arg (arg) {}

		public:
			/// The getMode() function returns the I/O mode
			/// requested.
			Mode getMode() const { return this->mode; }

			/// The getHandle() function returns the I/O handle 
			/// saved by setHandle().
			void* getHandle () const { return this->arg->handle; }

			/// The setHandle() function sets an I/O handle
			/// typically in the Stream::open() function. 
			void setHandle (void* handle) { this->arg->handle = handle; }

			/// The getName() function returns an I/O name.
			/// \return #QSE_NULL for the main data stream,
			///         file path for explicit file stream
			const char_t* getName () const { return this->arg->path; }

			/// The Sed* operator returns the associated Sed class.
			operator Sed* () const { return this->sed; }

			/// The sed_t* operator returns a pointer to the 
			/// underlying stream editor.
			operator sed_t* () const { return this->sed->getHandle(); }

		protected:
			Sed* sed;
			Mode mode;
			io_arg_t* arg;
		};

		/// The Stream() function constructs a stream.
		Stream () {}

		/// The Stream() function destructs a stream.
		virtual ~Stream () {}

		/// The open() function should be implemented by a subclass
		/// to open a stream. It can get the mode requested by calling
		/// the Data::getMode() function over the I/O parameter \a io.
		///
		/// The return value of 0 may look a bit tricky. Easygoers 
		/// can just return 1 on success and never return 0 from open().
		/// - If 0 is returned for the #READ mode, Sed::execute()
		///   returns success after having called close() as it has
		///   opened a console but has reached EOF.
		/// - If 0 is returned for the #WRITE mode and there are
		///   any write() calls, the Sed::execute() function returns
		///   failure after having called close() as it cannot write
		///   further on EOF.
		///
		/// \return -1 on failure, 1 on success, 
		///         0 on success but reached EOF.
		virtual int open (Data& io) = 0;

		/// The close() function should be implemented by a subclass
		/// to open a stream.
		virtual int close (Data& io) = 0;

		virtual ssize_t read (Data& io, char_t* buf, size_t len) = 0;
		virtual ssize_t write (Data& io, const char_t* buf, size_t len) = 0;

	private:
		Stream (const Stream&);
		Stream& operator= (const Stream&);
	};

	///
	/// The Sed() function creates an uninitialized stream editor.
	///
	Sed (Mmgr* mmgr): Mmged (mmgr), sed (QSE_NULL), dflerrstr (QSE_NULL) {}

	///
	/// The ~Sed() function destroys a stream editor. 
	/// \note The close() function is not called by this destructor.
	///       To avoid resource leaks, You should call close() before
	///       a stream editor is destroyed if it has been initialized
	///       with open().
	///
	virtual ~Sed () {}

	///
	/// The open() function initializes a stream editor and makes it
	/// ready for subsequent use.
	/// \return 0 on success, -1 on failure.
	///
	int open ();

	///
	/// The close() function finalizes a stream editor.
	///
	void close ();

	///
	/// The compile() function compiles a script from a stream 
	/// \a iostream.
	/// \return 0 on success, -1 on failure
	///
	int compile (Stream& sstream);

	///
	/// The execute() function executes compiled commands over the I/O
	/// streams defined through I/O handlers
	/// \return 0 on success, -1 on failure
	///
	int execute (Stream& iostream);

	///
	/// The stop() function makes a request to break a running loop
	/// inside execute(). Note that this does not affect blocking 
	/// operations in user-defined stream handlers.
	///
	void stop ();

	///
	/// The isStop() function returns true if stop() has been called
	/// since the last call to execute(), false otherwise.
	///
	bool isStop () const;

	///
	/// The getTrait() function gets the current traits.
	/// \return 0 or current options ORed of #trait_t enumerators.
	///
	int getTrait () const;

	///
	/// The setTrait() function sets traits for a stream editor.
	/// The option code \a opt is 0 or OR'ed of #trait_t enumerators.
	///
	void setTrait (
		int trait ///< option code
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

	const char_t* getCompileId () const;

	const char_t* setCompileId (
		const char_t* id
	);

	///
	/// The getConsoleLine() function returns the current line
	/// number from an input console. 
	/// \return current line number
	///
	size_t getConsoleLine ();

	///
	/// The setConsoleLine() function changes the current line
	/// number from an input console. 
	///
	void setConsoleLine (
		size_t num ///< a line number
	);

protected:
	///
	/// The getErrorString() function returns an error formatting string
	/// for the error number \a num. A subclass wishing to customize
	/// an error formatting string may override this function.
	/// 
	virtual const char_t* getErrorString (
		errnum_t num ///< an error number
	) const;

public:
	// use this with care
	sed_t* getHandle() const { return this->sed; }

protected:
	/// handle to a primitive sed object
	sed_t* sed;
	/// default error formatting string getter
	errstr_t dflerrstr; 
	/// Stream to read script from
	Stream* sstream;
	/// I/O stream to read data from and write output to.
	Stream* iostream;


private:
	static ssize_t sin (
		sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len);
	static ssize_t xin (
		sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len);
	static ssize_t xout (
		sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len);
	static const char_t* xerrstr (const sed_t* s, errnum_t num);

private:
	Sed (const Sed&);
	Sed& operator= (const Sed&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
