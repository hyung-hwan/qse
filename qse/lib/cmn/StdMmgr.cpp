/*
 * $Id$
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

#include <qse/cmn/StdMmgr.hpp>
#include <stdlib.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

StdMmgr StdMmgr::DFL;

void* StdMmgr::allocMem (size_t n) 
{
	return ::malloc (n); 
}

void* StdMmgr::reallocMem (void* ptr, size_t n) 
{ 
	return ::realloc (ptr, n); 
}

void StdMmgr::freeMem (void* ptr) 
{ 
	return ::free (ptr); 
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
