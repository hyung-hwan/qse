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
