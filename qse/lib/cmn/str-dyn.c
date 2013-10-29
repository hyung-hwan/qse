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

#include <qse/cmn/str.h>
#include "mem.h"

#undef char_t
#undef xstr_t
#undef str_sizer_t
#undef T
#undef strlen
#undef strncpy
#undef strxpac
#undef strxtrm
#undef str_t
#undef str_open
#undef str_close
#undef str_init
#undef str_fini
#undef str_getmmgr
#undef str_getxtn
#undef str_yield
#undef str_yieldptr
#undef str_getsizer
#undef str_setsizer
#undef str_getcapa
#undef str_setcapa
#undef str_getlen
#undef str_setlen
#undef str_clear
#undef str_swap
#undef str_cpy
#undef str_ncpy
#undef str_cat
#undef resize_for_ncat
#undef str_ncat
#undef str_nrcat
#undef str_ccat
#undef str_nccat
#undef str_del
#undef str_trm
#undef str_pac

#define char_t qse_mchar_t
#define xstr_t qse_mxstr_t
#define str_sizer_t qse_mbs_sizer_t
#define T(x) QSE_MT(x)
#define strlen(x) qse_mbslen(x)
#define strncpy(x,y,z) qse_mbsncpy(x,y,z)
#define strxpac(x,y) qse_mbsxpac(x,y)
#define strxtrm(x,y) qse_mbsxtrm(x,y)
#define str_t qse_mbs_t
#define str_open qse_mbs_open 
#define str_close qse_mbs_close 
#define str_init qse_mbs_init 
#define str_fini qse_mbs_fini 
#define str_getmmgr qse_mbs_getmmgr 
#define str_getxtn qse_mbs_getxtn 
#define str_yield qse_mbs_yield 
#define str_yieldptr qse_mbs_yieldptr 
#define str_getsizer qse_mbs_getsizer 
#define str_setsizer qse_mbs_setsizer 
#define str_getcapa qse_mbs_getcapa 
#define str_setcapa qse_mbs_setcapa 
#define str_getlen qse_mbs_getlen 
#define str_setlen qse_mbs_setlen 
#define str_clear qse_mbs_clear 
#define str_swap qse_mbs_swap 
#define str_cpy qse_mbs_cpy 
#define str_ncpy qse_mbs_ncpy 
#define str_cat qse_mbs_cat 
#define resize_for_ncat resize_for_mbs_ncat 
#define str_ncat qse_mbs_ncat 
#define str_nrcat qse_mbs_nrcat 
#define str_ccat qse_mbs_ccat 
#define str_nccat qse_mbs_nccat 
#define str_del qse_mbs_del 
#define str_trm qse_mbs_trm 
#define str_pac qse_mbs_pac 
#include "str-dyn.h"

/* -------------------------------------------------------- */

#undef char_t
#undef xstr_t
#undef str_sizer_t
#undef T
#undef strlen
#undef strncpy
#undef strxpac
#undef strxtrm
#undef str_t
#undef str_open
#undef str_close
#undef str_init
#undef str_fini
#undef str_getmmgr
#undef str_getxtn
#undef str_yield
#undef str_yieldptr
#undef str_getsizer
#undef str_setsizer
#undef str_getcapa
#undef str_setcapa
#undef str_getlen
#undef str_setlen
#undef str_clear
#undef str_swap
#undef str_cpy
#undef str_ncpy
#undef str_cat
#undef resize_for_ncat
#undef str_ncat
#undef str_nrcat
#undef str_ccat
#undef str_nccat
#undef str_del
#undef str_trm
#undef str_pac

#define char_t qse_wchar_t
#define xstr_t qse_wxstr_t
#define str_sizer_t qse_wcs_sizer_t
#define T(x) QSE_WT(x)
#define strlen(x) qse_wcslen(x)
#define strncpy(x,y,z) qse_wcsncpy(x,y,z)
#define strxpac(x,y) qse_wcsxpac(x,y)
#define strxtrm(x,y) qse_wcsxtrm(x,y)
#define str_t qse_wcs_t
#define str_open qse_wcs_open 
#define str_close qse_wcs_close 
#define str_init qse_wcs_init 
#define str_fini qse_wcs_fini 
#define str_getmmgr qse_wcs_getmmgr 
#define str_getxtn qse_wcs_getxtn 
#define str_yield qse_wcs_yield 
#define str_yieldptr qse_wcs_yieldptr 
#define str_getsizer qse_wcs_getsizer 
#define str_setsizer qse_wcs_setsizer 
#define str_getcapa qse_wcs_getcapa 
#define str_setcapa qse_wcs_setcapa 
#define str_getlen qse_wcs_getlen 
#define str_setlen qse_wcs_setlen 
#define str_clear qse_wcs_clear 
#define str_swap qse_wcs_swap 
#define str_cpy qse_wcs_cpy 
#define str_ncpy qse_wcs_ncpy 
#define str_cat qse_wcs_cat 
#define resize_for_ncat resize_for_wcs_ncat 
#define str_ncat qse_wcs_ncat 
#define str_nrcat qse_wcs_nrcat 
#define str_ccat qse_wcs_ccat 
#define str_nccat qse_wcs_nccat 
#define str_del qse_wcs_del 
#define str_trm qse_wcs_trm 
#define str_pac qse_wcs_pac 
#include "str-dyn.h"
