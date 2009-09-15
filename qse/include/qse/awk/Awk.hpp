/*
 * $Id: Awk.hpp 286 2009-09-14 13:29:55Z hyunghwan.chung $
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

#ifndef _QSE_AWK_AWK_HPP_
#define _QSE_AWK_AWK_HPP_

#include <qse/awk/awk.h>
#include <qse/cmn/map.h>
#include <qse/cmn/chr.h>
#include <qse/Mmgr.hpp>
#include <stdarg.h>

/** @file
 * AWK Interpreter
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/** 
 * Represents the AWK interpreter engine.
 */
class Awk: public Mmgr
{
public:
	typedef qse_map_t   map_t;
	typedef qse_map_pair_t pair_t;

	/** Defines a primitive handle */
	typedef qse_awk_t awk_t;

	typedef qse_awk_loc_t loc_t;
	typedef qse_awk_errnum_t errnum_t;
	typedef qse_awk_errstr_t errstr_t;
	typedef qse_awk_errinf_t errinf_t;

	typedef qse_awk_depth_t depth_t;

	/** Represents an internal awk value */
	typedef qse_awk_val_t val_t;

	/** Represents a runtime context */
	typedef qse_awk_rtx_t rtx_t;

	/** Represents an runtime I/O data */
	typedef qse_awk_rio_arg_t rio_arg_t;

	typedef qse_awk_rio_cmd_t rio_cmd_t;

	typedef qse_awk_sio_arg_t sio_arg_t;

	typedef qse_awk_sio_cmd_t sio_cmd_t;

	class Run;
	friend class Run;

	/**
	 * @name Error Handling
	 */
	/*@{*/

