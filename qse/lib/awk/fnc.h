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

#ifndef _QSE_LIB_AWK_FNC_H_
#define _QSE_LIB_AWK_FNC_H_

typedef struct qse_awk_fnc_t qse_awk_fnc_t;
struct qse_awk_fnc_t
{
	struct
	{
		qse_char_t* ptr;
		qse_size_t  len;
	} name;

	int dfl0; /* if set, ($0) is assumed if () is missing. 
	           * this ia mainly for the weird length() function */
	int valid; /* the entry is valid when this option is set */

	struct
	{
		qse_size_t min;
		qse_size_t max;
		qse_char_t* spec;
	} arg;

	qse_awk_fnc_impl_t handler;

	/*qse_awk_fnc_t* next;*/
};

#ifdef __cplusplus
extern "C" {
#endif

qse_awk_fnc_t* qse_awk_getfnc (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
