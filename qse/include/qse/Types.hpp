/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
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

#ifndef _QSE_TYPES_HPP_
#define _QSE_TYPES_HPP_

#include <qse/types.h>
#include <qse/macros.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Types class defines handy aliases for various QSE types.
 */
class Types
{
public:
	/** boolean data type */
	typedef qse_bool_t  bool_t;
	/** data type that can hold any character */
	typedef qse_char_t  char_t;
	/** data type that can hold any character or an end-of-file value */
	typedef qse_cint_t  cint_t;
	/** redefines an unsigned integer number of the same size as void* */
	typedef qse_size_t  size_t;
	/** signed version of size_t */
	typedef qse_ssize_t ssize_t;
	/** redefines an integer */
	typedef qse_long_t  long_t;
	/** redefines a floating-point number */
	typedef qse_real_t  real_t;
	/** redefines a structure of a constant character pointer and length */
	typedef qse_cstr_t cstr_t;
	/** redefines a structure of a character pointer and length */
	typedef qse_xstr_t xstr_t;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
