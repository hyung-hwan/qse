/*
 * $Id: unpack.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
#	pragma pack()
#elif defined(__HP_aCC) || defined(__HP_cc)
#	pragma PACK
#elif defined(_MSC_VER) || (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0500))
#	pragma pack(pop)
#elif defined(__DECC)
#	pragma pack(pop)
#else
#	pragma pack()
#endif
