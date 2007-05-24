/*
 * $Id: Awk.hpp,v 1.35 2007/05/23 11:43:24 bacon Exp $
 */

#ifndef _ASE_AWK_AWK_HPP_
#define _ASE_AWK_AWK_HPP_

#include <ase/awk/awk.h>
#include <ase/awk/map.h>
#include <stdarg.h>

namespace ASE
{

	class Awk
	{
	public:
		typedef ase_bool_t  bool_t;
		typedef ase_char_t  char_t;
		typedef ase_cint_t  cint_t;
		typedef ase_size_t  size_t;
		typedef ase_ssize_t ssize_t;
		typedef ase_long_t  long_t;
		typedef ase_real_t  real_t;

		typedef ase_awk_val_t val_t;
		typedef ase_awk_map_t map_t;
		typedef ase_awk_pair_t pair_t;
		typedef ase_awk_extio_t extio_t;
		typedef ase_awk_run_t run_t;
		typedef ase_awk_t awk_t;

		class Source
		{
		public:
			enum Mode
			{	
				READ,
				WRITE
			};

			Source (Mode mode);

			Mode getMode() const;
			const void* getHandle () const;
			void  setHandle (void* handle);

		private:
			Mode  mode;
			void* handle;
		};

		class Extio
		{
		protected:
			Extio (extio_t* extio);

		public:
			const char_t* getName() const;
			const void* getHandle () const;
			void  setHandle (void* handle);

		protected:
			extio_t* extio;
		};

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

		class Argument
		{
		protected:
			friend class Awk;

			Argument ();
			~Argument ();

			// initialization
			void* operator new (size_t n, awk_t* awk) throw ();
			void* operator new[] (size_t n, awk_t* awk) throw ();

			// deletion when initialization fails
			void operator delete (void* p, awk_t* awk);
			void operator delete[] (void* p, awk_t* awk);

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

		class Return
		{
		protected:
			friend class Awk;

			Return (run_t* run);
			~Return ();

			val_t* toVal () const;

		public:
			int set (long_t v);
			int set (real_t v); 
			int set (char_t* ptr, size_t len);
			void clear ();

		protected:
			run_t* run;
			int type;

			union 
			{
				ase_long_t inum;
				ase_real_t rnum;

				struct
				{
					char_t*        ptr;
					size_t         len;
				} str;
			} v;
		};

