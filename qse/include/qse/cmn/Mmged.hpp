/*
 * $Id$
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

#ifndef _QSE_CMN_MMGED_HPP_
#define _QSE_CMN_MMGED_HPP_

#include <qse/Types.hpp>
#include <qse/cmn/Mmgr.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The Mmged class defines a memory manager interface to be inherited by
/// a subclass that uses a memory manager.
///

class QSE_EXPORT Mmged: public Types
{
public:
	Mmged (Mmgr* mmgr): mmgr (mmgr) {}

	///
	/// The getMmgr() function returns the memory manager associated.
	///
	Mmgr* getMmgr () const { return mmgr; }

protected:
	Mmgr* mmgr;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