	///
	/// The ErrorNumber defines error numbers by redefining enumerators
	/// of the #qse_awk_errnum_t type.
	///
	enum ErrorNumber
	{
		ERR_NOERR = QSE_AWK_ENOERR,
		ERR_UNKNOWN = QSE_AWK_EUNKNOWN,
		ERR_INVAL = QSE_AWK_EINVAL,
		ERR_NOMEM = QSE_AWK_ENOMEM,
		ERR_NOSUP = QSE_AWK_ENOSUP,
		ERR_NOPER = QSE_AWK_ENOPER,
		ERR_NOENT = QSE_AWK_ENOENT, 
		ERR_EXIST = QSE_AWK_EEXIST,
		ERR_IOERR = QSE_AWK_EIOERR,
		ERR_OPEN = QSE_AWK_EOPEN,
		ERR_READ = QSE_AWK_EREAD,
		ERR_WRITE = QSE_AWK_EWRITE,
		ERR_CLOSE = QSE_AWK_ECLOSE,
		ERR_INTERN = QSE_AWK_EINTERN,
		ERR_RUNTIME = QSE_AWK_ERUNTIME,
		ERR_BLKNST = QSE_AWK_EBLKNST,
		ERR_EXPRNST = QSE_AWK_EEXPRNST,
		ERR_LXCHR = QSE_AWK_ELXCHR,
		ERR_LXDIG = QSE_AWK_ELXDIG,
		ERR_EOF = QSE_AWK_EEOF,
		ERR_CMTNC = QSE_AWK_ECMTNC,
		ERR_STRNC = QSE_AWK_ESTRNC,
		ERR_LBRACE = QSE_AWK_ELBRACE,
		ERR_LPAREN = QSE_AWK_ELPAREN,
		ERR_RPAREN = QSE_AWK_ERPAREN,
		ERR_RBRACK = QSE_AWK_ERBRACK,
		ERR_COMMA = QSE_AWK_ECOMMA,
		ERR_SCOLON = QSE_AWK_ESCOLON,
		ERR_COLON = QSE_AWK_ECOLON,
		ERR_STMEND = QSE_AWK_ESTMEND,
		ERR_KWIN = QSE_AWK_EKWIN,
		ERR_NOTVAR = QSE_AWK_ENOTVAR,
		ERR_EXPRNR = QSE_AWK_EEXPRNR,
		ERR_KWFNC = QSE_AWK_EKWFNC,
		ERR_KWWHL = QSE_AWK_EKWWHL,
		ERR_ASSIGN = QSE_AWK_EASSIGN,
		ERR_IDENT = QSE_AWK_EIDENT,
		ERR_FUNNAME = QSE_AWK_EFUNNAME,
		ERR_BLKBEG = QSE_AWK_EBLKBEG,
		ERR_BLKEND = QSE_AWK_EBLKEND,
		ERR_KWRED = QSE_AWK_EKWRED,
		ERR_FNCRED = QSE_AWK_EFNCRED,
		ERR_FUNRED = QSE_AWK_EFUNRED,
		ERR_GBLRED = QSE_AWK_EGBLRED,
		ERR_PARRED = QSE_AWK_EPARRED,
		ERR_VARRED = QSE_AWK_EVARRED,
		ERR_DUPPAR = QSE_AWK_EDUPPAR,
		ERR_DUPGBL = QSE_AWK_EDUPGBL,
		ERR_DUPLCL = QSE_AWK_EDUPLCL,
		ERR_BADPAR = QSE_AWK_EBADPAR,
		ERR_BADVAR = QSE_AWK_EBADVAR,
		ERR_UNDEF = QSE_AWK_EUNDEF,
		ERR_LVALUE = QSE_AWK_ELVALUE,
		ERR_GBLTM = QSE_AWK_EGBLTM,
		ERR_LCLTM = QSE_AWK_ELCLTM,
		ERR_PARTM = QSE_AWK_EPARTM,
		ERR_DELETE = QSE_AWK_EDELETE,
		ERR_BREAK = QSE_AWK_EBREAK,
		ERR_CONTINUE = QSE_AWK_ECONTINUE,
		ERR_NEXTBEG = QSE_AWK_ENEXTBEG,
		ERR_NEXTEND = QSE_AWK_ENEXTEND,
		ERR_NEXTFBEG = QSE_AWK_ENEXTFBEG,
		ERR_NEXTFEND = QSE_AWK_ENEXTFEND,
		ERR_PRINTFARG = QSE_AWK_EPRINTFARG,
		ERR_PREPST = QSE_AWK_EPREPST,
		ERR_INCDECOPR = QSE_AWK_EINCDECOPR,
		ERR_INCLSTR = QSE_AWK_EINCLSTR,
		ERR_INCLTD = QSE_AWK_EINCLTD,
		ERR_DIRECNR = QSE_AWK_EDIRECNR,
		ERR_DIVBY0 = QSE_AWK_EDIVBY0,
		ERR_OPERAND = QSE_AWK_EOPERAND,
		ERR_POSIDX = QSE_AWK_EPOSIDX,
		ERR_ARGTF = QSE_AWK_EARGTF,
		ERR_ARGTM = QSE_AWK_EARGTM,
		ERR_FUNNF = QSE_AWK_EFUNNF,
		ERR_NOTIDX = QSE_AWK_ENOTIDX,
		ERR_NOTDEL = QSE_AWK_ENOTDEL,
		ERR_NOTMAP = QSE_AWK_ENOTMAP,
		ERR_NOTMAPIN = QSE_AWK_ENOTMAPIN,
		ERR_NOTMAPNILIN = QSE_AWK_ENOTMAPNILIN,
		ERR_NOTREF = QSE_AWK_ENOTREF,
		ERR_NOTASS = QSE_AWK_ENOTASS,
		ERR_IDXVALASSMAP = QSE_AWK_EIDXVALASSMAP,
		ERR_POSVALASSMAP = QSE_AWK_EPOSVALASSMAP,
		ERR_MAPTOSCALAR = QSE_AWK_EMAPTOSCALAR,
		ERR_SCALARTOMAP = QSE_AWK_ESCALARTOMAP,
		ERR_MAPNA = QSE_AWK_EMAPNA,
		ERR_VALTYPE = QSE_AWK_EVALTYPE,
		ERR_RDELETE = QSE_AWK_ERDELETE,
		ERR_RRESET = QSE_AWK_ERRESET,
		ERR_RNEXTBEG = QSE_AWK_ERNEXTBEG,
		ERR_RNEXTEND = QSE_AWK_ERNEXTEND,
		ERR_RNEXTFBEG = QSE_AWK_ERNEXTFBEG,
		ERR_RNEXTFEND = QSE_AWK_ERNEXTFEND,
		ERR_FNCIMPL = QSE_AWK_EFNCIMPL,
		ERR_IOUSER = QSE_AWK_EIOUSER,
		ERR_IOIMPL = QSE_AWK_EIOIMPL,
		ERR_IONMNF = QSE_AWK_EIONMNF,
		ERR_IONMEM = QSE_AWK_EIONMEM,
		ERR_IONMNL = QSE_AWK_EIONMNL,
		ERR_FMTARG = QSE_AWK_EFMTARG,
		ERR_FMTCNV = QSE_AWK_EFMTCNV,
		ERR_CONVFMTCHR = QSE_AWK_ECONVFMTCHR,
		ERR_OFMTCHR = QSE_AWK_EOFMTCHR,
		ERR_REXRECUR = QSE_AWK_EREXRECUR,
		ERR_REXRPAREN = QSE_AWK_EREXRPAREN,
		ERR_REXRBRACKET = QSE_AWK_EREXRBRACKET,
		ERR_REXRBRACE = QSE_AWK_EREXRBRACE,
		ERR_REXUNBALPAREN = QSE_AWK_EREXUNBALPAREN,
		ERR_REXINVALBRACE = QSE_AWK_EREXINVALBRACE,
		ERR_REXCOLON = QSE_AWK_EREXCOLON,
		ERR_REXCRANGE = QSE_AWK_EREXCRANGE,
		ERR_REXCCLASS = QSE_AWK_EREXCCLASS,
		ERR_REXBRANGE = QSE_AWK_EREXBRANGE,
		ERR_REXEND = QSE_AWK_EREXEND,
		ERR_REXGARBAGE = QSE_AWK_EREXGARBAGE,
	};

protected:
	///
	/// The getErrorString() function returns a formatting string
	/// for an error code @a num. You can override this function
	/// to customize an error message. You must include the same numbers
	/// of ${X}'s as the orginal formatting string. Their order may be
	/// different. The example below changes the formatting string for
	/// ERR_NOENT.
	/// @code
	/// const MyAwk::char_t* MyAwk::getErrorString (ErrorNumber num) const 
	/// {
	///    if (num == ERR_NOENT) return QSE_T("cannot find '${0}'");
	///    return Awk::getErrorString (num);
	/// }
	/// @endcode
	///
	virtual const char_t* getErrorString (
		ErrorNumber num
	) const;

public:
	///
	/// The getErrorNumber() function returns the number of the last
	/// error occurred.
	///
	ErrorNumber getErrorNumber () const;

