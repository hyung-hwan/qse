/*
 * $Id: Awk.hpp 220 2009-07-01 13:14:39Z hyunghwan.chung $
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

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/** 
 * Represents the AWK interpreter engine
 */
class Awk: public Mmgr
{
public:
	typedef qse_map_t   map_t;
	typedef qse_map_pair_t pair_t;

	/** Represents a underlying interpreter */
	typedef qse_awk_t awk_t;

	typedef qse_awk_errnum_t errnum_t;
	typedef qse_awk_errstr_t errstr_t;

	/** Represents an internal awk value */
	typedef qse_awk_val_t val_t;

	/** Represents a runtime context */
	typedef qse_awk_rtx_t rtx_t;

	/** Represents an runtime I/O data */
	typedef qse_awk_rio_arg_t rio_arg_t;

	typedef qse_awk_rio_cmd_t rio_cmd_t;

	/**
	 * Represents the source code I/O context for Awk::parse.
	 * An instance of Awk::Source is passed to Awk::openSource, 
	 * Awk::readSource, Awk::writeSource, Awk::closeSource
	 * when Awk::parse calls them to read the source code and write the 
	 * internal parse tree. It indicates the mode of the context and
	 * provides space for data that may be needed for the I/O operation.
	 */
	class Source
	{
	public:
		friend class Awk;

		/** Mode of source code I/O. */
		enum Mode
		{	
			READ,   /**< source code read. */
			WRITE   /**< source code write. */
		};

	protected:
		Source (Mode mode);

	public:
		/**
		 * Returns the mode of the source code I/O. 
		 * You may call this method in Awk::openSource and 
		 * Awk::closeSource to determine the mode as shown in 
		 * the example below. This method always returns Source::READ 
		 * and Source::WRITE respectively when called from 
		 * Awk::readSource and Awk::writeSource.
		 *
		 * <pre>
		 * int openSource (Source& io)
		 * {
		 * 	if (io.getMode() == Source::READ)
		 * 	{
		 * 		// open for reading 
		 * 		return 1;
		 * 	}
		 * 	else (io.getMode() == Source::WRITE)
		 * 	{
		 *		// open for writing
		 *		return 1;
		 * 	}
		 * 	return -1;
		 * }
		 *
		 * int closeSource (Source& io)
		 * {
		 * 	if (io.getMode() == Source::READ)
		 * 	{
		 * 		// close for reading 
		 * 		return 0;
		 * 	}
		 * 	else (io.getMode() == Source::WRITE)
		 * 	{
		 *		// close for writing
		 *		return 0;
		 * 	}
		 * 	return -1;
		 * }
		 * </pre>
		 *
		 * @return Awk::Source::READ or Awk::Source::WRITE
		 */
		Mode getMode() const;

		/**
		 * Returns the value set with Source::setHandle. 
		 * QSE_NULL is returned if it has not been set with 
		 * Source::setHandle. You usually call this method
		 * from Awk::readSource, Awk::writeSource, and 
		 * Awk::closeSource to get the value set in Awk::openSource
		 * as shown in the example below.
		 *
		 * <pre>
		 * int closeSource (Source& io)
		 * {
		 * 	if (io.getMode() == Source::READ)
		 * 	{
		 * 		fclose ((FILE*)io.getHandle());
		 * 		return 0;
		 * 	}
		 * 	else (io.getMode() == Source::WRITE)
		 * 	{
		 * 		fclose ((FILE*)io.getHandle());
		 * 		return 0;
		 * 	}
		 * 	return -1;
		 * }
		 * </pre>
		 *
		 * @return an arbitrary value of type void* set with 
		 *         Source::setHandle or QSE_NULL
		 */
		const void* getHandle () const;

		/**
		 * Sets the handle value. Source::getHandle can retrieve
		 * the value set with Source::setHandle. You usually call 
		 * this from Awk::openSource as shown in the example below.
		 *
		 * <pre>
		 * int openSource (Source& io)
		 * {
		 * 	if (io.getMode() == Source::READ)
		 * 	{
		 * 		FILE* fp = fopen ("t.awk", "r");
		 * 		if (fp == NULL) return -1;
		 * 		io.setHandle (fp);
		 * 		return 1;
		 * 	}
		 * 	else (io.getMode() == Source::WRITE)
		 * 	{
		 * 		FILE* fp = fopen ("t.out", "w");
		 * 		if (fp == NULL) return -1;
		 * 		io.setHandle (fp);
		 *		return 1;
		 * 	}
		 * 	return -1;
		 * }
		 * </pre>
		 *
		 * @param handle an arbitrary value of the type void*
		 */
		void  setHandle (void* handle);

