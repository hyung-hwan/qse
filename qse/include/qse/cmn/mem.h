/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_MEM_H_
#define _QSE_CMN_MEM_H_

/** @file
 * This file defines functions and macros for memory manipulation.
 */

#include <qse/types.h>
#include <qse/macros.h>

/** 
 * The QSE_MMGR_GETDFL() macro returns the default memory manager.
 */
#define QSE_MMGR_GETDFL()  qse_getdflmmgr()

/**
 * The QSE_MMGR_SETDFL() macro changes the default memory manager.
 */
#define QSE_MMGR_SETDFL(m) qse_setdflmmgr(m)

/**
 * The QSE_MMGR_ALLOC() macro allocates a memory block of the @a size bytes
 * using the @a mmgr memory manager.
 */
#define QSE_MMGR_ALLOC(mmgr,size) ((mmgr)->alloc(mmgr,size))

/**
 * The QSE_MMGR_REALLOC() macro resizes a memory block pointed to by @a ptr 
 * to the @a size bytes using the @a mmgr memory manager.
 */
#define QSE_MMGR_REALLOC(mmgr,ptr,size) ((mmgr)->realloc(mmgr,ptr,size))

/** 
 * The QSE_MMGR_FREE() macro deallocates the memory block pointed to by @a ptr.
 */
#define QSE_MMGR_FREE(mmgr,ptr) ((mmgr)->free(mmgr,ptr))

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_memcpy() functions copies @a n bytes from the source memory block 
 * @a src to the destinaion memory block @a dst. The memory blocks must not 
 * overlap. Use qse_memmove() if they overlap.
 *
 * @return destination memory block @a dst.
 */
QSE_EXPORT void* qse_memcpy (
	void*       dst, /**< destination memory block */
	const void* src, /**< source memory block */
	qse_size_t  n    /**< number of bytes to copy */
);

/**
 * The qse_memmove() functions copies @a n bytes from the source memory block
 * @a src to the destinaion memory block @a dst without corrupting overlapping
 * zone.
 *
 * @return destination memory block @a dst.
 */
QSE_EXPORT void* qse_memmove (
	void*       dst, /**< destination memory block */
	const void* src, /**< source memory block */
	qse_size_t  n    /**< number of bytes to copy */
);

/**
 * The qse_memset() function fills leading @a n bytes of the destination 
 * memory block @a dst with the byte @a val.
 *
 * @return destination memory block @a dst.
 */
QSE_EXPORT void* qse_memset (
	void*       dst, /**< destination memory block */
	int         val, /**< value fill the memory block with */
	qse_size_t  n   /**< number of bytes to fill */
);

/**
 * The qse_memcmp() function compares leading *a n bytes of two memory blocks 
 * @a s1 and @a s2.
 *
 * @return
 * 0 if two memory ares have the same leadning @a n bytes.
 * positive number if the first different byte of s1 is greater than that of s2.
 * negative number if the first different byte of s1 is less than that of s2.
 */
QSE_EXPORT int qse_memcmp (
	const void* s1, /**< first memory block to compare */
	const void* s2, /**< second memory block to compare */ 
	qse_size_t  n   /**< the number of bytes to compare */
);

/**
 * The qse_membyte() function scans the memory block @a s from the first byte
 * up to the nth byte in search of the byte @a val. If it finds a match,
 * it aborts scanning the memory block and returns the pointer to the matching
 * location.
 *
 * @return
 *  #QSE_NULL if the byte @a val is not found.
 *  pointer to the location in the memory block @a s matching the byte @a val
 *  if a match is found.
 */
QSE_EXPORT void* qse_membyte (
	const void* s,     /**< memory block to scan */
	int         val,   /**< byte to find */
	qse_size_t  n      /**< number of bytes to scan */ 
);

/**
 * The qse_memrbyte() function scans the memory block @a s from the nth byte
 * backward to the first byte in search of the byte @a val. If it finds a match,
 * it aborts scanning the memory block and returns the pointer to the matching 
 * location.
 *
 * @return
 *  #QSE_NULL if the byte val is not found.
 *  pointer to the location in the memory block s matching the byte val
 *  if a match is found.
 */
QSE_EXPORT void* qse_memrbyte (
	const void* s,     /**< memory block to scan */
	int         val,   /**< byte to find */
	qse_size_t  n      /**< number of bytes to scan */ 
);

/**
 * The qse_memmem() functions scans the first @a hl bytes of the memory 
 * block @a hs in search of the byte block @a nd of the length @a nl bytes.
 *
 * @return
 *  #QSE_NULL if no match is found.
 *  pointer to the start of the matching location if a match is found.
 */
QSE_EXPORT void* qse_memmem (
	const void* hs,  /**< memory block to scan */
	qse_size_t  hl,  /**< number of bytes to scan */
	const void* nd,  /**< byte block to find */
	qse_size_t  nl   /**< number of bytes in the block */
);

/**
 * The qse_memrmem() functions scans the first @a hl bytes of the memory
 * block @a hs backward in search of the byte block @a nd of the length 
 * @a nl bytes.
 *
 * @return
 * #QSE_NULL if no match is found.
 * pointer to the start of the matching location if a match is found.
 */
QSE_EXPORT void* qse_memrmem (
	const void* hs,  /**< memory block to scan */
	qse_size_t  hl,  /**< number of bytes to scan */
	const void* nd,  /**< byte block to find */
	qse_size_t  nl   /**< number of bytes in the block */
);

/**
 * The qse_getdflmmgr() function returns the default memory manager.
 */
QSE_EXPORT qse_mmgr_t* qse_getdflmmgr (
	void
);

/**
 * The qse_setdflmmgr() function changes the default memory manager.
 * If mmgr is #QSE_NULL, the memory manager is set to the builtin
 * default.
 */
QSE_EXPORT void qse_setdflmmgr (
	qse_mmgr_t* mmgr
);

#if defined(__cplusplus)
}
#endif

#endif