	///
	/// The getErrorLocation() function returns the location of the 
	/// last error occurred.
	///
	loc_t getErrorLocation () const;

	///
	/// The Awk::getErrorMessage() function returns a message describing
	/// the last error occurred.
	///
	const char_t* getErrorMessage () const;

	///
	/// The setError() function sets error information.
	///
	void setError (
		ErrorNumber   code, ///< error code
		const cstr_t* args  = QSE_NULL, ///< message formatting 
		                                ///  argument array
		const loc_t*  loc   = QSE_NULL  ///< error location
	);

	///
	/// The setErrorWithMessage() functions sets error information
	/// with a customized error message.
	///
	void setErrorWithMessage (
		ErrorNumber   code, ///< error code
		const char_t* msg,  ///< error message
		const loc_t*  loc   ///< error location
	);

	///
	/// The clearError() function clears error information 
	///
	void clearError ();

protected:
	void retrieveError ();
	void retrieveError (Run* run);
	/*@}*/

protected:
	class NoSource;

public:
	/// 
	/// The Source class is an abstract class to encapsulate
	/// source script I/O. The Awk::parse function requires a concrete
	/// object instantiated from its child class.
	///
	class Source
	{
	public:
		///
		/// The Mode type defines opening mode.
		///
		enum Mode
		{	
			READ,   ///< open for read
			WRITE   ///< open for write
		};

		///
		/// The Data class encapsulates information passed in and out
		/// for source script I/O. 
		///
		class Data: protected sio_arg_t
		{
		public:
			friend class Awk;

		protected:
			Data (Awk* awk, Mode mode, sio_arg_t* arg): 
				awk (awk), mode (mode), arg (arg)
			{
			}

		public:
			Mode getMode() const
			{
				return mode;
			}

			const char_t* getName() const
			{
				return arg->name;
			}

			const void* getHandle () const
			{
				return arg->handle;
			}

			void  setHandle (void* handle)
			{
				arg->handle = handle;
			}

			operator Awk* () const
			{
				return awk;
			}

			operator awk_t* () const
			{
				return awk->awk;
			}

		protected:
			Awk* awk;
			Mode  mode;
			sio_arg_t* arg;
		};

		Source () {}
		virtual ~Source () {}

		virtual int open (Data& io) = 0;
		virtual int close (Data& io) = 0;
		virtual ssize_t read (Data& io, char_t* buf, size_t len) = 0;
		virtual ssize_t write (Data& io, char_t* buf, size_t len) = 0;

		///
		/// The NONE object indicates no source.
		///
		static NoSource NONE;

