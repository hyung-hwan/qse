/*
 * $Id$
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

#ifndef _QSE_CMN_MBWC_H_
#define _QSE_CMN_MBWC_H_

/** \file
 * This file provides functions and definitions needed for 
 * multibyte/wide-characer conversion.
 */
#include <qse/types.h>
#include <qse/macros.h>

typedef qse_cmgr_t* (*qse_cmgr_finder_t) (const qse_char_t* name);

/** 
 * The qse_cmgr_id_t type defines the builtin-in cmgr IDs.
 */
enum qse_cmgr_id_t
{
	/** The slmb cmgr relies on the locale routnines in the underlying
	 *  platforms. You should initialize locale properly before using this.
	 */
	QSE_CMGR_SLMB,

	/**
	 * The utf cmgr converts between utf8 and unicode characters.
	 */
	QSE_CMGR_UTF8,

	/**
	 * The mb8 cmgr is used to convert raw bytes to wide characters and
	 * vice versa.
	 */
	QSE_CMGR_MB8

#if defined(QSE_ENABLE_XCMGRS)
	,
	QSE_CMGR_CP949, /**< cp949 */
	QSE_CMGR_CP950  /**< cp950 */
#endif
};
typedef enum qse_cmgr_id_t qse_cmgr_id_t;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_findcmgrbyid() function returns a built-in cmgr for a given \a id.
 */
QSE_EXPORT qse_cmgr_t* qse_findcmgrbyid (
	qse_cmgr_id_t id
);

/**
 * The qse_getfindcmgr() function find a built-in cmgr matching a given 
 * \a name and returns it. It returns #QSE_NULL if no match is found.
 * The \a name can be one of "slmb", "utf8", "mb8", "cp949", "cp950", and an 
 * empty string. Calling this function with an empty string is the same
 * as calling qse_getdflcmgr().
 */
QSE_EXPORT qse_cmgr_t* qse_findcmgr (
	const qse_char_t* name
);

QSE_EXPORT void qse_setcmgrfinder (
	qse_cmgr_finder_t finder
);

QSE_EXPORT qse_cmgr_finder_t qse_getcmgrfinder (
	void
);

/* --------------------------------------------------- */
/* DEFAULT GLOBAL CMGR                                 */
/* --------------------------------------------------- */
QSE_EXPORT qse_cmgr_t* qse_getdflcmgr (
	void
);

QSE_EXPORT void qse_setdflcmgr (
	qse_cmgr_t* cmgr
);

/**
 * The qse_setdflcmgrbyid() function finds a built-in
 * cmgr for the \a id and sets it as a default cmgr.
 */
QSE_EXPORT void qse_setdflcmgrbyid (
	qse_cmgr_id_t id
);

/* --------------------------------------------------- */
/* STRING CONVERSION USING CMGR                        */
/* --------------------------------------------------- */

