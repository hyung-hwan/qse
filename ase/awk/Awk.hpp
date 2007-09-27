/*
 * $Id: Awk.hpp,v 1.61 2007/09/25 15:27:54 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_AWK_AWK_HPP_
#define _ASE_AWK_AWK_HPP_

#include <ase/awk/awk.h>
#include <ase/awk/map.h>
#include <stdarg.h>

/////////////////////////////////
ASE_BEGIN_NAMESPACE(ASE)
/////////////////////////////////

/** 
 * Represents the AWK interpreter engine
 */
class Awk
{
public:
	/** Boolean data type */
	typedef ase_bool_t  bool_t;
	/** Data type that can hold any character */
	typedef ase_char_t  char_t;
	/** Data type that can hold any character or an end-of-file value */
	typedef ase_cint_t  cint_t;
	/** Represents an unsigned integer number of the same size as void* */
	typedef ase_size_t  size_t;
	/** Signed version of size_t */
	typedef ase_ssize_t ssize_t;
	/** Represents an integer */
	typedef ase_long_t  long_t;
	/** Represents a floating-point number */
	typedef ase_real_t  real_t;
	/** Represents an internal awk value */
	typedef ase_awk_val_t val_t;
	/** Represents the internal hash table */
	typedef ase_awk_map_t map_t;
	/** Represents a key/value pair */
	typedef ase_awk_pair_t pair_t;
	/** Represents the external I/O context */
	typedef ase_awk_extio_t extio_t;
	/** Represents the run-time context */
	typedef ase_awk_run_t run_t;
	/** Represents the underlying interpreter */
	typedef ase_awk_t awk_t;

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
		 * ASE_NULL is returned if it has not been set with 
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
		 *         Source::setHandle or ASE_NULL
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
	 * Extio class 
	 */
	class Extio
	{
	protected:
		Extio (extio_t* extio);

	public:
		const char_t* getName() const;
		const void* getHandle () const;
		void  setHandle (void* handle);

		/** 
		 * Returns the underlying extio_t handle
		 */
		const extio_t* getRawExtio () const;

		/**
		 * Returns the underlying run_t handle associated 
		 * with the underlying extio_t handle
		 */
		const run_t* getRawRun () const;

	protected:
		extio_t* extio;
	};