	private:
		Source (const Source&);
		Source& operator= (const Source&);
	};

protected:
	class NoSource: public Source
	{
	public:
		int open (Data& io) { return -1; }
		int close (Data& io) { return 0; }
		ssize_t read (Data& io, char_t* buf, size_t len) { return 0; }
		ssize_t write (Data& io, char_t* buf, size_t len) { return 0; }
	};

public:
	/**
	 * The RIOBase class is a base class to represent runtime I/O context.
	 * The Console, File, Pipe classes inherit this class to implement
	 * an actual I/O context.
	 */
	class RIOBase
	{
	protected:
		RIOBase (Run* run, rio_arg_t* riod);

	public:
		const char_t* getName() const;
		const void* getHandle () const;
		void  setHandle (void* handle);

		operator Awk* () const;
		operator awk_t* () const;
		operator rio_arg_t* () const;
		operator Run* () const;
		operator rtx_t* () const;

	protected:
		Run* run;
		rio_arg_t* riod;

	private:
		RIOBase (const RIOBase&);
		RIOBase& operator= (const RIOBase&);
	};

	/**
	 * The Pipe class encapsulates the pipe operations indicated by
	 * the | and || operators.
	 */
	class Pipe: public RIOBase
	{
	public:
		friend class Awk;

		/// The Mode type defines the opening mode.
		enum Mode
		{
			/// open for read-only access
			READ = QSE_AWK_RIO_PIPE_READ,
			/// open for write-only access
			WRITE = QSE_AWK_RIO_PIPE_WRITE,
			/// open for read and write
			RW = QSE_AWK_RIO_PIPE_RW
		};

		/// The CloseMode type defines the closing mode for a pipe
		/// opened in the #RW mode.
		enum CloseMode
		{
			/// close both read and write ends
			CLOSE_FULL = QSE_AWK_RIO_CLOSE_FULL, 
			/// close the read end only
			CLOSE_READ = QSE_AWK_RIO_CLOSE_READ,
			/// close the write end only
			CLOSE_WRITE = QSE_AWK_RIO_CLOSE_WRITE
		};

	protected:
		Pipe (Run* run, rio_arg_t* riod);

	public:
		/// The getMode() function returns the opening mode requested.
		/// You can inspect the opening mode, typically in the 
		/// openPipe() function, to create a pipe with proper 
		/// access mode. It is harmless to call this function from
		/// other pipe handling functions.
		Mode getMode () const;

		/// The getCloseMode() function returns the closing mode 
		/// requested. The returned value is valid if getMode() 
		/// returns #RW.
		CloseMode getCloseMode () const;
	};

