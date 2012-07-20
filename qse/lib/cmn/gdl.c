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

#include <qse/cmn/gdl.h>

void qse_gdl_chain (qse_gdl_t* p, qse_gdl_t* x, qse_gdl_t* n)
{
	x->prev = p; x->next = n; n->prev = x; p->next = x;
}

void qse_gdl_unchain (qse_gdl_t* x)
{
	qse_gdl_t* p = x->prev;
	qse_gdl_t* n = x->next;
	n->prev = p; p->next = n;
}