	/**
	 * Pipe
	 */
	class Pipe: public Extio
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = ASE_AWK_EXTIO_PIPE_READ,
			WRITE = ASE_AWK_EXTIO_PIPE_WRITE
		};

	protected:
		Pipe (extio_t* extio);

	public:
		Mode getMode () const;
	};

	/**
	 * File
	 */
	class File: public Extio
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = ASE_AWK_EXTIO_FILE_READ,
			WRITE = ASE_AWK_EXTIO_FILE_WRITE,
			APPEND = ASE_AWK_EXTIO_FILE_APPEND
		};

	protected:
		File (extio_t* extio);

	public:
		Mode getMode () const;
	};

	/**
	 * Console
	 */
	class Console: public Extio
	{
	public:
		friend class Awk;

		enum Mode
		{
			READ = ASE_AWK_EXTIO_CONSOLE_READ,
			WRITE = ASE_AWK_EXTIO_CONSOLE_WRITE
		};

	protected:
		Console (extio_t* extio);
		~Console ();

	public:
		Mode getMode () const;
		int setFileName (const char_t* name);

	protected:
		char_t* filename;
	};

	class Run;

	/**
	 * Represents an argument to an intrinsic function
	 */
	class Argument
	{
	public:
		friend class Awk;
		friend class Awk::Run;

		Argument ();
		~Argument ();

	protected:
		void clear ();

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
		int init (run_t* run, val_t* v);

	public:
		long_t toInt () const;
		real_t toReal () const;
		const char_t* toStr (size_t* len) const;

	protected:
		run_t* run;
		val_t* val;

		ase_long_t inum;
		ase_real_t rnum;

		struct
		{
			char_t*        ptr;
			size_t         len;
		} str;
	};

	/**
	 * Represents a return value of an intrinsic function
	 */
	class Return
	{
	protected:
		friend class Awk;

		Return (awk_t* awk);
		~Return ();

		val_t* toVal (run_t* run) const;

	public:
		int set (long_t v);
		int set (real_t v); 
		int set (const char_t* ptr, size_t len);

		void clear ();

	protected:
		awk_t* awk;
		int type;

		union 
		{
			ase_long_t inum;
			ase_real_t rnum;

			struct
			{
				char_t* ptr;
				size_t  len;
			} str;
		} v;
	};

	// generated by generrcode.awk
	/** Defines the error code */
	enum ErrorCode
	{
		ERR_NOERR = ASE_AWK_ENOERR,
		ERR_INVAL = ASE_AWK_EINVAL,
		ERR_NOMEM = ASE_AWK_ENOMEM,
		ERR_NOSUP = ASE_AWK_ENOSUP,
		ERR_NOPER = ASE_AWK_ENOPER,
		ERR_NODEV = ASE_AWK_ENODEV,
		ERR_NOSPC = ASE_AWK_ENOSPC,
		ERR_MFILE = ASE_AWK_EMFILE,
		ERR_MLINK = ASE_AWK_EMLINK,
		ERR_AGAIN = ASE_AWK_EAGAIN,
		ERR_NOENT = ASE_AWK_ENOENT,
		ERR_EXIST = ASE_AWK_EEXIST,
		ERR_FTBIG = ASE_AWK_EFTBIG,
		ERR_TBUSY = ASE_AWK_ETBUSY,
		ERR_ISDIR = ASE_AWK_EISDIR,
		ERR_IOERR = ASE_AWK_EIOERR,
		ERR_OPEN = ASE_AWK_EOPEN,
		ERR_READ = ASE_AWK_EREAD,
		ERR_WRITE = ASE_AWK_EWRITE,
		ERR_CLOSE = ASE_AWK_ECLOSE,
		ERR_INTERN = ASE_AWK_EINTERN,
		ERR_RUNTIME = ASE_AWK_ERUNTIME,
		ERR_BLKNST = ASE_AWK_EBLKNST,
		ERR_EXPRNST = ASE_AWK_EEXPRNST,
		ERR_SINOP = ASE_AWK_ESINOP,
		ERR_SINCL = ASE_AWK_ESINCL,
		ERR_SINRD = ASE_AWK_ESINRD,
		ERR_SOUTOP = ASE_AWK_ESOUTOP,
		ERR_SOUTCL = ASE_AWK_ESOUTCL,
		ERR_SOUTWR = ASE_AWK_ESOUTWR,
		ERR_LXCHR = ASE_AWK_ELXCHR,
		ERR_LXDIG = ASE_AWK_ELXDIG,
		ERR_LXUNG = ASE_AWK_ELXUNG,
		ERR_ENDSRC = ASE_AWK_EENDSRC,
		ERR_ENDCMT = ASE_AWK_EENDCMT,
		ERR_ENDSTR = ASE_AWK_EENDSTR,
		ERR_ENDREX = ASE_AWK_EENDREX,
		ERR_LBRACE = ASE_AWK_ELBRACE,
		ERR_LPAREN = ASE_AWK_ELPAREN,
		ERR_RPAREN = ASE_AWK_ERPAREN,
		ERR_RBRACK = ASE_AWK_ERBRACK,
		ERR_COMMA = ASE_AWK_ECOMMA,
		ERR_SCOLON = ASE_AWK_ESCOLON,
		ERR_COLON = ASE_AWK_ECOLON,
		ERR_STMEND = ASE_AWK_ESTMEND,
		ERR_IN = ASE_AWK_EIN,
		ERR_NOTVAR = ASE_AWK_ENOTVAR,
		ERR_EXPRES = ASE_AWK_EEXPRES,
		ERR_WHILE = ASE_AWK_EWHILE,
		ERR_ASSIGN = ASE_AWK_EASSIGN,
		ERR_IDENT = ASE_AWK_EIDENT,
		ERR_FNNAME = ASE_AWK_EFNNAME,
		ERR_BLKBEG = ASE_AWK_EBLKBEG,
		ERR_BLKEND = ASE_AWK_EBLKEND,
		ERR_DUPBEG = ASE_AWK_EDUPBEG,
		ERR_DUPEND = ASE_AWK_EDUPEND,
		ERR_BFNRED = ASE_AWK_EBFNRED,
		ERR_AFNRED = ASE_AWK_EAFNRED,
		ERR_GBLRED = ASE_AWK_EGBLRED,
		ERR_PARRED = ASE_AWK_EPARRED,
		ERR_DUPPAR = ASE_AWK_EDUPPAR,
		ERR_DUPGBL = ASE_AWK_EDUPGBL,
		ERR_DUPLCL = ASE_AWK_EDUPLCL,
		ERR_BADPAR = ASE_AWK_EBADPAR,
		ERR_BADVAR = ASE_AWK_EBADVAR,
		ERR_UNDEF = ASE_AWK_EUNDEF,
		ERR_LVALUE = ASE_AWK_ELVALUE,
		ERR_GBLTM = ASE_AWK_EGBLTM,
		ERR_LCLTM = ASE_AWK_ELCLTM,
		ERR_PARTM = ASE_AWK_EPARTM,
		ERR_DELETE = ASE_AWK_EDELETE,
		ERR_BREAK = ASE_AWK_EBREAK,
		ERR_CONTINUE = ASE_AWK_ECONTINUE,
		ERR_NEXTBEG = ASE_AWK_ENEXTBEG,
		ERR_NEXTEND = ASE_AWK_ENEXTEND,
		ERR_NEXTFBEG = ASE_AWK_ENEXTFBEG,
		ERR_NEXTFEND = ASE_AWK_ENEXTFEND,
		ERR_PRINTFARG = ASE_AWK_EPRINTFARG,
		ERR_PREPST = ASE_AWK_EPREPST,
		ERR_GLNCPS = ASE_AWK_EGLNCPS,
		ERR_DIVBY0 = ASE_AWK_EDIVBY0,
		ERR_OPERAND = ASE_AWK_EOPERAND,
		ERR_POSIDX = ASE_AWK_EPOSIDX,
		ERR_ARGTF = ASE_AWK_EARGTF,
		ERR_ARGTM = ASE_AWK_EARGTM,
		ERR_FNNONE = ASE_AWK_EFNNONE,
		ERR_NOTIDX = ASE_AWK_ENOTIDX,
		ERR_NOTDEL = ASE_AWK_ENOTDEL,
		ERR_NOTMAP = ASE_AWK_ENOTMAP,
		ERR_NOTMAPIN = ASE_AWK_ENOTMAPIN,
		ERR_NOTMAPNILIN = ASE_AWK_ENOTMAPNILIN,
		ERR_NOTREF = ASE_AWK_ENOTREF,
		ERR_NOTASS = ASE_AWK_ENOTASS,
		ERR_IDXVALASSMAP = ASE_AWK_EIDXVALASSMAP,
		ERR_POSVALASSMAP = ASE_AWK_EPOSVALASSMAP,
		ERR_MAPTOSCALAR = ASE_AWK_EMAPTOSCALAR,
		ERR_SCALARTOMAP = ASE_AWK_ESCALARTOMAP,
		ERR_MAPNOTALLOWED = ASE_AWK_EMAPNOTALLOWED,
		ERR_VALTYPE = ASE_AWK_EVALTYPE,
		ERR_RDELETE = ASE_AWK_ERDELETE,
		ERR_RNEXTBEG = ASE_AWK_ERNEXTBEG,
		ERR_RNEXTEND = ASE_AWK_ERNEXTEND,
		ERR_RNEXTFBEG = ASE_AWK_ERNEXTFBEG,
		ERR_RNEXTFEND = ASE_AWK_ERNEXTFEND,
		ERR_BFNUSER = ASE_AWK_EBFNUSER,
		ERR_BFNIMPL = ASE_AWK_EBFNIMPL,
		ERR_IOUSER = ASE_AWK_EIOUSER,
		ERR_IONONE = ASE_AWK_EIONONE,
		ERR_IOIMPL = ASE_AWK_EIOIMPL,
		ERR_IONMEM = ASE_AWK_EIONMEM,
		ERR_IONMNL = ASE_AWK_EIONMNL,
		ERR_FMTARG = ASE_AWK_EFMTARG,
		ERR_FMTCNV = ASE_AWK_EFMTCNV,
		ERR_CONVFMTCHR = ASE_AWK_ECONVFMTCHR,
		ERR_OFMTCHR = ASE_AWK_EOFMTCHR,
		ERR_REXRECUR = ASE_AWK_EREXRECUR,
		ERR_REXRPAREN = ASE_AWK_EREXRPAREN,
		ERR_REXRBRACKET = ASE_AWK_EREXRBRACKET,
		ERR_REXRBRACE = ASE_AWK_EREXRBRACE,
		ERR_REXUNBALPAR = ASE_AWK_EREXUNBALPAR,
		ERR_REXCOLON = ASE_AWK_EREXCOLON,
		ERR_REXCRANGE = ASE_AWK_EREXCRANGE,
		ERR_REXCCLASS = ASE_AWK_EREXCCLASS,
		ERR_REXBRANGE = ASE_AWK_EREXBRANGE,
		ERR_REXEND = ASE_AWK_EREXEND,
		ERR_REXGARBAGE = ASE_AWK_EREXGARBAGE,
	};
	// end of enum ErrorCode


	// generated by genoptcode.awk
	/** Defines options */
	enum Option
	{
		OPT_IMPLICIT = ASE_AWK_IMPLICIT,
		OPT_EXPLICIT = ASE_AWK_EXPLICIT,
		OPT_UNIQUEFN = ASE_AWK_UNIQUEFN,
		OPT_SHADING = ASE_AWK_SHADING,
		OPT_SHIFT = ASE_AWK_SHIFT,
		OPT_IDIV = ASE_AWK_IDIV,
		OPT_STRCONCAT = ASE_AWK_STRCONCAT,
		OPT_EXTIO = ASE_AWK_EXTIO,
		OPT_COPROC = ASE_AWK_COPROC,
		OPT_BLOCKLESS = ASE_AWK_BLOCKLESS,
		OPT_BASEONE = ASE_AWK_BASEONE,
		OPT_STRIPSPACES = ASE_AWK_STRIPSPACES,
		/** Support the nextofile statement */
		OPT_NEXTOFILE = ASE_AWK_NEXTOFILE,
		/** Use CR+LF instead of LF for line breaking. */
		OPT_CRLF = ASE_AWK_CRLF,
		/** 
		 * When set, the values specified in a call to Awk::run 
		 * as the second and the third parameter are passed to 
		 * the function specified as the first parameter.
		 */
		OPT_ARGSTOMAIN = ASE_AWK_ARGSTOMAIN,
		/** Enables the keyword 'reset' */
		OPT_RESET = ASE_AWK_RESET
	};
	// end of enum Option

	/** Represents the execution context */
	class Run
	{
	protected:
		friend class Awk;

		Run (Awk* awk);
		Run (Awk* awk, run_t* run);
		~Run ();

	public:
		int stop () const;

		ErrorCode getErrorCode () const;
		size_t getErrorLine () const;
		const char_t* getErrorMessage () const;

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
		 * Gets the value of a global variable.
		 *
		 * @param id
		 * 	The ID to a global variable. This value corresponds
		 * 	to the predefined global variable IDs or the value
		 * 	returned by Awk::addGlobal.
		 * @param global
		 * 	The reference to the value holder of a global variable
		 * 	indicated by id. The parameter is set if thie method
		 * 	returns 0.
		 *
		 * @return 
		 * 	On success, 0 is returned.
		 * 	On failure, -1 is returned.
		 */
		int getGlobal (int id, Argument& global) const;

	protected:
		Awk* awk;
		run_t* run;
		//mutable Value global;
		bool callbackFailed;
	};

	/** Constructor */
	Awk ();
	/** Destructor */
	virtual ~Awk ();
	
	/** Returns the error code */
	ErrorCode getErrorCode () const;

	/** Returns the line of the source code where the error occurred */
	size_t getErrorLine () const ;

	/** Returns the error message */
	const char_t* getErrorMessage () const;

