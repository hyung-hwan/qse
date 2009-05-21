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

#include <qse/sed/StdSed.hpp>
#include <stdlib.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

void* StdSed::allocMem (qse_size_t n) throw ()
{ 
	return ::malloc (n); 
}

void* StdSed::reallocMem (void* ptr, qse_size_t n) throw ()
{ 
	return ::realloc (ptr, n); 
}

void StdSed::freeMem (void* ptr) throw ()
{ 
	::free (ptr); 
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

