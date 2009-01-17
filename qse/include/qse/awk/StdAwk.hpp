/*
 * $Id: StdAwk.hpp 499 2008-12-16 09:42:48Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * Provides a more useful AWK interpreter by overriding primitive methods,
 * the file handler, the pipe handler and implementing common AWK intrinsic 
 * functions.
 */
class StdAwk: public Awk
{
public:
	StdAwk ();
	int open ();
	int run (const char_t* main, const char_t** args, size_t nargs);

protected:

	// intrinsic functions 
	int sin (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int cos (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int tan (Run& run, Return& ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int atan (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int atan2 (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int log (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int exp (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int sqrt (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int fnint (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int rand (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int srand (Run& run, Return& ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int system (Run& run, Return& ret, const Argument* args, size_t nargs,
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

	// primitive handlers 
	void* allocMem   (size_t n);
	void* reallocMem (void* ptr, size_t n);
	void  freeMem    (void* ptr);

	bool_t isType    (cint_t c, ccls_type_t type);
	cint_t transCase (cint_t c, ccls_type_t type);

	real_t pow (real_t x, real_t y);
	int    vsprintf (char_t* buf, size_t size,
	                 const char_t* fmt, va_list arg);

protected:
	unsigned int seed; 
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


