/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
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

#ifndef _QSE_SED_STDSED_HPP_
#define _QSE_SED_STDSED_HPP_

#include <qse/sed/Sed.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The StdSed class inherits the Sed class and implements the standard
 * IO handlers and memory manager for easier use.
 *
 */
class StdSed: public Sed
{
protected:
	void* allocMem   (qse_size_t n)            throw ();
	void* reallocMem (void* ptr, qse_size_t n) throw ();
	void  freeMem    (void* ptr)               throw ();

	int openConsole (Console& io);
	int closeConsole (Console& io);
	ssize_t readConsole (Console& io, char_t* buf, size_t len);
	ssize_t writeConsole (Console& io, const char_t* data, size_t len);

	int openFile (File& io);
	int closeFile (File& io);
	ssize_t readFile (File& io, char_t* buf, size_t len);
	ssize_t writeFile (File& io, const char_t* data, size_t len);
};

/** 
 * @example sed02.cpp 
 * The example shows how to use the QSE::StdSed class to write a simple stream
 * editor that reads from a standard input and writes to a standard output.
 *
 * @example sed03.cpp 
 * The example shows how to extend the QSE::StdSed class to read from and 
 * write to a string.
 */

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