	/**
	 * File
	 */
	class File: public RIOBase
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = QSE_AWK_RIO_FILE_READ,
			WRITE = QSE_AWK_RIO_FILE_WRITE,
			APPEND = QSE_AWK_RIO_FILE_APPEND
		};

	protected:
		File (Run* run, rio_arg_t* riod);

	public:
		Mode getMode () const;
	};

	/**
	 * Console
	 */
	class Console: public RIOBase
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = QSE_AWK_RIO_CONSOLE_READ,
			WRITE = QSE_AWK_RIO_CONSOLE_WRITE
		};

	protected:
		Console (Run* run, rio_arg_t* riod);
		~Console ();

	public:
		Mode getMode () const;
		int setFileName (const char_t* name);
		int setFNR (long_t fnr);

	protected:
		char_t* filename;
	};

	/**
	 * Represents a value.
	 */
	class Value
	{
	public:
		friend class Awk;

		// initialization
		void* operator new (size_t n, Run* run) throw ();
		void* operator new[] (size_t n, Run* run) throw ();

	#if !defined(__BORLANDC__) 
		// deletion when initialization fails
		void operator delete (void* p, Run* run);
		void operator delete[] (void* p, Run* run);
	#endif

		// normal deletion
		void operator delete (void* p);
		void operator delete[] (void* p);

		/**
		 * Represents an index of an arrayed value
		 */
		class Index
		{
		public:
			friend class Value;

			Index (): ptr (EMPTY_STRING), len (0) {}
			Index (const char_t* ptr, size_t len):
				ptr (ptr), len (len) {}

			const char_t* ptr;
			size_t        len;
		};

		/**
		 * Represents a numeric index of an arrayed value
		 */
		class IntIndex: public Index
		{
		public:
			IntIndex (long_t num);

		protected:
			// 2^32: 4294967296
			// 2^64: 18446744073709551616
			// 2^128: 340282366920938463463374607431768211456 
			// -(2^32/2): -2147483648
			// -(2^64/2): -9223372036854775808
			// -(2^128/2): -170141183460469231731687303715884105728
		#if QSE_SIZEOF_LONG_T > 16
		#	error SIZEOF(qse_long_t) TOO LARGE. 
		#	error INCREASE THE BUFFER SIZE TO SUPPORT IT.
		#elif QSE_SIZEOF_LONG_T == 16
			char_t buf[41];
		#elif QSE_SIZEOF_LONG_T == 8
			char_t buf[21];
		#else
			char_t buf[12];
		#endif
		};

		/**
		 * helper class to iterate over array elements
		 */
		class IndexIterator
		{
		public:
			friend class Value;

			static IndexIterator END;

			IndexIterator (): pair (QSE_NULL), buckno (0) {}
			IndexIterator (pair_t* pair, size_t buckno): 
				pair (pair), buckno (buckno) {}

			bool operator==  (const IndexIterator& ii) const
			{
				return pair == ii.pair && buckno == ii.buckno;
			}

			bool operator!=  (const IndexIterator& ii) const
			{
				return !operator== (ii);
			}

		protected:
			pair_t* pair;
			size_t  buckno;
		};

		Value ();
		Value (Run& run);
		Value (Run* run);

		Value (const Value& v);
		~Value ();

		Value& operator= (const Value& v);

		void clear ();

		operator val_t* () const { return val; }
		operator long_t () const;
		operator real_t () const;
		operator const char_t* () const;

		val_t* toVal () const
		{
			return operator val_t* ();
		}

		long_t toInt () const
		{
			return operator long_t ();
		}

		real_t toReal () const
		{
			return operator real_t ();
		}

		const char_t* toStr (size_t* len) const
		{
			const char_t* p;
			size_t l;

			if (getStr (&p, &l) == -1) 
			{
				p = EMPTY_STRING;
				l = 0;
			}
			
			if (len != QSE_NULL) *len = l;
			return p;
		}

		int getInt (long_t* v) const;
		int getReal (real_t* v) const;
		int getStr (const char_t** str, size_t* len) const;

		int setVal (val_t* v);
		int setVal (Run* r, val_t* v);

		int setInt (long_t v);
		int setInt (Run* r, long_t v);
		int setReal (real_t v);
		int setReal (Run* r, real_t v);
		int setStr (const char_t* str, size_t len);
		int setStr (Run* r, const char_t* str, size_t len);
		int setStr (const char_t* str);
		int setStr (Run* r, const char_t* str);

		int setIndexedVal (
			const Index& idx,
			val_t*       v
		);

		int setIndexedVal (
			Run*         r, 
			const Index& idx, 
			val_t*       v
		);

		int setIndexedInt (
			const Index& idx,
			long_t       v
		);

		int setIndexedInt (
			Run* r,
			const Index& idx,
			long_t v);

		int setIndexedReal (
			const Index&  idx,
			real_t        v
		);

		int setIndexedReal (
			Run*          r,
			const Index&  idx,
			real_t        v
		);

		int setIndexedStr (
			const Index&  idx,
			const char_t* str,
			size_t        len
		);

		int setIndexedStr (
			Run*          r,
			const Index&  idx,
			const char_t* str,
			size_t        len
		);

		int setIndexedStr (
			const Index&  idx,
			const char_t* str
		);

		int setIndexedStr (
			Run*          r,
			const Index&  idx,
			const char_t* str
		);

		/** determines if a value is arrayed */
		bool isIndexed () const;

		int getIndexed (
			const Index&  idx,
			Value*        val
		) const;

		IndexIterator getFirstIndex (
			Index* idx
		) const;

		IndexIterator getNextIndex (
			Index* idx,
			const IndexIterator& iter
		) const;

	protected:
		Run* run;
		val_t* val;

		mutable struct
		{	
			qse_xstr_t str;
		} cached;

		static const char_t* EMPTY_STRING;
	};

