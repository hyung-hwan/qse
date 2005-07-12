/*
 * $Id: stx.h,v 1.36 2005-07-12 16:16:42 bacon Exp $
 */

#ifndef _XP_STX_STX_H_
#define _XP_STX_STX_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_stx_objhdr_t xp_stx_objhdr_t;
typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_word_object_t xp_stx_word_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_char_object_t xp_stx_char_object_t;
typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_t xp_stx_t;

/* common object structure */
struct xp_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	xp_word_t access; 
	xp_word_t class;
};

struct xp_stx_object_t
{
	xp_stx_objhdr_t header;
};

struct xp_stx_word_object_t
{
	xp_stx_objhdr_t header;
	xp_word_t data[1];
};

struct xp_stx_byte_object_t
{
	xp_stx_objhdr_t header;
	xp_byte_t data[1];
};

struct xp_stx_char_object_t
{
	xp_stx_objhdr_t header;
	xp_char_t data[1];
};

struct xp_stx_memory_t
{
	xp_word_t capacity;
	xp_stx_object_t** slots;
	xp_stx_object_t** free;
	xp_bool_t __malloced;
};

struct xp_stx_t
{
	xp_stx_memory_t memory;

	xp_word_t nil;
	xp_word_t true;
	xp_word_t false;

	xp_word_t symbol_table;
	xp_word_t smalltalk;

	xp_word_t class_symlink;
	xp_word_t class_symbol;
	xp_word_t class_metaclass;
	xp_word_t class_pairlink;

	xp_word_t class_object;
	xp_word_t class_class;
	xp_word_t class_array;
	xp_word_t class_bytearray;
	xp_word_t class_string;
	xp_word_t class_character;
	xp_word_t class_dictionary;
	xp_word_t class_method;
	xp_word_t class_smallinteger;

	xp_bool_t __malloced;
	xp_bool_t __wantabort; /* TODO: make it a function pointer */
};

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

#define XP_STX_IS_OINDEX(x)     (((x) & 0x01) == 0x00)
#define XP_STX_TO_OINDEX(x)     (((x) << 1) | 0x00)
#define XP_STX_FROM_OINDEX(x)   ((x) >> 1)

#define XP_STX_NIL   XP_STX_TO_OINDEX(0)
#define XP_STX_TRUE  XP_STX_TO_OINDEX(1)
#define XP_STX_FALSE XP_STX_TO_OINDEX(2)

#define XP_STX_OBJECT(stx,idx) (((stx)->memory).slots[XP_STX_FROM_OINDEX(idx)])
#define XP_STX_CLASS(stx,idx)  (XP_STX_OBJECT(stx,(idx))->header.class)
#define XP_STX_ACCESS(stx,idx) (XP_STX_OBJECT(stx,(idx))->header.access)
#define XP_STX_DATA(stx,idx)   ((void*)(XP_STX_OBJECT(stx,idx) + 1))

#define XP_STX_TYPE(stx,idx) (XP_STX_ACCESS(stx,idx) & 0x03)
#define XP_STX_SIZE(stx,idx) (XP_STX_ACCESS(stx,idx) >> 0x02)

#define XP_STX_WORD_INDEXED  (0x00)
#define XP_STX_BYTE_INDEXED  (0x01)
#define XP_STX_CHAR_INDEXED  (0x02)

#define XP_STX_IS_WORD_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_WORD_INDEXED)
#define XP_STX_IS_BYTE_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_BYTE_INDEXED)
#define XP_STX_IS_CHAR_OBJECT(stx,idx) \
	(XP_STX_TYPE(stx,idx) == XP_STX_CHAR_INDEXED)

#define XP_STX_WORD_OBJECT(stx,idx) \
	((xp_stx_word_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_BYTE_OBJECT(stx,idx) \
	((xp_stx_byte_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_CHAR_OBJECT(stx,idx) \
	((xp_stx_char_object_t*)XP_STX_OBJECT(stx,idx))

#define XP_STX_WORDAT(stx,idx,n) \
	(((xp_word_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_BYTEAT(stx,idx,n) \
	(((xp_byte_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_CHARAT(stx,idx,n) \
	(((xp_char_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_word_t capacity);
void xp_stx_close (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