	protected:
		Mode  mode;
		void* handle;
	};

	/**
	 * RIO class 
	 */
	class RIOBase
	{
	protected:
		RIOBase (rtx_t* rtx, rio_arg_t* riod);

	public:
		const char_t* getName() const;
		const void* getHandle () const;
		void  setHandle (void* handle);

		operator Awk* () const;
		operator awk_t* () const;
		operator rio_arg_t* () const;
		operator rtx_t* () const;

	protected:
		rtx_t* rtx;
		rio_arg_t* riod;
	};

	/**
	 * Pipe
	 */
	class Pipe: public RIOBase
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = QSE_AWK_RIO_PIPE_READ,
			WRITE = QSE_AWK_RIO_PIPE_WRITE,
			RW = QSE_AWK_RIO_PIPE_RW
		};

	protected:
		Pipe (rtx_t* rtx, rio_arg_t* riod);

	public:
		Mode getMode () const;
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
		File (rtx_t* rtx, rio_arg_t* riod);

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
		Console (rtx_t* rtx, rio_arg_t* riod);
		~Console ();

	public:
		Mode getMode () const;
		int setFileName (const char_t* name);
		int setFNR (long_t fnr);

	protected:
		char_t* filename;
	};

	class Run;
	class Argument;
	class Return;

	friend class Run;
	friend class Argument;
	friend class Return;

	/**
	 * Represents an argument to an intrinsic function
	 */
	class Argument
	{
	public:
		friend class Awk;
		friend class Run;

		Argument (Run& run);
		Argument (Run* run);
		~Argument ();

	protected:
		Argument ();
		void clear ();

	public:
		// initialization
		void* operator new (size_t n, awk_t* awk) throw ();
		void* operator new[] (size_t n, awk_t* awk) throw ();

	#if !defined(__BORLANDC__) 
		// deletion when initialization fails
		void operator delete (void* p, awk_t* awk);
		void operator delete[] (void* p, awk_t* awk);
	#endif

		// normal deletion
		void operator delete (void* p);
		void operator delete[] (void* p);

	private:
		Argument (const Argument&);
		Argument& operator= (const Argument&);

	protected:
		int init (val_t* v);
		int init (const char_t* str, size_t len);

	public:
		long_t toInt () const;
		real_t toReal () const;
		const char_t* toStr (size_t* len) const;

		bool isIndexed () const;

		int getIndexed (const char_t* idxptr, Argument& val) const;
		int getIndexed (const char_t* idxptr, size_t idxlen, Argument& val) const;
		int getIndexed (long_t idx, Argument& val) const;

		int getFirstIndex (Argument& val) const;
		int getNextIndex (Argument& val) const;

	protected:
		Run* run;
		val_t* val;

		qse_long_t inum;
		qse_real_t rnum;
		mutable qse_str_t str;
	};

	/**
	 * Represents a return value of an intrinsic function
	 */
	class Return
	{
	public:
		friend class Awk;
		friend class Run;

		Return (Run& run);
		Return (Run* run);
		~Return ();

	private:
		Return (const Return&);
		Return& operator= (const Return&);

	protected:
		val_t* toVal () const;
		operator val_t* () const;

	public:
		int set (long_t v);
		int set (real_t v); 
		int set (const char_t* ptr, size_t len);

		bool isIndexed () const;

		int setIndexed (const char_t* idx, size_t iln, long_t v);
		int setIndexed (const char_t* idx, size_t iln, real_t v);
		int setIndexed (const char_t* idx, size_t iln, const char_t* str, size_t sln);
		int setIndexed (long_t idx, long_t v);
		int setIndexed (long_t idx, real_t v);
		int setIndexed (long_t idx, const char_t* str, size_t sln);

		void clear ();

	protected:
		Run* run;
		val_t* val;
	};

	// generated by generrcode.awk
	/** Defines the error code */
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
		ERR_SINOP = QSE_AWK_ESINOP,
		ERR_SINCL = QSE_AWK_ESINCL,
		ERR_SINRD = QSE_AWK_ESINRD,
		ERR_SOUTOP = QSE_AWK_ESOUTOP,
		ERR_SOUTCL = QSE_AWK_ESOUTCL,
		ERR_SOUTWR = QSE_AWK_ESOUTWR,
		ERR_LXCHR = QSE_AWK_ELXCHR,
		ERR_LXDIG = QSE_AWK_ELXDIG,
		ERR_LXUNG = QSE_AWK_ELXUNG,
		ERR_ENDSRC = QSE_AWK_EENDSRC,
		ERR_ENDCMT = QSE_AWK_EENDCMT,
		ERR_ENDSTR = QSE_AWK_EENDSTR,
		ERR_ENDREX = QSE_AWK_EENDREX,
		ERR_LBRACE = QSE_AWK_ELBRACE,
		ERR_LPAREN = QSE_AWK_ELPAREN,
		ERR_RPAREN = QSE_AWK_ERPAREN,
		ERR_RBRACK = QSE_AWK_ERBRACK,
		ERR_COMMA = QSE_AWK_ECOMMA,
		ERR_SCOLON = QSE_AWK_ESCOLON,
		ERR_COLON = QSE_AWK_ECOLON,
		ERR_STMEND = QSE_AWK_ESTMEND,
		ERR_IN = QSE_AWK_EIN,
		ERR_NOTVAR = QSE_AWK_ENOTVAR,
		ERR_EXPRES = QSE_AWK_EEXPRES,
		ERR_FUNCTION = QSE_AWK_EFUNCTION,
		ERR_WHILE = QSE_AWK_EWHILE,
		ERR_ASSIGN = QSE_AWK_EASSIGN,
		ERR_IDENT = QSE_AWK_EIDENT,
		ERR_FUNNAME = QSE_AWK_EFUNNAME,
		ERR_BLKBEG = QSE_AWK_EBLKBEG,
		ERR_BLKEND = QSE_AWK_EBLKEND,
		ERR_DUPBEG = QSE_AWK_EDUPBEG,
		ERR_DUPEND = QSE_AWK_EDUPEND,
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
		ERR_MAPNOTALLOWED = QSE_AWK_EMAPNOTALLOWED,
		ERR_VALTYPE = QSE_AWK_EVALTYPE,
		ERR_RDELETE = QSE_AWK_ERDELETE,
		ERR_RNEXTBEG = QSE_AWK_ERNEXTBEG,
		ERR_RNEXTEND = QSE_AWK_ERNEXTEND,
		ERR_RNEXTFBEG = QSE_AWK_ERNEXTFBEG,
		ERR_RNEXTFEND = QSE_AWK_ERNEXTFEND,
		ERR_FNCUSER = QSE_AWK_EFNCUSER,
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
	// end of enum ErrorNumber


	// generated by genoptcode.awk
	/** Defines options */
	enum Option
	{
		OPT_IMPLICIT = QSE_AWK_IMPLICIT,
		OPT_EXPLICIT = QSE_AWK_EXPLICIT,
		OPT_BXOR = QSE_AWK_BXOR,
		OPT_SHIFT = QSE_AWK_SHIFT,
		OPT_IDIV = QSE_AWK_IDIV,
		OPT_RIO = QSE_AWK_RIO,
		OPT_RWPIPE = QSE_AWK_RWPIPE,

		/** Can terminate a statement with a new line */
		OPT_NEWLINE = QSE_AWK_NEWLINE,

		OPT_STRIPSPACES = QSE_AWK_STRIPSPACES,

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
		OPT_NCMPONSTR = QSE_AWK_NCMPONSTR 
	};
	// end of enum Option
	
	enum Global
	{
		GBL_ARGC = QSE_AWK_GBL_ARGC,
		GBL_ARGV = QSE_AWK_GBL_ARGV,
		GBL_CONVFMT = QSE_AWK_GBL_CONVFMT,
		GBL_FILENAME = QSE_AWK_GBL_FILENAME,
		GBL_FNR = QSE_AWK_GBL_FNR,
		GBL_FS = QSE_AWK_GBL_FS,
		GBL_IGNORECASE = QSE_AWK_GBL_IGNORECASE,
		GBL_NF = QSE_AWK_GBL_NF,
		GBL_NR = QSE_AWK_GBL_NR,
		GBL_OFILENAME = QSE_AWK_GBL_OFILENAME,
		GBL_OFMT = QSE_AWK_GBL_OFMT,
		GBL_OFS = QSE_AWK_GBL_OFS,
		GBL_ORS = QSE_AWK_GBL_ORS,
		GBL_RLENGTH = QSE_AWK_GBL_RLENGTH,
		GBL_RS = QSE_AWK_GBL_RS,
		GBL_RSTART = QSE_AWK_GBL_RSTART,
		GBL_SUBSEP = QSE_AWK_GBL_SUBSEP
	};

	/** Represents the execution context */
	class Run
	{
	protected:
		friend class Awk;
		friend class Argument;
		friend class Return;
		friend class RIO;

		Run (Awk* awk);
		Run (Awk* awk, rtx_t* run);
		~Run ();

	public:
		operator Awk* () const;
		operator rtx_t* () const;

		void stop () const;
		bool isStop () const;

		ErrorNumber getErrorNumber () const;
		size_t getErrorLine () const;
		const char_t* getErrorMessage () const;

		void setError (ErrorNumber code);
		void setError (ErrorNumber code, size_t line);
		void setError (ErrorNumber code, size_t line, const char_t* arg);
		void setError (ErrorNumber code, size_t line, const char_t* arg, size_t len);

		void setErrorWithMessage (
			ErrorNumber code, size_t line, const char_t* msg);

		/** 
		 * Sets the value of a global variable. The global variable
		 * is indicated by the first parameter. 
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param v
		 * 	The value to assign to the global variable.
		 *
		 * @return
		 * 	On success, 0 is returned.
		 * 	On failure, -1 is returned.
		 */
		int setGlobal (int id, long_t v);

		/** 
		 * Sets the value of a global variable. The global variable
		 * is indicated by the first parameter. 
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param v
		 * 	The value to assign to the global variable.
		 *
		 * @return
		 * 	On success, 0 is returned.
		 * 	On failure, -1 is returned.
		 */
		int setGlobal (int id, real_t v); 

		/** 
		 * Sets the value of a global variable. The global variable
		 * is indicated by the first parameter. 
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param ptr The pointer to a character array
		 * @param len The number of characters in the array
		 *
		 * @return
		 *  	On success, 0 is returned.
		 *  	On failure, -1 is returned.
		 */
		int setGlobal (int id, const char_t* ptr, size_t len);


		/** 
		 * Sets the value of a global variable. The global variable
		 * is indicated by the first parameter. 
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param global 
		 * 	The reference to the value holder 
		 *
		 * @return
		 *  	On success, 0 is returned.
		 *  	On failure, -1 is returned.
		 */
		int setGlobal (int id, const Return& global);

		/**
		 * Gets the value of a global variable.
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param global
		 * 	The reference to the value holder of a global variable
		 * 	indicated by id. The parameter is set if this method
		 * 	returns 0.
		 *
		 * @return 
		 * 	On success, 0 is returned.
		 * 	On failure, -1 is returned.
		 */
		int getGlobal (int id, Argument& global) const;

		/**
		 * Sets a value into the data field
		 */
		void setData (void* data);

		/**
		 * Gets the value stored in the data field
		 */
		void* getData () const;

		void* alloc (size_t size);
		void free (void* ptr);

	protected:
		Awk*   awk;
		rtx_t* run;
		void*  data;
	};

	/** Constructor */
	Awk () throw ();

	/** Returns the underlying handle */
	operator awk_t* () const;
	
	/** Returns the error code */
	ErrorNumber getErrorNumber () const;

	/** Returns the line of the source code where the error occurred */
	size_t getErrorLine () const ;

	/** Returns the error message */
	const char_t* getErrorMessage () const;

	mmgr_t* getMmgr() 
	{
		return qse_awk_getmmgr (awk);
	}

	const mmgr_t* getMmgr() const
	{
		return qse_awk_getmmgr (awk);
	}