public:
	/**
	 * Defines an identifier of predefined global variables.
	 * Awk::setGlobal and Awk::getGlobal can take one of these enumerators.
	 */
	enum Global
	{
		GBL_ARGC = QSE_AWK_GBL_ARGC,           /**< ARGC */
		GBL_ARGV = QSE_AWK_GBL_ARGV,           /**< ARGV */
		GBL_CONVFMT = QSE_AWK_GBL_CONVFMT,     /**< CONVFMT */
		GBL_FILENAME = QSE_AWK_GBL_FILENAME,   /**< FILENAME */
		GBL_FNR = QSE_AWK_GBL_FNR,             /**< FNR */
		GBL_FS = QSE_AWK_GBL_FS,               /**< FS */
		GBL_IGNORECASE = QSE_AWK_GBL_IGNORECASE, /**< IGNORECASE */
		GBL_NF = QSE_AWK_GBL_NF,               /**< NF */
		GBL_NR = QSE_AWK_GBL_NR,               /**< NR */
		GBL_OFILENAME = QSE_AWK_GBL_OFILENAME, /**< OFILENAME */
		GBL_OFMT = QSE_AWK_GBL_OFMT,           /**< OFMT */
		GBL_OFS = QSE_AWK_GBL_OFS,             /**< OFS */
		GBL_ORS = QSE_AWK_GBL_ORS,             /**< ORS */
		GBL_RLENGTH = QSE_AWK_GBL_RLENGTH,     /**< RLENGTH */
		GBL_RS = QSE_AWK_GBL_RS,               /**< RS */
		GBL_RSTART = QSE_AWK_GBL_RSTART,       /**< RSTART */
		GBL_SUBSEP = QSE_AWK_GBL_SUBSEP        /**< SUBSEP */
	};

	/** Represents the execution context */
	class Run
	{
	protected:
		friend class Awk;
		friend class Value; 
		friend class RIOBase;
		friend class Console;

		Run (Awk* awk);
		Run (Awk* awk, rtx_t* run);
		~Run ();

	public:
		operator Awk* () const;
		operator rtx_t* () const;

		void stop () const;
		bool shouldStop () const;

		ErrorNumber getErrorNumber () const;
		loc_t getErrorLocation () const;
		const char_t* getErrorMessage () const;

		void setError (
			ErrorNumber   code, 
			const cstr_t* args = QSE_NULL,
			const loc_t*  loc  = QSE_NULL
		);

		void setErrorWithMessage (
			ErrorNumber   code, 
			const char_t* msg,
			const loc_t*  loc
		);

		/** 
		 * Sets the value of a global variable identified by @a id
		 * to @a v.
		 * @return 0 on success, -1 on failure
		 */
		int setGlobal (int id, long_t v);

		/** 
		 * Sets the value of a global variable identified by @a id
		 * to @a v.
		 * @return 0 on success, -1 on failure
		 */
		int setGlobal (int id, real_t v); 

		/** 
		 * Sets the value of a global variable identified by @a id
		 * to a string as long as @a len characters pointed to by 
		 * @a ptr.
		 * @return 0 on success, -1 on failure
		 */
		int setGlobal (int id, const char_t* ptr, size_t len);

		/** 
		 * Sets a global variable identified by @a id to a value @a v.
		 * @return 0 on success, -1 on failure
		 */
		int setGlobal (int id, const Value& v);

		/**
		 * Gets the value of a global variable identified by @a id 
		 * and store it in @a v.
		 * @return 0 on success, -1 on failure
		 */
		int getGlobal (int id, Value& v) const;

	protected:
		Awk*   awk;
		rtx_t* rtx;
	};

	/** Returns the primitive handle */
	operator awk_t* () const;

	/**
	 * @name Basic Functions
	 */
	/*@{*/
	/** Constructor */
	Awk ();

	/**
	 * The Awk::open() function initializes an interpreter. 
	 * You must call this function before doing anything meaningful.
	 * @return 0 on success, -1 on failure
	 */
	int open ();

	/** Closes the interpreter. */
	void close ();

	/**
	 * The Awk::parse() function parses the source code read from the input 
	 * stream @a in and writes the parse tree to the output stream @a out.
	 * To disable deparsing, you may set @a out to Awk::Source::NONE. 
	 * However, it is not allowed to specify Awk::Source::NONE for @a in.
	 *
	 * @return Run object on success, #QSE_NULL on failure
	 */
	Awk::Run* parse (
		Source& in,  ///< script to parse 
		Source& out  ///< deparsing target 
	);

	/**
	 * Executes the BEGIN block, pattern-action blocks, and the END block.
	 * @return 0 on succes, -1 on failure
	 */
	int loop (
		Value* ret  ///< return value holder
	);

	/**
	 * Calls a function
	 */
	int call (
		const char_t* name,  ///< function name
		Value*        ret,   ///< return value holder
		const Value*  args,  ///< argument array
		size_t        nargs  ///< number of arguments
	);

	/**
	 * Makes request to abort execution
	 */
	void stop ();
	/*@}*/

	/**
	 * @name Configuration
	 */
	/*@{*/

	/** Defines options */
	enum Option
	{
		OPT_IMPLICIT = QSE_AWK_IMPLICIT,
		OPT_EXPLICIT = QSE_AWK_EXPLICIT,
		OPT_EXTRAOPS = QSE_AWK_EXTRAOPS,
		OPT_RIO = QSE_AWK_RIO,
		OPT_RWPIPE = QSE_AWK_RWPIPE,

		/** Can terminate a statement with a new line */
		OPT_NEWLINE = QSE_AWK_NEWLINE,

		OPT_STRIPRECSPC = QSE_AWK_STRIPRECSPC,
		OPT_STRIPSTRSPC = QSE_AWK_STRIPSTRSPC,

		/** Support the nextofile statement */
		OPT_NEXTOFILE = QSE_AWK_NEXTOFILE,
		/** Enables the keyword 'reset' */
		OPT_RESET = QSE_AWK_RESET,
		/** Use CR+LF instead of LF for line breaking. */
		OPT_CRLF = QSE_AWK_CRLF,
		/** Allows the assignment of a map value to a variable */
		OPT_MAPTOVAR = QSE_AWK_MAPTOVAR,
		/** Allows BEGIN, END, pattern-action blocks */
		OPT_PABLOCK = QSE_AWK_PABLOCK,
		/** Allows {n,m} in a regular expression */
		OPT_REXBOUND = QSE_AWK_REXBOUND,
	        /**
		 * Performs numeric comparison when a string convertable
		 * to a number is compared with a number or vice versa.
		 *
		 * For an expression (9 > "10.9"),
		 * - 9 is greater if #QSE_AWK_NCMPONSTR is off;
		 * - "10.9" is greater if #QSE_AWK_NCMPONSTR is on
		 */
		OPT_NCMPONSTR = QSE_AWK_NCMPONSTR,

		/**
		 * Enables the strict naming rule.
		 * - a parameter name can not be the same as the owning
		 *   function name.
		 * - a local variable name can not be the same as the 
		 *   owning function name.
		 */

		OPT_STRICTNAMING = QSE_AWK_STRICTNAMING,

		/** Enables 'include' */
		OPT_INCLUDE = QSE_AWK_INCLUDE
	};
	/** Gets the option */
	int getOption () const;

	/** Sets the option */
	void setOption (
		int opt
	);

	/** Defines the depth ID */
	enum Depth
	{
		DEPTH_BLOCK_PARSE = QSE_AWK_DEPTH_BLOCK_PARSE,
		DEPTH_BLOCK_RUN   = QSE_AWK_DEPTH_BLOCK_RUN,
		DEPTH_EXPR_PARSE  = QSE_AWK_DEPTH_EXPR_PARSE,
		DEPTH_EXPR_RUN    = QSE_AWK_DEPTH_EXPR_RUN,
		DEPTH_REX_BUILD   = QSE_AWK_DEPTH_REX_BUILD,
		DEPTH_REX_MATCH   = QSE_AWK_DEPTH_REX_MATCH
	};

	/** Sets the maximum depth */
	void setMaxDepth (int ids, size_t depth);
	/** Gets the maximum depth */
	size_t getMaxDepth (depth_t id) const;

	/**
	 * Adds an ARGV string as long as @a len characters pointed to 
	 * by @a arg. loop() and call() make a string added available 
	 * to a script through ARGV. 
	 * @return 0 on success, -1 on failure
	 */
	int addArgument (
		const char_t* arg, 
		size_t        len
	);

	/**
	 * Adds a null-terminated string @a arg. loop() and call() 
	 * make a string added available to a script through ARGV. 
	 * @return 0 on success, -1 on failure
	 */
	int addArgument (const char_t* arg);

	/**
	 * Deletes all ARGV strings.
	 */
	void clearArguments ();

	/**
	 * Registers an intrinsic global variable. 
	 * @return integer >= 0 on success, -1 on failure.
	 */
	int addGlobal (
		const char_t* name ///< variable name
	);

	/**
	 * Unregisters an intrinsic global variable. 
	 * @return 0 on success, -1 on failure.
	 */
	int deleteGlobal (
		const char_t* name ///< variable name
	);

	/**
	 * Sets the value of a global variable identified by @a id.
	 * The @a id is either a value returned by Awk::addGlobal or one of 
	 * Awk::Global enumerators. It is not allowed to call this function
	 * prior to Awk::parse.
	 * @return 0 on success, -1 on failure
	 */
	int setGlobal (
		int          id,  ///< numeric identifier
		const Value& v    ///< value
	);

	/**
	 * Gets the value of a global riable identified by @a id.
	 * The @a id is either a value returned by Awk::addGlobal or one of 
	 * Awk::::Global enumerators. It is not allowed to call this function
	 * prior to Awk::parse.
	 * @return 0 on success, -1 on failure
	 */
	int getGlobal (
		int    id, ///< numeric identifier 
		Value& v   ///< value store 
	);

	/**
	 * Defines a intrinsic function handler.
	 */
	typedef int (Awk::*FunctionHandler) (
		Run&          run,
		Value&        ret,
		const Value*  args,
		size_t        nargs, 
		const cstr_t* name
	);

	/**
	 * Adds a new user-defined intrinsic function.
	 */
	int addFunction (
		const char_t* name, size_t minArgs, size_t maxArgs, 
		FunctionHandler handler);

	/**
	 * Deletes a user-defined intrinsic function
	 */
	int deleteFunction (const char_t* name);
	/*@}*/

	/**
	 * @name Word Substitution
	 */
	/*@{*/
	int getWord (
		const cstr_t* ow,
		cstr_t*       nw
	);

	int setWord (
		const cstr_t* ow,
		const cstr_t* nw
	);

	int unsetWord (
		const cstr_t* ow
	);

	void unsetAllWords ();
	/*@}*/

