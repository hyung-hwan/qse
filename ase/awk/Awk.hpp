/*
 * $Id: Awk.hpp,v 1.13 2007/05/08 15:09:38 bacon Exp $
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
		public:
			Extio (const char_t* name);

			const char_t* getName() const;
			const void* getHandle () const;
			void  setHandle (void* handle);

		private:
			const char_t* name;
			void* handle;
		};

		class Pipe: public Extio
		{
		public:
			enum Mode
			{
				READ = ASE_AWK_EXTIO_PIPE_READ,
				WRITE = ASE_AWK_EXTIO_PIPE_WRITE
			};

			Pipe (char_t* name, Mode mode);

			Mode getMode () const;

		private:
			Mode mode;
		};

		class File: public Extio
		{
		public:
			enum Mode
			{
				READ = ASE_AWK_EXTIO_FILE_READ,
				WRITE = ASE_AWK_EXTIO_FILE_WRITE,
				APPEND = ASE_AWK_EXTIO_FILE_APPEND
			};

			File (char_t* name, Mode mode);

			Mode getMode () const;

		private:
			Mode mode;
		};

		class Console: public Extio
		{
		public:
			enum Mode
			{
				READ = ASE_AWK_EXTIO_CONSOLE_READ,
				WRITE = ASE_AWK_EXTIO_CONSOLE_WRITE
			};

			Console (char_t* name, Mode mode);

			Mode getMode () const;

		private:
			Mode mode;
		};

		class Value
		{
		public:
			Value ();
			Value (long_t l);
			Value (real_t r);
			Value (char_t* ptr, size_t len);
			~Value ();

			void setInt (long_t l);
			void setReal (real_t r);
			void setStr (char_t* ptr, size_t len);

			char_t* toStr (size_t* len);
			long_t  toInt ();
			real_t  toReal ();

		protected:
			int type;

			union
			{
				long_t l;
				real_t r;

				struct
				{
					char_t* ptr;
					size_t len;
				} s;
			} v;
		};

		Awk ();
		virtual ~Awk ();
		
		virtual int open ();
		virtual void close ();

		virtual int parse ();
		virtual int run (const char_t* main = ASE_NULL, 
		         const char_t** args = ASE_NULL);

		typedef Value* (Awk::*FunctionHandler) (size_t nargs, Value** args);

		virtual int addFunction (
			const char_t* name, size_t minArgs, size_t maxArgs, 
			FunctionHandler handler);
		virtual int deleteFunction (const char_t* main);

	protected:

		virtual int dispatchFunction (
			ase_awk_run_t* run, const char_t* name, size_t len);

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
		virtual int     nextPipe  (Pipe& io) = 0;

		// file io handlers 
		virtual int     openFile  (File& io) = 0;
		virtual int     closeFile (File& io) = 0;
		virtual ssize_t readFile  (File& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writeFile (File& io, char_t* buf, size_t len) = 0;
		virtual int     flushFile (File& io) = 0;
		virtual int     nextFile  (File& io) = 0;

		// console io handlers 
		virtual int     openConsole  (Console& io) = 0;
		virtual int     closeConsole (Console& io) = 0;
		virtual ssize_t readConsole  (Console& io, char_t* buf, size_t len) = 0;
		virtual ssize_t writeConsole (Console& io, char_t* buf, size_t len) = 0;
		virtual int     flushConsole (Console& io) = 0;
		virtual int     nextConsole  (Console& io) = 0;

		// run-time callbacks
		/*
		virtual void onStart () {}
		virtual void onReturn () {}
		virtual void onStop () {}
		*/

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
			ase_awk_run_t* run, const char_t* name, size_t len);
		static void freeFunctionMapValue (void* owner, void* value);

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
		ase_awk_t* awk;
		ase_awk_map_t* functionMap;

		Source sourceIn;
		Source sourceOut;
	};

}

#endif
