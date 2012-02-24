/*
 * $Id$
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

#ifndef _QSE_CMN_MBWC_H_
#define _QSE_CMN_MBWC_H_

/** @file
 * This file provides functions and definitions needed for 
 * multibyte/wide-characer conversion.
 */
#include <qse/types.h>
#include <qse/macros.h>

typedef qse_cmgr_t* (*qse_cmgr_finder_t) (const qse_char_t* name);

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------- */
/* BUILTIN CMGR                                        */
/* --------------------------------------------------- */
extern qse_cmgr_t* qse_slmbcmgr;
extern qse_cmgr_t* qse_utf8cmgr;
extern qse_cmgr_t* qse_cp949cmgr;
extern qse_cmgr_t* qse_cp950cmgr;

/**
 * The qse_getfindcmgr() function find a builtin cmgr matching a given 
 * @a name and returns it. It returns #QSE_NULL if no match is found.
 * The @a name can be one of "utf8", "slmb", and an empty string. Calling this
 * function with an empty string is the same as calling qse_getdflcmgr().
 */
qse_cmgr_t* qse_findcmgr (
	const qse_char_t* name
);

void qse_setcmgrfinder (
	qse_cmgr_finder_t finder
);

qse_cmgr_finder_t qse_getcmgrfinder (
	void
);

/* --------------------------------------------------- */
/* DEFAULT GLOBAL CMGR                                 */
/* --------------------------------------------------- */
qse_cmgr_t* qse_getdflcmgr (
	void
);

void qse_setdflcmgr (
	qse_cmgr_t* cmgr
);

/* --------------------------------------------------- */
/* STRING CONVERSION USING CMGR                        */
/* --------------------------------------------------- */

int qse_mbstowcswithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

int qse_mbstowcsallwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

int qse_mbsntowcsnwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

int qse_mbsntowcsnallwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

int qse_mbsntowcsnuptowithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_wchar_t        stopper,
	qse_cmgr_t*        cmgr
);

