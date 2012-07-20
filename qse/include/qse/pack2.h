/*
 * $Id: pack2.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
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

#if defined(__GNUC__)
#	pragma pack(2)
#elif defined(__HP_aCC) || defined(__HP_cc)
#	pragma PACK 2
#elif defined(_MSC_VER) || defined(__BORLANDC__)
#	pragma pack(push,2)
#elif defined(__DECC)
#	pragma pack(push,2)
#else
#	pragma pack(2)
#endif
