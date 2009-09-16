/*
 * $Id: StdSed.hpp 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifndef _QSE_SED_STDSED_HPP_
#define _QSE_SED_STDSED_HPP_

#include <qse/sed/Sed.hpp>

/** @file
 * Standard Stream Editor
 */

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The StdSed class inherits the Sed class and implements the standard
 * I/O handlers and memory manager for easier use.
 *
 */
class StdSed: public Sed
{
protected:
	void* allocMem   (qse_size_t n);
	void* reallocMem (void* ptr, qse_size_t n);
	void  freeMem    (void* ptr);

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
 */

/**
 * @example sed03.cpp 
 * The example shows how to extend the QSE::StdSed class to read from and 
 * write to a string.
 */

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