		// generated by generrcode.awk
		enum ErrorCode
		{
			E_NOERR = ASE_AWK_ENOERR,
			E_INVAL = ASE_AWK_EINVAL,
			E_NOMEM = ASE_AWK_ENOMEM,
			E_NOSUP = ASE_AWK_ENOSUP,
			E_NOPER = ASE_AWK_ENOPER,
			E_NODEV = ASE_AWK_ENODEV,
			E_NOSPC = ASE_AWK_ENOSPC,
			E_MFILE = ASE_AWK_EMFILE,
			E_MLINK = ASE_AWK_EMLINK,
			E_AGAIN = ASE_AWK_EAGAIN,
			E_NOENT = ASE_AWK_ENOENT,
			E_EXIST = ASE_AWK_EEXIST,
			E_FTBIG = ASE_AWK_EFTBIG,
			E_TBUSY = ASE_AWK_ETBUSY,
			E_ISDIR = ASE_AWK_EISDIR,
			E_IOERR = ASE_AWK_EIOERR,
			E_OPEN = ASE_AWK_EOPEN,
			E_READ = ASE_AWK_EREAD,
			E_WRITE = ASE_AWK_EWRITE,
			E_CLOSE = ASE_AWK_ECLOSE,
			E_INTERN = ASE_AWK_EINTERN,
			E_RUNTIME = ASE_AWK_ERUNTIME,
			E_BLKNST = ASE_AWK_EBLKNST,
			E_EXPRNST = ASE_AWK_EEXPRNST,
			E_SINOP = ASE_AWK_ESINOP,
			E_SINCL = ASE_AWK_ESINCL,
			E_SINRD = ASE_AWK_ESINRD,
			E_SOUTOP = ASE_AWK_ESOUTOP,
			E_SOUTCL = ASE_AWK_ESOUTCL,
			E_SOUTWR = ASE_AWK_ESOUTWR,
			E_LXCHR = ASE_AWK_ELXCHR,
			E_LXUNG = ASE_AWK_ELXUNG,
			E_ENDSRC = ASE_AWK_EENDSRC,
			E_ENDCMT = ASE_AWK_EENDCMT,
			E_ENDSTR = ASE_AWK_EENDSTR,
			E_ENDREX = ASE_AWK_EENDREX,
			E_LBRACE = ASE_AWK_ELBRACE,
			E_LPAREN = ASE_AWK_ELPAREN,
			E_RPAREN = ASE_AWK_ERPAREN,
			E_RBRACK = ASE_AWK_ERBRACK,
			E_COMMA = ASE_AWK_ECOMMA,
			E_SCOLON = ASE_AWK_ESCOLON,
			E_COLON = ASE_AWK_ECOLON,
			E_STMEND = ASE_AWK_ESTMEND,
			E_IN = ASE_AWK_EIN,
			E_NOTVAR = ASE_AWK_ENOTVAR,
			E_EXPRES = ASE_AWK_EEXPRES,
			E_WHILE = ASE_AWK_EWHILE,
			E_ASSIGN = ASE_AWK_EASSIGN,
			E_IDENT = ASE_AWK_EIDENT,
			E_FNNAME = ASE_AWK_EFNNAME,
			E_BLKBEG = ASE_AWK_EBLKBEG,
			E_BLKEND = ASE_AWK_EBLKEND,
			E_DUPBEG = ASE_AWK_EDUPBEG,
			E_DUPEND = ASE_AWK_EDUPEND,
			E_BFNRED = ASE_AWK_EBFNRED,
			E_AFNRED = ASE_AWK_EAFNRED,
			E_GBLRED = ASE_AWK_EGBLRED,
			E_PARRED = ASE_AWK_EPARRED,
			E_DUPPAR = ASE_AWK_EDUPPAR,
			E_DUPGBL = ASE_AWK_EDUPGBL,
			E_DUPLCL = ASE_AWK_EDUPLCL,
			E_BADPAR = ASE_AWK_EBADPAR,
			E_BADVAR = ASE_AWK_EBADVAR,
			E_UNDEF = ASE_AWK_EUNDEF,
			E_LVALUE = ASE_AWK_ELVALUE,
			E_GBLTM = ASE_AWK_EGBLTM,
			E_LCLTM = ASE_AWK_ELCLTM,
			E_PARTM = ASE_AWK_EPARTM,
			E_DELETE = ASE_AWK_EDELETE,
			E_BREAK = ASE_AWK_EBREAK,
			E_CONTINUE = ASE_AWK_ECONTINUE,
			E_NEXTBEG = ASE_AWK_ENEXTBEG,
			E_NEXTEND = ASE_AWK_ENEXTEND,
			E_NEXTFBEG = ASE_AWK_ENEXTFBEG,
			E_NEXTFEND = ASE_AWK_ENEXTFEND,
			E_PRINTFARG = ASE_AWK_EPRINTFARG,
			E_PREPST = ASE_AWK_EPREPST,
			E_GLNCPS = ASE_AWK_EGLNCPS,
			E_DIVBY0 = ASE_AWK_EDIVBY0,
			E_OPERAND = ASE_AWK_EOPERAND,
			E_POSIDX = ASE_AWK_EPOSIDX,
			E_ARGTF = ASE_AWK_EARGTF,
			E_ARGTM = ASE_AWK_EARGTM,
			E_FNNONE = ASE_AWK_EFNNONE,
			E_NOTIDX = ASE_AWK_ENOTIDX,
			E_NOTDEL = ASE_AWK_ENOTDEL,
			E_NOTMAP = ASE_AWK_ENOTMAP,
			E_NOTMAPIN = ASE_AWK_ENOTMAPIN,
			E_NOTMAPNILIN = ASE_AWK_ENOTMAPNILIN,
			E_NOTREF = ASE_AWK_ENOTREF,
			E_NOTASS = ASE_AWK_ENOTASS,
			E_IDXVALASSMAP = ASE_AWK_EIDXVALASSMAP,
			E_POSVALASSMAP = ASE_AWK_EPOSVALASSMAP,
			E_MAPTOSCALAR = ASE_AWK_EMAPTOSCALAR,
			E_SCALARTOMAP = ASE_AWK_ESCALARTOMAP,
			E_MAPNOTALLOWED = ASE_AWK_EMAPNOTALLOWED,
			E_VALTYPE = ASE_AWK_EVALTYPE,
			E_RDELETE = ASE_AWK_ERDELETE,
			E_RNEXTBEG = ASE_AWK_ERNEXTBEG,
			E_RNEXTEND = ASE_AWK_ERNEXTEND,
			E_RNEXTFBEG = ASE_AWK_ERNEXTFBEG,
			E_RNEXTFEND = ASE_AWK_ERNEXTFEND,
			E_BFNUSER = ASE_AWK_EBFNUSER,
			E_BFNIMPL = ASE_AWK_EBFNIMPL,
			E_IOUSER = ASE_AWK_EIOUSER,
			E_IONONE = ASE_AWK_EIONONE,
			E_IOIMPL = ASE_AWK_EIOIMPL,
			E_IONMEM = ASE_AWK_EIONMEM,
			E_IONMNL = ASE_AWK_EIONMNL,
			E_FMTARG = ASE_AWK_EFMTARG,
			E_FMTCNV = ASE_AWK_EFMTCNV,
			E_CONVFMTCHR = ASE_AWK_ECONVFMTCHR,
			E_OFMTCHR = ASE_AWK_EOFMTCHR,
			E_REXRECUR = ASE_AWK_EREXRECUR,
			E_REXRPAREN = ASE_AWK_EREXRPAREN,
			E_REXRBRACKET = ASE_AWK_EREXRBRACKET,
			E_REXRBRACE = ASE_AWK_EREXRBRACE,
			E_REXUNBALPAR = ASE_AWK_EREXUNBALPAR,
			E_REXCOLON = ASE_AWK_EREXCOLON,
			E_REXCRANGE = ASE_AWK_EREXCRANGE,
			E_REXCCLASS = ASE_AWK_EREXCCLASS,
			E_REXBRANGE = ASE_AWK_EREXBRANGE,
			E_REXEND = ASE_AWK_EREXEND,
			E_REXGARBAGE = ASE_AWK_EREXGARBAGE
		};
		// end of enum ErrorCode
		
