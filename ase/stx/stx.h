/*
 * $Id: stx.h,v 1.2 2005-05-08 10:31:24 bacon Exp $
 */

#ifndef _XP_STX_STX_H_
#define _XP_STX_STX_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef xp_byte_t xp_stx_byte_t;
typedef xp_char_t xp_stx_char_t;
typedef xp_size_t xp_stx_word_t;
typedef xp_size_t xp_stx_size_t;
typedef xp_size_t xp_stx_index_t;

typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_string_object_t xp_stx_string_object_t;
typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_t xp_stx_t;

#define XP_STX_CHAR(x) XP_CHAR(x)
#define XP_STX_TEXT(x) XP_TEXT(x)

/* common object header structure */

#define XP_STX_OBJECT_HEADER_SIZE \
	(xp_sizeof(xp_stx_object_t) - xp_sizeof(xp_stx_word_t))

struct xp_stx_object_t
{
	/* access - mode: 2; size: rest;
	 * mode - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	xp_stx_word_t access; 
	xp_stx_word_t class;
	/*xp_stx_word_t didxa[1];*/
};

/*
struct xp_stx_byte_object_t
{
	xp_stx_word_t access; 
	xp_stx_word_t class;
	xp_stx_byte_t didxa[1];
};

struct xp_stx_string_object_t
{
	xp_stx_word_t access; 
	xp_stx_word_t class;
	xp_stx_char_t didxa[1];
};
*/

struct xp_stx_memory_t
{
	xp_stx_word_t capacity;
	xp_stx_object_t** slots;
	xp_stx_object_t** free;
	xp_bool_t __malloced;
};

struct xp_stx_t
{
	xp_stx_memory_t memory;

	xp_stx_word_t nil;
	xp_stx_word_t true;
	xp_stx_word_t false;

	xp_bool_t __malloced;
};

#define XP_STX_NIL   0
#define XP_STX_TRUE  1
#define XP_STX_FALSE 2

/*
#define XP_STX_OBJECT(mem,idx) \
	((xp_stx_object_t*)((mem)->slots[idx]))
#define XP_STX_BYTE_OBJECT(mem,idx) \
	((xp_stx_byte_object_t*)((mem)->slots[idx]))
#define XP_STX_STRING_OBJECT(mem,idx) \
	((xp_stx_string_object_t*)((mem)->slots[idx]))
*/

#define XP_STX_OBJECT(mem,idx) ((mem)->slots[idx])
#define XP_STX_OBJECT_ACCESS(mem,idx) (XP_STX_OBJECT(mem,(idx))->access)
#define XP_STX_OBJECT_CLASS(mem,idx) (XP_STX_OBJECT(mem,(idx))->class)
/*
#define XP_STX_OBJECT_DATA(mem,idx) \
	(((XP_STX_OBJECT_ACCESS(mem,idx) & 0x03) == 0x00)? \
		(XP_STX_OBJECT(mem,idx)).didxa): \
	 (((XP_STX_OBJECT_ACCESS(mem,idx) & 0x03) == 0x01)? \
		(XP_STX_BYTE_OBJECT(mem,idx)).didxa): \
		(XP_STX_STRING_OBJECT(mem,idx)).didxa))
*/

#define XP_STX_OBJECT_AT(mem,idx,n) \
	(((xp_stx_word_t*)(XP_STX_OBJECT(mem,idx) + 1))[n])
#define XP_STX_OBJECT_BYTEAT(mem,idx,n) \
	(((xp_stx_byte_t*)(XP_STX_OBJECT(mem,idx) + 1))[n])
#define XP_STX_OBJECT_CHARAT(mem,idx,n) \
	(((xp_stx_char_t*)(XP_STX_OBJECT(mem,idx) + 1))[n])

/*
#define XP_STX_OBJECT_DATA(mem,idx) \
	(((XP_STX_OBJECT_ACCESS(mem,idx) & 0x03) == 0x00)? \
		(((xp_stx_word_t*)XP_STX_OBJECT(mem,idx)) + 1): \
	 (((XP_STX_OBJECT_ACCESS(mem,idx) & 0x03) == 0x01)? \
		(((xp_stx_byte_t*)XP_STX_OBJECT(mem,idx)) + 1): \
		(((xp_stx_char_t*)XP_STX_OBJECT(mem,idx)) + 1)))

*/



#ifdef __cplusplus
extern "C" {
#endif

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity);
void xp_stx_close (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
