/*
 * $Id: Awk.hpp,v 1.5 2007/05/04 10:25:14 bacon Exp $
 */

#ifndef _ASE_AWK_AWK_HPP_
#define _ASE_AWK_AWK_HPP_

#include <ase/awk/awk.h>
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
		typedef ase_real_t  real_t;

		Awk ();
		virtual ~Awk ();
		
		int parse ();
		int run (const char_t* main, const char_t** args);
		void close ();

		enum SourceMode
		{
			SOURCE_READ,
			SOURCE_WRITE
		};

	protected:
		int open ();

		virtual int openSource (SourceMode mode) = 0;
		virtual int closeSource (SourceMode mode) = 0;
		virtual ssize_t readSource (char_t* buf, size_t len) = 0;
		virtual ssize_t writeSource (char_t* buf, size_t len) = 0;

		virtual void* malloc    (size_t n) = 0;
		virtual void* realloc   (void* ptr, size_t n) = 0;
		virtual void  free      (void* ptr) = 0;

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

		static ssize_t sourceReader (
        		int cmd, void* arg, char_t* data, size_t count);
		static ssize_t sourceWriter (
        		int cmd, void* arg, char_t* data, size_t count);

		static void* malloc  (void* custom, size_t n);
		static void* realloc (void* custom, void* ptr, size_t n);
		static void  free    (void* custom, void* ptr);

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

	private:
		ase_awk_t* awk;
	};

}

#endif
