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

#ifndef _QSE_MMGR_HPP_
#define _QSE_MMGR_HPP_

#include <qse/types.h>
#include <qse/macros.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class Mmgr: public qse_mmgr_t
{
public:
	Mmgr () throw ()
	{
		this->alloc = alloc_mem;
		this->realloc = realloc_mem;
		this->free = free_mem;
		this->data = this;
	}

	virtual ~Mmgr () {}

protected:
	virtual void* allocMem (qse_size_t n) throw () = 0;
	virtual void* reallocMem (void* ptr, qse_size_t n) throw () = 0;
	virtual void  freeMem (void* ptr) throw () = 0;

	static void* alloc_mem (void* data, qse_size_t n) throw ()
	{
		return ((Mmgr*)data)->allocMem (n);
	}

	static void* realloc_mem (void* data, void* ptr, qse_size_t n) throw ()
	{
		return ((Mmgr*)data)->reallocMem (ptr, n);
	}

	static void  free_mem (void* data, void* ptr) throw ()
	{
		return ((Mmgr*)data)->freeMem (ptr);
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
