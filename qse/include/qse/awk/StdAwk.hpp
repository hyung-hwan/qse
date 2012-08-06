/*
 * $Id: StdAwk.hpp 516 2011-07-23 09:03:48Z hyunghwan.chung $
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_AWK_STDAWK_HPP_
#define _QSE_AWK_STDAWK_HPP_

#include <qse/awk/Awk.hpp>
#include <qse/cmn/StdMmgr.hpp>

/// @file
/// Standard AWK Interpreter
///
/// @example awk05.cpp
/// This program demonstrates how to use QSE::StdAwk::loop().
///
/// @example awk06.cpp
/// This program demonstrates how to use QSE::StdAwk::call().
///
/// @example awk07.cpp
/// This program demonstrates how to handle an indexed value.
///
/// @example awk08.cpp
/// This program shows how to add intrinsic functions.
///
/// @example awk12.cpp
/// This program shows how to override console methods to use 
/// string buffers for console input and output.
///
/// @example awk13.cpp
/// This program shows how to use resetRunContext(). It is similar
/// to awk12.cpp in principle.
///
/// @example awk14.cpp
/// This program shows how to define a console handler to use
/// string buffers for console input and output. It is identical
/// to awk13.cpp except that it relies an external console handler
/// rather than overriding console methods.
///

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
////////////////////////////////

///
/// The StdAwk class provides an easier-to-use interface by overriding 
/// primitive methods, and implementing the file handler, the pipe handler, 
/// and common intrinsic functions.
///
class StdAwk: public Awk
{
public:
	///
	/// The SourceFile class implements script I/O from and to a file.
	///
	class SourceFile: public Source 
	{
	public:
		SourceFile (const char_t* name, qse_cmgr_t* cmgr = QSE_NULL): 
			name (name), cmgr (cmgr)
		{
			dir.ptr = QSE_NULL; dir.len = 0; 
		}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* name;
		qse_cstr_t dir;
		qse_cmgr_t* cmgr;
	};

	///
	/// The SourceString class implements script input from a string. 
	/// Deparsing is not supported.
	///
	class SourceString: public Source
	{
	public:
		SourceString (const char_t* str): str (str) {}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, const char_t* buf, size_t len);

	protected:
		const char_t* str;
		const char_t* ptr;
	};
        
	StdAwk (Mmgr* mmgr = &StdMmgr::DFL): Awk (mmgr) {}

	int open ();
	void close ();
	Run* parse (Source& in, Source& out);

	int addConsoleOutput (const char_t* arg, size_t len);
	int addConsoleOutput (const char_t* arg);
	void clearConsoleOutputs ();

protected:
	// intrinsic functions 
	int rand (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int srand (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int system (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int time (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);

	qse_cmgr_t* getcmgr (const char_t* ioname);
	int setioattr (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int getioattr (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);

	// pipe io handlers 
	int openPipe (Pipe& io);
	int closePipe (Pipe& io);
	ssize_t readPipe  (Pipe& io, char_t* buf, size_t len);
	ssize_t writePipe (Pipe& io, const char_t* buf, size_t len);
	int flushPipe (Pipe& io);

	// file io handlers 
	int openFile (File& io);
	int closeFile (File& io);
	ssize_t readFile (File& io, char_t* buf, size_t len);
	ssize_t writeFile (File& io, const char_t* buf, size_t len);
	int flushFile (File& io);

	// console io handlers 
	int openConsole (Console& io);
	int closeConsole (Console& io);
	ssize_t readConsole (Console& io, char_t* buf, size_t len);
	ssize_t writeConsole (Console& io, const char_t* buf, size_t len);
	int flushConsole (Console& io);
	int nextConsole (Console& io);

	// primitive handlers 
	void* allocMem   (size_t n);
	void* reallocMem (void* ptr, size_t n);
	void  freeMem    (void* ptr);


	int vsprintf (char_t* buf, size_t size,
	              const char_t* fmt, va_list arg);

	flt_t pow (flt_t x, flt_t y);
	flt_t mod (flt_t x, flt_t y);
	flt_t sin (flt_t x);
	flt_t cos (flt_t x);
	flt_t tan (flt_t x);
	flt_t atan (flt_t x);
	flt_t atan2 (flt_t x, flt_t y);
	flt_t log (flt_t x);
	flt_t log10 (flt_t x);
	flt_t exp (flt_t x);
	flt_t sqrt (flt_t x);

protected:
	unsigned int seed; 
	qse_htb_t cmgrtab;
	bool cmgrtab_inited;

	/* global variables */
	int gbl_argc;
	int gbl_argv;
	int gbl_environ;
	int gbl_procinfo;

	/* standard input console - reuse runarg */
	size_t runarg_index;
	size_t runarg_count;

	/* standard output console */
	xstrs_t ofile;
	size_t ofile_index;
	size_t ofile_count;

private:
	int open_console_in (Console& io);
	int open_console_out (Console& io);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