protected:
	void setError (ErrorCode code, size_t line = 0, 
		const char_t* arg = ASE_NULL, size_t len = 0);
	void setError (ErrorCode code, size_t line, const char_t* msg);

	void clearError ();
	void retrieveError ();

public:
	/**
	 * Opens the interpreter. 
	 *
	 * An application should call this method before doing anything 
	 * meaningful to the instance of this class.
	 *
	 * @return
	 * 	On success, 0 is returned. On failure -1 is returned and
	 * 	extended error information is set. Call Awk::getErrorCode
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
		DEPTH_BLOCK_PARSE = ASE_AWK_DEPTH_BLOCK_PARSE,
		DEPTH_BLOCK_RUN   = ASE_AWK_DEPTH_BLOCK_RUN,
		DEPTH_EXPR_PARSE  = ASE_AWK_DEPTH_EXPR_PARSE,
		DEPTH_EXPR_RUN    = ASE_AWK_DEPTH_EXPR_RUN,
		DEPTH_REX_BUILD   = ASE_AWK_DEPTH_REX_BUILD,
		DEPTH_REX_MATCH   = ASE_AWK_DEPTH_REX_MATCH
	};

	/** Sets the maximum depth */
	virtual void   setMaxDepth (int ids, size_t depth);
	/** Gets the maximum depth */
	virtual size_t getMaxDepth (int id) const;

	virtual const char_t* getErrorString (ErrorCode num) const;
	virtual int setErrorString (ErrorCode num, const char_t* str);

	virtual int setWord (
		const char_t* ow, const char_t* nw);
	virtual int setWord (
		const char_t* ow, ase_size_t owl,
		const char_t* nw, ase_size_t nwl);

	virtual int unsetWord (const char_t* ow);
	virtual int unsetWord (const char_t* ow, ase_size_t owl);
	virtual int unsetAllWords ();

	/**
	 * Parses the source code.
	 *
	 * Awk::parse parses the source code read from the input stream and 
	 * writes the parse tree to the output stream. A child class should
	 * override Awk::openSource, Awk::closeSource, Awk::readSource, 
	 * Awk::writeSource to implement the source code stream.
	 *
	 * @return 
	 * 	On success, 0 is returned. On failure, -1 is returned and 
	 * 	extended error information is set. Call Awk::getErrorCode
	 * 	to get it.
	 */
	virtual int parse ();

	/**
	 * Executes the parse tree.
	 *
	 * This method executes the parse tree formed by Awk::parse.
	 *
	 * @param main Name of an entry point.
	 * 	If it is set, Awk::run executes the function of the specified 
	 * 	name instead of entering BEGIN/pattern/END blocks.
	 * @param args Pointer to an array of character strings.
	 * 	If it is specified, the charater strings are passed to
	 * 	an AWK program. The values can be accesed with ARGC & ARGV
	 * 	inside the AWK program. If Awk::OPT_ARGSTOMAIN is set and 
	 * 	the name of entry point is specified, the values are 
	 * 	accessible as arguments to the entry point function.
	 * 	In this case, the number of arguments specified in the 
	 * 	function definition should not exceed the number of
	 * 	character string passed here.
	 * @param nargs Number of character strings in the array
	 *
	 * @return
	 * 	On success, 0 is returned. On failure, -1 is returned if 
	 * 	the run-time callback is not enabled. If the run-time callback
	 * 	is enabled, 0 is returned and the error is indicated through
	 * 	Awk::onRunEnd. The run-time callback is enabled and disbaled 
	 * 	with Awk::enableRunCallback and Awk::disableRunCallback.
	 * 	Call Awk::getErrorCode to get extended error information.
	 */
	virtual int run (const char_t* main = ASE_NULL, 
	         const char_t** args = ASE_NULL, size_t nargs = 0);

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
	virtual int dispatchFunction (
		run_t* run, const char_t* name, size_t len);

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
	 * retrieve the custom information needed to complete the operation.
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
	virtual ssize_t writePipe (Pipe& io, char_t* buf, size_t len) = 0;
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
	virtual ssize_t writeFile (File& io, char_t* buf, size_t len) = 0;
	virtual int     flushFile (File& io) = 0;
	/*@}*/

	/** 
	 * @name Console I/O handlers
	 * Console operations are achieved through the following methods.
	 */
	virtual int     openConsole  (Console& io) = 0;
	virtual int     closeConsole (Console& io) = 0;
	virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
	virtual ssize_t writeConsole (Console& io, char_t* buf, size_t len) = 0;
	virtual int     flushConsole (Console& io) = 0;
	virtual int     nextConsole  (Console& io) = 0;
	/*@}*/

	// run-time callbacks
	virtual void onRunStart (const Run& run);
	virtual void onRunEnd (const Run& run);
	virtual void onRunReturn (const Run& run, const Argument& ret);
	virtual void onRunStatement (const Run& run, size_t line);
	
	// primitive handlers 
	virtual void* allocMem   (size_t n) = 0;
	virtual void* reallocMem (void* ptr, size_t n) = 0;
	virtual void  freeMem    (void* ptr) = 0;

	virtual bool_t isUpper  (cint_t c) = 0;
	virtual bool_t isLower  (cint_t c) = 0;
	virtual bool_t isAlpha  (cint_t c) = 0;
	virtual bool_t isDigit  (cint_t c) = 0;
	virtual bool_t isXdigit (cint_t c) = 0;
	virtual bool_t isAlnum  (cint_t c) = 0;
	virtual bool_t isSpace  (cint_t c) = 0;
	virtual bool_t isPrint  (cint_t c) = 0;
	virtual bool_t isGraph  (cint_t c) = 0;
	virtual bool_t isCntrl  (cint_t c) = 0;
	virtual bool_t isPunct  (cint_t c) = 0;
	virtual cint_t toUpper  (cint_t c) = 0;
	virtual cint_t toLower  (cint_t c) = 0;

	virtual real_t pow (real_t x, real_t y) = 0;
	virtual int    vsprintf (char_t* buf, size_t size,
	                         const char_t* fmt, va_list arg) = 0;
	virtual void   vdprintf (const char_t* fmt, va_list arg) = 0;

	// static glue members for various handlers
	static ssize_t sourceReader (
        		int cmd, void* arg, char_t* data, size_t count);
	static ssize_t sourceWriter (
        		int cmd, void* arg, char_t* data, size_t count);

	static ssize_t pipeHandler (
        		int cmd, void* arg, char_t* data, size_t count);
	static ssize_t fileHandler (
        		int cmd, void* arg, char_t* data, size_t count);
	static ssize_t consoleHandler (
        		int cmd, void* arg, char_t* data, size_t count);

	static int functionHandler (
		run_t* run, const char_t* name, size_t len);
	static void freeFunctionMapValue (void* owner, void* value);

	static void onRunStart (run_t* run, void* custom);
	static void onRunEnd (run_t* run, int errnum, void* custom);
	static void onRunReturn (run_t* run, val_t* ret, void* custom);
	static void onRunStatement (run_t* run, size_t line, void* custom);

	static void* allocMem   (void* custom, size_t n);
	static void* reallocMem (void* custom, void* ptr, size_t n);
	static void  freeMem    (void* custom, void* ptr);

	static bool_t isUpper  (void* custom, cint_t c); 
	static bool_t isLower  (void* custom, cint_t c); 
	static bool_t isAlpha  (void* custom, cint_t c);
	static bool_t isDigit  (void* custom, cint_t c);
	static bool_t isXdigit (void* custom, cint_t c);
	static bool_t isAlnum  (void* custom, cint_t c);
	static bool_t isSpace  (void* custom, cint_t c);
	static bool_t isPrint  (void* custom, cint_t c);
	static bool_t isGraph  (void* custom, cint_t c);
	static bool_t isCntrl  (void* custom, cint_t c);
	static bool_t isPunct  (void* custom, cint_t c);
	static cint_t toUpper  (void* custom, cint_t c);
	static cint_t toLower  (void* custom, cint_t c);

	static real_t pow     (void* custom, real_t x, real_t y);
	static int    sprintf (void* custom, char_t* buf, size_t size,
	                       const char_t* fmt, ...);
	static void   dprintf (void* custom, const char_t* fmt, ...);

protected:
	awk_t* awk;
	map_t* functionMap;

	Source sourceIn;
	Source sourceOut;

	ErrorCode errnum;
	size_t    errlin;
	char_t    errmsg[256];

	bool      runCallback;

private:
	Awk (const Awk&);
	Awk& operator= (const Awk&);
};

/////////////////////////////////
ASE_END_NAMESPACE(ASE)
/////////////////////////////////

#endif
