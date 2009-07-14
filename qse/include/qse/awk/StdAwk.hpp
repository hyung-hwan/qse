/*
 * $Id: StdAwk.hpp 230 2009-07-13 08:51:23Z hyunghwan.chung $
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

#ifndef _QSE_AWK_STDAWK_HPP_
#define _QSE_AWK_STDAWK_HPP_

#include <qse/awk/Awk.hpp>

/**
 * @example awk05.cpp
 * This program demonstrates how to embed QSE::StdAwk::loop().
 * @example awk06.cpp
 * This program demonstrates how to use QSE::StdAwk::call().
 * @example awk07.cpp
 * This program demonstrates how to handle an indexed value.
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
////////////////////////////////

/**
 * Provides a more useful AWK interpreter by overriding primitive methods,
 * the file handler, the pipe handler and implementing common AWK intrinsic 
 * functions.
 */
class StdAwk: public Awk
{
public:
	class SourceFile: public Source 
	{
	public:
		SourceFile (const char_t* name): name (name) {}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, char_t* buf, size_t len);

	protected:
		const char_t* name;
	};

	class SourceString: public Source
	{
	public:
		SourceString (const char_t* str): str (str) {}

		int open (Data& io);
		int close (Data& io);
		ssize_t read (Data& io, char_t* buf, size_t len);
		ssize_t write (Data& io, char_t* buf, size_t len);

	protected:
		const char_t* str;
		const char_t* ptr;
	};
        
	int open ();
	void close ();

	virtual int addConsoleOutput (const char_t* arg, size_t len);
	virtual int addConsoleOutput (const char_t* arg);
	virtual void clearConsoleOutputs ();

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
	void* allocMem   (size_t n) throw ();
	void* reallocMem (void* ptr, size_t n) throw ();
	void  freeMem    (void* ptr) throw ();

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


