/*
 * $Id: mem.h 330 2008-08-16 15:29:33Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MEM_H_
#define _ASE_CMN_MEM_H_

#include <ase/types.h>
#include <ase/macros.h>

/* gets the default memory manager */
#define ASE_MMGR_GETDFL()  (ase_mmgr)

/* sets the default memory manager */
#define ASE_MMGR_SETDFL(m) ((ase_mmgr)=(m))

#define ASE_MMGR_ALLOC(mmgr,size) \
	(mmgr)->malloc((mmgr)->custom_data,size)

#define ASE_MMGR_REALLOC(mmgr,ptr,size) \
	(mmgr)->realloc((mmgr)->custom_data,ptr,size)

#define ASE_MMGR_FREE(mmgr,ptr) \
	(mmgr)->free((mmgr)->custom_data,ptr)

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * NAME: point to the default memory manager 
 *
 * DESCRIPTION:
 *  The ASE_MMGR_GETDFL() macro returns the default memory manager.
 *  You may use ASE_MMGR_SETDFL() to change the default memory manager.
 */
extern ase_mmgr_t* ase_mmgr;

/*
 * NAME: copy memory area
 *
 * DESCRIPTION:
 *  The ase_memcpy() functions copies n bytes from the source memory area src 
 *  to the destinaion memory area dst. 
 *
 * RETURNS: the destination memory area dst.
 *
 * WARNING:
 *  The memory areas should not overlap. Use the ase_memmove() function if
 *  they overlap.
 */

void* ase_memcpy (
	void* dst /* a pointer to the destination memory area */ , 
	const void* src /* a pointer to the source memory area */ , 
	ase_size_t n /* the number of bytes to copy */
);

/*
 * NAME: copy memory area with more care 
 *
 * DESCRIPTION:
 *  The ase_memmove() functions copies n bytes from the source memory area src 
 *  to the destinaion memory area dst without corrupting overlapping zone.
 *
 * RETURNS: the destination memory area dst.
 */
void* ase_memmove (
	void* dst /* a pointer to the destination memory area */,
	const void* src /* a pointer to the source memory area */,
	ase_size_t n /* the number of bytes to copy */
);

/*
 * NAME: fill memory area
 *
 * DESCRIPTION:
 *  The ase_memset() function fills leading n bytes of the destination 
 *  memory area dst with the byte val.
 * 
 * RETURNS: the destination memory area dst
 */
void* ase_memset (
	void* dst /* a pointer to the destination memory area */,
	int val /* the byte to fill the memory area with */,
	ase_size_t n /* the number of bytes to fill */
);

/*
 * NAME: compare memory areas
 * 
 * DESCRIPTION:
 *  The ase_memcmp() function compares leading n bytes of two memory areas 
 *  s1 and s2.
 *
 * RETURNS: 
 *  0 if two memory ares have the same leadning n bytes.
 *  a positive number if the first different byte of s1 is greater than that 
 *  of s2.
 *  a negative number if the first different byte of s1 is less than that of s2.
 */
int ase_memcmp (
	const void* s1 /* a pointer to the first memory area to compare */, 
	const void* s2 /* a pointer to the second memory area to compare */, 
	ase_size_t n /* the number of bytes to compare */
);

/*
 * NAME: find a byte forward in memory area
 * 
 * DESCRIPTION:
 *  The ase_membyte() function scans the memory area s from the first byte
 *  up to the nth byte in search of the byte val. If it finds a match,
 *  it aborts scanning the memory area and returns the pointer to the matching
 *  location.
 *
 * RETURNS:
 *  ASE_NULL if the byte val is not found.
 *  The pointer to the location in the memory area s matching the byte val
 *  if a match is found.
 */
void* ase_membyte (
	const void* s /* a pointer to the memory area to scan */,
	int val /* a byte to find */,
	ase_size_t n /* the number of bytes to scan */ 
);

/*
 * NAME: find a byte backward in memory area
 * 
 * DESCRIPTION:
 *  The ase_memrbyte() function scans the memory area s from the nth byte
 *  backward to the first byte in search of the byte val. If it finds a match,
 *  it aborts scanning the memory area and returns the pointer to the matching 
 *  location.
 *
 * RETURNS:
 *  ASE_NULL if the byte val is not found.
 *  The pointer to the location in the memory area s matching the byte val
 *  if a match is found.
 */
void* ase_memrbyte (
	const void* s /* a pointer to the memory area to scan */,
	int val /* a byte to find */,
	ase_size_t n /* the number of bytes to scan */ 
);

/*
 * NAME: find a block of bytes forward in memory area
 * 
 * DESCRIPTION:
 *  The ase_memmem() functions scans the first hl bytes of the memory area hs
 *  in search of the byte block nd of the length nl bytes.
 *
 * RETURNS:
 *  ASE_NULL if the byte val is not found.
 *  The pointer to the location in the memory area s matching the byte val
 *  if a match is found.
 *
 * RETURNS:
 *  ASE_NULL if no match is found.
 *  The pointer to the start of the matching location if a match is found.
 */
void* ase_memmem (
	const void* hs /* a pointer to the memory area to scan */,
	ase_size_t hl /* the number of bytes to scan */,
	const void* nd /* a pointer to the byte block to find */,
	ase_size_t nl /* the number of bytes in the block */
);

/*
 * NAME: find a block of bytes backward in memory area
 *
 * DESCRIPTION:
 *  The ase_memrmem() functions scans the first hl bytes of the memory area hs
 *  backward in search of the byte block nd of the length nl bytes.
 *
 * RETURNS:
 *  ASE_NULL if no match is found.
 *  The pointer to the start of the matching location if a match is found.
 */
void* ase_memrmem (
	const void* hs /* a pointer to the memory area to scan */,
	ase_size_t hl /* the number of bytes to scan */,
	const void* nd /* a pointer to the byte block to find */,
	ase_size_t nl /* the number of bytes in the block */
);

#ifdef __cplusplus
}
#endif

#endif