qse_wchar_t* qse_mbstowcsdupwithcmgr (
	const qse_mchar_t* mbs,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

qse_wchar_t* qse_mbstowcsalldupwithcmgr (
	const qse_mchar_t* mbs,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

qse_wchar_t* qse_mbsatowcsdupwithcmgr (
	const qse_mchar_t* mbs[],
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

qse_wchar_t* qse_mbsatowcsalldupwithcmgr (
	const qse_mchar_t* mbs[],
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

int qse_wcstombswithcmgr (
	const qse_wchar_t* wcs,    /**< [in] wide-character string to convert*/
	qse_size_t*        wcslen, /**< [out] number of wide-characters handled */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen, /**< [in,out] buffer size for in, 
	                                         actual length  for out*/
	qse_cmgr_t*        cmgr
);

int qse_wcsntombsnwithcmgr (
	const qse_wchar_t* wcs,    /**< [in] wide string */
	qse_size_t*        wcslen, /**< [in,out] wide string length for in,
	                               number of wide characters handled for out */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen, /**< [in,out] buffer size for in,
	                                         actual size for out */
	qse_cmgr_t*        cmgr
);

qse_mchar_t* qse_wcstombsdupwithcmgr (
	const qse_wchar_t* wcs,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

qse_mchar_t* qse_wcsatombsdupwithcmgr (
	const qse_wchar_t* wcs[],
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);


/* --------------------------------------------------- */
/* STRING CONVERSION WITH DEFAULT GLOBAL CMGR          */
/* --------------------------------------------------- */

/**
 * The qse_mbstowcs() function converts a null-terminated multibyte string to
 * a wide character string.
 *
 * It never returns -2 if @a wcs is #QSE_NULL.
 *
 * @code
 *  const qse_mchar_t* mbs = QSE_MT("a multibyte string");
 *  qse_wchar_t wcs[100];
 *  qse_size_t wcslen = QSE_COUNTOF(buf), n;
 *  qse_size_t mbslen;
 *  int n;
 *  n = qse_mbstowcs (mbs, &mbslen, wcs, &wcslen);
 *  if (n <= -1) { invalid/incomplenete sequence or buffer to small }
 * @endcode
 *
 * @return 0 on success. 
 *         -1 if @a mbs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if @a mbs is not a complete sequence.
 */
int qse_mbstowcs (
	const qse_mchar_t* mbs,    /**< [in] multibyte string to convert */
	qse_size_t*        mbslen, /**< [out] number of multibyte characters
	                                      handled */
	qse_wchar_t*       wcs,    /**< [out] wide-character string buffer */
	qse_size_t*        wcslen  /**< [in,out] buffer size for in, 
	                                number of characters in the buffer for out */
);

int qse_mbstowcsall (
	const qse_mchar_t* mbs,    /**< [in] multibyte string to convert */
	qse_size_t*        mbslen, /**< [out] number of multibyte characters
	                                      handled */
	qse_wchar_t*       wcs,    /**< [out] wide-character string buffer */
	qse_size_t*        wcslen  /**< [in,out] buffer size for in, 
	                                number of characters in the buffer for out */
);

/**
 * The qse_mbsntowcsn() function converts a multibyte string to a 
 * wide character string.
 *
 * It never returns -2 if @a wcs is #QSE_NULL.
 *
 * @return 0 on success. 
 *         -1 if @a mbs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if @a mbs is not a complete sequence.
 */
int qse_mbsntowcsn (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

int qse_mbsntowcsnall (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

/**
 * The qse_mbsntowcsnupto() function is the same as qse_mbsntowcsn()
 * except that it stops once it has processed the @a stopper character.
 */
int qse_mbsntowcsnupto (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_wchar_t        stopper
);

qse_wchar_t* qse_mbstowcsdup (
	const qse_mchar_t* mbs,
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_mbstowcsalldup (
	const qse_mchar_t* mbs,
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_mbsatowcsdup (
	const qse_mchar_t* mbs[],
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_mbsatowcsalldup (
	const qse_mchar_t* mbs[],
	qse_mmgr_t*        mmgr
);

/**
 * The qse_wcstombs() function converts a null-terminated wide character 
 * string @a wcs to a multibyte string and writes it into the buffer pointed to
 * by @a mbs, but not more than @a mbslen bytes including the terminating null.
 * 
 * Upon return, @a mbslen is modifed to the actual bytes written to @a mbs
 * excluding the terminating null; @a wcslen is modifed to the number of
 * wide characters converted.
 *
 * You may pass #QSE_NULL for @a mbs to dry-run conversion or to get the 
 * required buffer size for conversion. -2 is never returned in this case.
 *
 * @return 
 * - 0 on full conversion, 
 * - -1 on no or partial conversion for an illegal character encountered,
 * - -2 on no or partial conversion for a small buffer.
 *
 * @code
 *   const qse_wchar_t* wcs = QSE_T("hello");
 *   qse_mchar_t mbs[10];
 *   qse_size_t wcslen;
 *   qse_size_t mbslen = QSE_COUNTOF(mbs);
 *   n = qse_wcstombs (wcs, &wcslen, mbs, &mbslen);
 *   if (n <= -1)
 *   {
 *       // wcs fully scanned and mbs null-terminated
 *   }
 * @endcode
 */
int qse_wcstombs (
	const qse_wchar_t* wcs,    /**< [in] wide-character string to convert*/
	qse_size_t*        wcslen, /**< [out] number of wide-characters handled */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen  /**< [in,out] buffer size for in, 
	                                         actual length  for out*/
);
	
/**
 * The qse_wcsntombsn() function converts the first @a wcslen characters from 
 * a wide character string @a wcs to a multibyte string and writes it to a 
 * buffer @a mbs not more than @a mbslen bytes. 
 *
 * Upon return, it modifies @a mbslen to the actual bytes written to @a mbs
 * and @a wcslen to the number of wide characters converted.
 * 
 * You may pass #QSE_NULL for @a mbs to dry-run conversion or to get the 
 * required buffer size for conversion.
 *
 * 0 is returned on full conversion. The number of wide characters handled
 * is stored into @a wcslen and the number of produced multibyte characters
 * is stored into @a mbslen. -1 is returned if an illegal character is 
 * encounterd during conversion and -2 is returned if the buffer is not 
 * large enough to perform full conversion. however, the number of wide 
 * characters handled so far stored into @a wcslen and the number of produced 
 * multibyte characters so far stored into @a mbslen are still valid.
 * If @a mbs is #QSE_NULL, -2 is never returned.
 * 
 * @return 0 on success, 
 *         -1 if @a wcs contains an illegal character,
 *         -2 if the multibyte string buffer is too small.
 */
int qse_wcsntombsn (
	const qse_wchar_t* wcs,   /**< [in] wide string */
	qse_size_t*        wcslen,/**< [in,out] wide string length for in,
	                               number of wide characters handled for out */
	qse_mchar_t*       mbs,   /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen /**< [in,out] buffer size for in,
	                                        actual size for out */
);

qse_mchar_t* qse_wcstombsdup (
	const qse_wchar_t* wcs,
	qse_mmgr_t*        mmgr
);

qse_mchar_t* qse_wcsatombsdup (
	const qse_wchar_t* wcs[],
	qse_mmgr_t*        mmgr
);

#ifdef __cplusplus
}
#endif

#endif