protected:
	void setError (ErrorNumber code);
	void setError (ErrorNumber code, size_t line);
	void setError (ErrorNumber code, size_t line, const char_t* arg);
	void setError (ErrorNumber code, size_t line, const char_t* arg, size_t len);

	void setErrorWithMessage (
		ErrorNumber code, size_t line, const char_t* msg);

	void clearError ();
	void retrieveError ();
	void retrieveError (rtx_t* rtx);

public:
	/**
	 * Opens the interpreter. 
	 *
	 * An application should call this method before doing anything 
	 * meaningful to the instance of this class.
	 *
	 * @return
	 * 	On success, 0 is returned. On failure -1 is returned and
	 * 	extended error information is set. Call Awk::getErrorNumber
	 * 	to get it.
	 */
	virtual int open ();

	/** Closes the interpreter. */
	virtual void close ();

	/** Sets the option */
	virtual void setOption (int opt);

	/** Gets the option */
	virtual int  getOption () const;

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
	virtual void   setMaxDepth (int ids, size_t depth);
	/** Gets the maximum depth */
	virtual size_t getMaxDepth (int id) const;

	virtual const char_t* getErrorString (
		ErrorNumber num
	) const;

	virtual int getWord (
		const char_t* ow, qse_size_t owl,
		const char_t** nw, qse_size_t* nwl);
	virtual int setWord (
		const char_t* ow, const char_t* nw);
	virtual int setWord (
		const char_t* ow, qse_size_t owl,
		const char_t* nw, qse_size_t nwl);

	virtual int unsetWord (const char_t* ow);
	virtual int unsetWord (const char_t* ow, qse_size_t owl);
	virtual int unsetAllWords ();

	/**
	 * Parses the source code.
	 *
	 * Awk::parse parses the source code read from the input stream and 
	 * writes the parse tree to the output stream. A child class should
	 * override Awk::openSource, Awk::closeSource, Awk::readSource, 
	 * Awk::writeSource to implement the source code stream.
	 *
	 * @return 0 on success, -1 on failure
	 */
	virtual int parse ();

	/**
	 * Executes the BEGIN block, pattern-action blocks, and the END block.
	 * @return 0 on succes, -1 on failure
	 */
	virtual int loop ();

	/**
	 * Makes request to abort execution
	 */
	virtual void stop ();

	/**
	 * Adds a string for ARGV. loop() and call() makes a string added 
	 * available to a script through ARGV. Note this is not related to
	 * the Awk::Argument class.
	 */
	virtual int addArgument (
		const char_t* arg, 
		size_t        len
	);

	virtual int addArgument (const char_t* arg);

	/**
	 * Deletes all ARGV strings.
	 */
	virtual void clearArguments ();

	/**
	 * Adds a intrinsic global variable. 
	 */
	virtual int addGlobal (const char_t* name);

	/**
	 * Deletes a intrinsic global variable. 
	 */
	virtual int deleteGlobal (const char_t* name);

	/**
	 * Represents a user-defined intrinsic function.
	 */
	typedef int (Awk::*FunctionHandler) (
		Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);

	/**
	 * Adds a new user-defined intrinsic function.
	 */
	virtual int addFunction (
		const char_t* name, size_t minArgs, size_t maxArgs, 
		FunctionHandler handler);

	/**
	 * Deletes a user-defined intrinsic function
	 */
	virtual int deleteFunction (const char_t* name);

	/**
	 * Enables the run-time callback
	 */
	virtual void enableRunCallback ();

	/**
	 * Disables the run-time callback
	 */
	virtual void disableRunCallback ();