QSE_EXPORT int qse_mbstowcswithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_mbstowcsallwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_mbsntowcsnwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_mbsntowcsnallwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_mbsntowcsnuptowithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_wchar_t        stopper,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsntowcsdupwithcmgr (
     const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsntowcsalldupwithcmgr (
     const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbstowcsdupwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbstowcsalldupwithcmgr (
	const qse_mchar_t* mbs,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsatowcsdupwithcmgr (
	const qse_mchar_t* mbs[],
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsatowcsalldupwithcmgr (
	const qse_mchar_t* mbs[],
	qse_size_t*        wcslen, 
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_wcstombswithcmgr (
	const qse_wchar_t* wcs,    /**< [in] wide-character string to convert*/
	qse_size_t*        wcslen, /**< [out] number of wide-characters handled */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen, /**< [in,out] buffer size for in, 
	                                         actual length  for out*/
	qse_cmgr_t*        cmgr
);

QSE_EXPORT int qse_wcsntombsnwithcmgr (
	const qse_wchar_t* wcs,    /**< [in] wide string */
	qse_size_t*        wcslen, /**< [in,out] wide string length for in,
	                               number of wide characters handled for out */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen, /**< [in,out] buffer size for in,
	                                         actual size for out */
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_mchar_t* qse_wcstombsdupwithcmgr (
	const qse_wchar_t* wcs,
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_mchar_t* qse_wcsntombsdupwithcmgr (
	const qse_wchar_t* wcs,
	qse_size_t         wcslen,
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

QSE_EXPORT qse_mchar_t* qse_wcsatombsdupwithcmgr (
	const qse_wchar_t* wcs[],
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);


QSE_EXPORT qse_mchar_t* qse_wcsnatombsdupwithcmgr (
	const qse_wcstr_t wcs[],
	qse_size_t*       mbslen,
	qse_mmgr_t*       mmgr,
	qse_cmgr_t*       cmgr
);

/* --------------------------------------------------- */
/* STRING CONVERSION WITH DEFAULT GLOBAL CMGR          */
/* --------------------------------------------------- */

/**
 * The qse_mbstowcs() function converts a null-terminated multibyte string to
 * a wide character string.
 *
 * It never returns -2 if \a wcs is #QSE_NULL.
 *
 * \code
 *  const qse_mchar_t* mbs = QSE_MT("a multibyte string");
 *  qse_wchar_t wcs[100];
 *  qse_size_t wcslen = QSE_COUNTOF(buf), n;
 *  qse_size_t mbslen;
 *  int n;
 *  n = qse_mbstowcs (mbs, &mbslen, wcs, &wcslen);
 *  if (n <= -1) { invalid/incomplenete sequence or buffer to small }
 * \endcode
 *
 * \return 0 on success. 
 *         -1 if \a mbs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if \a mbs is not a complete sequence.
 */
QSE_EXPORT int qse_mbstowcs (
	const qse_mchar_t* mbs,    /**< [in] multibyte string to convert */
	qse_size_t*        mbslen, /**< [out] number of multibyte characters
	                                      handled */
	qse_wchar_t*       wcs,    /**< [out] wide-character string buffer */
	qse_size_t*        wcslen  /**< [in,out] buffer size for in, 
	                                number of characters in the buffer for out */
);

/**
 * The qse_mbstowcsall() functions behaves like qse_mbstowcs() except
 * it converts an invalid sequence or an incomplete sequence to a question
 * mark. it never returns -1 or -3.
 */
QSE_EXPORT int qse_mbstowcsall (
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
 * It never returns -2 if \a wcs is #QSE_NULL.
 *
 * \return 0 on success. 
 *         -1 if \a mbs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if \a mbs is not a complete sequence.
 */
QSE_EXPORT int qse_mbsntowcsn (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

QSE_EXPORT int qse_mbsntowcsnall (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

/**
 * The qse_mbsntowcsnupto() function is the same as qse_mbsntowcsn()
 * except that it stops once it has processed the \a stopper character.
 */
QSE_EXPORT int qse_mbsntowcsnupto (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen,
	qse_wchar_t        stopper
);

QSE_EXPORT qse_wchar_t* qse_mbsntowcsdup (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsntowcsalldup (
	const qse_mchar_t* mbs,
	qse_size_t*        mbslen,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_mbstowcsdup (
	const qse_mchar_t* mbs,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_mbstowcsalldup (
	const qse_mchar_t* mbs,
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsatowcsdup (
	const qse_mchar_t* mbs[],
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_mbsatowcsalldup (
	const qse_mchar_t* mbs[],
	qse_size_t*        wcslen,
	qse_mmgr_t*        mmgr
);

/**
 * The qse_wcstombs() function converts a null-terminated wide character 
 * string \a wcs to a multibyte string and writes it into the buffer pointed to
 * by \a mbs, but not more than \a mbslen bytes including the terminating null.
 * 
 * Upon return, \a mbslen is modified to the actual bytes written to \a mbs
 * excluding the terminating null; \a wcslen is modified to the number of
 * wide characters converted.
 *
 * You may pass #QSE_NULL for \a mbs to dry-run conversion or to get the 
 * required buffer size for conversion. -2 is never returned in this case.
 *
 * \return 
 * - 0 on full conversion, 
 * - -1 on no or partial conversion for an illegal character encountered,
 * - -2 on no or partial conversion for a small buffer.
 *
 * \code
 *   const qse_wchar_t* wcs = QSE_T("hello");
 *   qse_mchar_t mbs[10];
 *   qse_size_t wcslen;
 *   qse_size_t mbslen = QSE_COUNTOF(mbs);
 *   n = qse_wcstombs (wcs, &wcslen, mbs, &mbslen);
 *   if (n <= -1)
 *   {
 *      // conversion error
 *   }
 * \endcode
 */
QSE_EXPORT int qse_wcstombs (
	const qse_wchar_t* wcs,    /**< [in] wide-character string to convert*/
	qse_size_t*        wcslen, /**< [out] number of wide-characters handled */
	qse_mchar_t*       mbs,    /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen  /**< [in,out] buffer size for in, 
	                                         actual length  for out*/
);
	
/**
 * The qse_wcsntombsn() function converts the first \a wcslen characters from 
 * a wide character string \a wcs to a multibyte string and writes it to a 
 * buffer \a mbs not more than \a mbslen bytes. 
 *
 * Upon return, it modifies \a mbslen to the actual bytes written to \a mbs
 * and \a wcslen to the number of wide characters converted.
 * 
 * You may pass #QSE_NULL for \a mbs to dry-run conversion or to get the 
 * required buffer size for conversion.
 *
 * 0 is returned on full conversion. The number of wide characters handled
 * is stored into \a wcslen and the number of produced multibyte characters
 * is stored into \a mbslen. -1 is returned if an illegal character is 
 * encounterd during conversion and -2 is returned if the buffer is not 
 * large enough to perform full conversion. however, the number of wide 
 * characters handled so far stored into \a wcslen and the number of produced 
 * multibyte characters so far stored into \a mbslen are still valid.
 * If \a mbs is #QSE_NULL, -2 is never returned.
 * 
 * \return 0 on success, 
 *         -1 if \a wcs contains an illegal character,
 *         -2 if the multibyte string buffer is too small.
 */
QSE_EXPORT int qse_wcsntombsn (
	const qse_wchar_t* wcs,   /**< [in] wide string */
	qse_size_t*        wcslen,/**< [in,out] wide string length for in,
	                               number of wide characters handled for out */
	qse_mchar_t*       mbs,   /**< [out] #QSE_NULL or buffer pointer */
	qse_size_t*        mbslen /**< [in,out] buffer size for in,
	                                        actual size for out */
);

QSE_EXPORT qse_mchar_t* qse_wcstombsdup (
	const qse_wchar_t* wcs,
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_wcsntombsdup (
	const qse_wchar_t* wcs,
	qse_size_t         wcslen,
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_wcsatombsdup (
	const qse_wchar_t* wcs[],
	qse_size_t*        mbslen,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_wcsnatombsdup (
	const qse_wcstr_t wcs[],
	qse_size_t*       mbslen,
	qse_mmgr_t*       mmgr
);

#if defined(__cplusplus)
}
#endif

#endif
