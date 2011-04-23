/*
 * $Id: StdAwk.hpp 441 2011-04-22 14:28:43Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
		SourceFile (const char_t* name): name (name) 
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

	int addConsoleOutput (const char_t* arg, size_t len);
	int addConsoleOutput (const char_t* arg);
	void clearConsoleOutputs ();

protected:
	// intrinsic functions 
	int sin (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len);
	int cos (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len);
	int tan (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len);
	int atan (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int atan2 (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int log (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int exp (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int sqrt (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int fnint (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int rand (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int srand (Run& run, Value& ret, const Value* args, size_t nargs,
		const char_t* name, size_t len);
	int system (Run& run, Value& ret, const Value* args, size_t nargs,
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

	real_t pow (real_t x, real_t y);
	int    vsprintf (char_t* buf, size_t size,
	                 const char_t* fmt, va_list arg);

protected:
	unsigned int seed; 

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