protected:
	/** 
	 * @name Pipe I/O handlers
	 * Pipe operations are achieved through the following functions.
	 */
	/*@{*/

	/// The openPipe() function is a pure virtual function that must be
	/// overridden by a child class to open a pipe. It must return 1
	/// on success, 0 on end of a pipe, and -1 on failure.
	virtual int     openPipe  (Pipe& io) = 0;

	/// The closePipe() function is a pure virtual function that must be
	/// overridden by a child class to close a pipe. It must return 0
	/// on success and -1 on failure.
	virtual int     closePipe (Pipe& io) = 0;

	virtual ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writePipe (Pipe& io, const char_t* buf, size_t len) = 0;
	virtual int     flushPipe (Pipe& io) = 0;
	/*@}*/

	/** 
	 * @name File I/O handlers
	 * File operations are achieved through the following functions.
	 */
	/*@{*/
	virtual int     openFile  (File& io) = 0;
	virtual int     closeFile (File& io) = 0;
	virtual ssize_t readFile  (File& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeFile (File& io, const char_t* buf, size_t len) = 0;
	virtual int     flushFile (File& io) = 0;
	/*@}*/

	/** 
	 * @name Console I/O handlers
	 * Console operations are achieved through the following functions.
	 */
	virtual int     openConsole  (Console& io) = 0;
	virtual int     closeConsole (Console& io) = 0;
	virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeConsole (Console& io, const char_t* buf, size_t len) = 0;
	virtual int     flushConsole (Console& io) = 0;
	virtual int     nextConsole  (Console& io) = 0;
	/*@}*/

	// primitive handlers 
	virtual real_t pow (real_t x, real_t y) = 0;
	virtual int    vsprintf (char_t* buf, size_t size,
	                         const char_t* fmt, va_list arg) = 0;

	// static glue members for various handlers
	static ssize_t readSource (
		awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
		char_t* data, size_t count);
	static ssize_t writeSource (
		awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
		char_t* data, size_t count);

	static ssize_t pipeHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t fileHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t consoleHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);

	static int functionHandler (rtx_t* rtx, const cstr_t* name);

	static real_t pow     (awk_t* data, real_t x, real_t y);
	static int    sprintf (awk_t* data, char_t* buf, size_t size,
	                       const char_t* fmt, ...);

protected:
	awk_t* awk;

	errstr_t dflerrstr;
	errinf_t errinf;

	map_t* functionMap;

	Source* sourceReader;
	Source* sourceWriter;

	struct xstrs_t
	{
		xstrs_t (): ptr (QSE_NULL), len (0), capa (0) {}

		int add (awk_t* awk, const char_t* arg, size_t len);
		void clear (awk_t* awk);

		qse_xstr_t* ptr;
		size_t      len;
		size_t      capa;
	};

	xstrs_t runarg;

private:
	Run runctx;

	int init_runctx ();
	void fini_runctx ();

	int dispatch_function (Run* run, const cstr_t* name);

	static const char_t* xerrstr (awk_t* a, errnum_t num);

private:
	Awk (const Awk&);
	Awk& operator= (const Awk&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