		// generated by genoptcode.awk
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
			OPT_STRBASEONE = ASE_AWK_STRBASEONE,
			OPT_STRIPSPACES = ASE_AWK_STRIPSPACES,
			OPT_NEXTOFILE = ASE_AWK_NEXTOFILE,
			OPT_CRLF = ASE_AWK_CRLF,
			OPT_ARGSTOMAIN = ASE_AWK_ARGSTOMAIN
		};
		// end of enum Option

		class Run
		{
		protected:
			friend class Awk;

			Run (Awk* awk);

		public:
			int stop ();

			ErrorCode getErrorCode () const;
			size_t getErrorLine () const;
			const char_t* getErrorMessage () const;

		protected:
			Awk* awk;
			run_t* run;
			bool callbackFailed;
		};

	
		Awk ();
		virtual ~Awk ();
		
		ErrorCode getErrorCode () const;
		size_t getErrorLine () const ;
		const char_t* getErrorMessage () const;

	protected:
		void setError (ErrorCode code, size_t line = 0, 
			const char_t* arg = ASE_NULL, size_t len = 0);
		void clearError ();
		void retrieveError ();

	public:
		virtual int open ();
		virtual void close ();

		virtual void setOption (int opt);
		virtual int  getOption () const;

		enum Depth
		{
			DEPTH_BLOCK_PARSE = ASE_AWK_DEPTH_BLOCK_PARSE,
			DEPTH_BLOCK_RUN   = ASE_AWK_DEPTH_BLOCK_RUN,
			DEPTH_EXPR_PARSE  = ASE_AWK_DEPTH_EXPR_PARSE,
			DEPTH_EXPR_RUN    = ASE_AWK_DEPTH_EXPR_RUN,
			DEPTH_REX_BUILD   = ASE_AWK_DEPTH_REX_BUILD,
			DEPTH_REX_MATCH   = ASE_AWK_DEPTH_REX_MATCH
		};

		virtual void setDepth (int id, size_t depth);
		virtual int  getDepth (int id);

		virtual int parse ();
		virtual int run (const char_t* main = ASE_NULL, 
		         const char_t** args = ASE_NULL, size_t nargs = 0);

		typedef int (Awk::*FunctionHandler) (
			Return* ret, const Argument* args, size_t nargs);

		virtual int addFunction (
			const char_t* name, size_t minArgs, size_t maxArgs, 
			FunctionHandler handler);
		virtual int deleteFunction (const char_t* main);

		virtual void enableRunCallback ();
		virtual void disableRunCallback ();

	protected:
		virtual int dispatchFunction (
			run_t* run, const char_t* name, size_t len);

		// source code io handlers 
		virtual int     openSource  (Source& io) = 0;
		virtual int     closeSource (Source& io) = 0;
		virtual ssize_t readSource  (Source& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writeSource (Source& io, char_t* buf, size_t len) = 0;

		// pipe io handlers 
		virtual int     openPipe  (Pipe& io) = 0;
		virtual int     closePipe (Pipe& io) = 0;
		virtual ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writePipe (Pipe& io, char_t* buf, size_t len) = 0;
		virtual int     flushPipe (Pipe& io) = 0;

		// file io handlers 
		virtual int     openFile  (File& io) = 0;
		virtual int     closeFile (File& io) = 0;
		virtual ssize_t readFile  (File& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writeFile (File& io, char_t* buf, size_t len) = 0;
		virtual int     flushFile (File& io) = 0;

		// console io handlers 
		virtual int     openConsole  (Console& io) = 0;
		virtual int     closeConsole (Console& io) = 0;
		virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writeConsole (Console& io, char_t* buf, size_t len) = 0;
		virtual int     flushConsole (Console& io) = 0;
		virtual int     nextConsole  (Console& io) = 0;

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

}

#endif