protected:
	virtual int dispatchFunction (Run* run, const char_t* name, size_t len);

	/** 
	 * @name Source code I/O handlers
	 * A subclass should override the following methods to support the 
	 * source code input and output. The awk interpreter calls the 
	 * following methods when the parse method is invoked.
	 *
	 * To read the source code, Awk::parse calls Awk::openSource, 
	 * Awk::readSource, and Awk::closeSource as shown in the diagram below.
	 * Any failures wll cause Awk::parse to return an error.
	 *
	 * \image html awk-srcio-read.png
	 *
	 * Awk::parse is able to write back the internal parse tree by
	 * calling Awk::openSource, Awk::writeSource, and Awk::closeSource
	 * as shown in the diagram below. Any failures will cause Awk::parse
	 * to return an error.
	 *
	 * \image html awk-srcio-write.png
	 *
	 * Awk::parse passes an instance of Awk::Source when invoking these
	 * methods. You can determine the context of the method by calling
	 * Awk::Source::getMode and inspecting its return value. You may use
	 * Awk::Source::getHandle and Awk::Source::setHandle to store and
	 * retrieve the data information needed to complete the operation.
	 */
	/*@{*/
	/** 
	 * Opens the source code stream. 
	 * A subclass should override this method. It should return 1 on
	 * success, -1 on failure, and 0 if the opening operation
	 * is successful but has reached the end of the stream.
	 * @param io  I/O context passed from Awk::parse
	 * @see Awk::Source::getMode, Awk::Source::setHandle
	 */
	virtual int openSource  (Source& io) = 0;

	/** 
	 * Closes the source code stream.
	 * A subclass should override this method. It should return 0 on
	 * success and -1 on failure.
	 * @param io  I/O context passed from Awk::parse
	 * @see Awk::Source::getMode, Awk::Source::getHandle
	 */
	virtual int closeSource (Source& io) = 0;

	/** 
	 * Reads from the source code input stream.
	 * A subclass should override this method. It should return 0 when
	 * it has reached the end of the stream and -1 on falure.
	 * When it has data to return, it should read characters not longer
	 * than len characters, fill the buffer pointed at by buf with them,
	 * and return the number of the charaters read.
	 * @param io  I/O context passed from Awk::parse
	 * @param buf pointer to a character buffer
	 * @param len number of characters in the buffer
	 */
	virtual ssize_t readSource  (Source& io, char_t* buf, size_t len) = 0;

	/** 
	 * Writes to the source code output stream.
	 * A subclass should override this method. It should return 0 when
	 * it has reachedthe end of the stream and -1 on failure.
	 * It should write up to len characters from the buffer pointed at
	 * by buf and return the number of characters written.
	 * @param io  I/O context passed from Awk::parse
	 * @param buf pointer to a character buffer
	 * @param len size of the buffer in characters
	 */
	virtual ssize_t writeSource (Source& io, char_t* buf, size_t len) = 0;
	/*@}*/
	
	/** 
	 * @name Pipe I/O handlers
	 * Pipe operations are achieved through the following methods.
	 */
	/*@{*/
	virtual int     openPipe  (Pipe& io) = 0;
	virtual int     closePipe (Pipe& io) = 0;
	virtual ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writePipe (Pipe& io, const char_t* buf, size_t len) = 0;
	virtual int     flushPipe (Pipe& io) = 0;
	/*@}*/

	/** 
	 * @name File I/O handlers
	 * File operations are achieved through the following methods.
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
	 * Console operations are achieved through the following methods.
	 */
	virtual int     openConsole  (Console& io) = 0;
	virtual int     closeConsole (Console& io) = 0;
	virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeConsole (Console& io, const char_t* buf, size_t len) = 0;
	virtual int     flushConsole (Console& io) = 0;
	virtual int     nextConsole  (Console& io) = 0;
	/*@}*/

	// run-time callbacks
	virtual bool onLoopEnter (Run& run);
	virtual void onLoopExit (Run& run, const Argument& ret);
	virtual void onStatement (Run& run, size_t line);
	
	// primitive handlers 
	virtual real_t pow (real_t x, real_t y) = 0;
	virtual int    vsprintf (char_t* buf, size_t size,
	                         const char_t* fmt, va_list arg) = 0;

	// static glue members for various handlers
	static ssize_t sourceReader (
		awk_t* awk, qse_awk_sio_cmd_t cmd, char_t* data, size_t count);
	static ssize_t sourceWriter (
		awk_t* awk, qse_awk_sio_cmd_t cmd, char_t* data, size_t count);

	static ssize_t pipeHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t fileHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);
	static ssize_t consoleHandler (
		rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
		char_t* data, size_t count);

	static int functionHandler (
		rtx_t* rtx, const char_t* name, size_t len);
	static void freeFunctionMapValue (map_t* map, void* dptr, size_t dlen);

	static int  onLoopEnter (rtx_t* run, void* data);
	static void onLoopExit (rtx_t* run, val_t* ret, void* data);
	static void onStatement (rtx_t* run, size_t line, void* data);

	static real_t pow     (awk_t* data, real_t x, real_t y);
	static int    sprintf (awk_t* data, char_t* buf, size_t size,
	                       const char_t* fmt, ...);

protected:
	awk_t* awk;
	errstr_t dflerrstr;
	map_t* functionMap;

	Source sourceIn;
	Source sourceOut;

	ErrorNumber errnum;
	size_t      errlin;
	char_t      errmsg[256];

	bool        runCallback;

	struct
	{
		qse_xstr_t* ptr;
		size_t      len;
		size_t      capa;
	} runarg;

private:
	static const char_t* xerrstr (awk_t* a, errnum_t num) throw ();

private:
	Awk (const Awk&);
	Awk& operator= (const Awk&);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
