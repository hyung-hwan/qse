/*
 * $Id: Awk.hpp,v 1.28 2007/05/16 07:13:32 bacon Exp $
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

			run_t* getRun () const;
			awk_t* getAwk () const;

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

			run_t* getRun () const;
			awk_t* getAwk () const;

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
			run_t* getRun () const;
			awk_t* getAwk () const;

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

		class Run
		{
		protected:
			friend class Awk;
			Run (run_t* run);

		protected:
			run_t* run;
		};

		Awk ();
		virtual ~Awk ();
		
		virtual int open ();
		virtual void close ();

		virtual int parse ();
		virtual int run (const char_t* main = ASE_NULL, 
		         const char_t** args = ASE_NULL, size_t nargs = 0);

		typedef int (Awk::*FunctionHandler) (
			Return* ret, const Argument* args, size_t nargs);

		virtual int addFunction (
			const char_t* name, size_t minArgs, size_t maxArgs, 
			FunctionHandler handler);
		virtual int deleteFunction (const char_t* main);

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
		virtual void onRunEnd (const Run& run, int errnum);

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

	private:
		Awk (const Awk&);
		Awk& operator= (const Awk&);
	};

}

#endif
