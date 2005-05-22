/*
 * $Id: stx.h,v 1.19 2005-05-22 13:41:14 bacon Exp $
 */

#ifndef _XP_STX_STX_H_
#define _XP_STX_STX_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef xp_byte_t xp_stx_byte_t;
typedef xp_char_t xp_stx_char_t;
typedef xp_cint_t xp_stx_cint_t;
typedef xp_size_t xp_stx_word_t;

typedef struct xp_stx_objhdr_t xp_stx_objhdr_t;
typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_word_object_t xp_stx_word_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_char_object_t xp_stx_char_object_t;
typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_t xp_stx_t;

#define XP_STX_CHAR(x) XP_CHAR(x)
#define XP_STX_TEXT(x) XP_TEXT(x)

/* common object structure */
struct xp_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	xp_stx_word_t access; 
	xp_stx_word_t class;
};

struct xp_stx_object_t
{
	xp_stx_objhdr_t header;
};

struct xp_stx_word_object_t
{
	xp_stx_objhdr_t header;
	xp_stx_word_t data[1];
};

struct xp_stx_byte_object_t
{
	xp_stx_objhdr_t header;
	xp_stx_byte_t data[1];
};

struct xp_stx_char_object_t
{
	xp_stx_objhdr_t header;
	xp_stx_char_t data[1];
};

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

	xp_stx_word_t symbol_table;
	xp_stx_word_t smalltalk;

	xp_stx_word_t class_symlink;
	xp_stx_word_t class_symbol;
	xp_stx_word_t class_metaclass;
	xp_stx_word_t class_pairlink;

	xp_stx_word_t class_method;
	xp_stx_word_t class_context;

	xp_bool_t __malloced;
	xp_bool_t __wantabort; /* TODO: make it a function pointer */
};

#define XP_STX_NIL   0
#define XP_STX_TRUE  1
#define XP_STX_FALSE 2

#define XP_STX_OBJECT(stx,idx) (((stx)->memory).slots[idx])
#define XP_STX_CLASS(stx,idx)  (XP_STX_OBJECT(stx,(idx))->header.class)
#define XP_STX_ACCESS(stx,idx) (XP_STX_OBJECT(stx,(idx))->header.access)
#define XP_STX_DATA(stx,idx)   ((void*)(XP_STX_OBJECT(stx,idx) + 1))

#define XP_STX_TYPE(stx,idx) (XP_STX_ACCESS(stx,idx) & 0x03)
#define XP_STX_SIZE(stx,idx) (XP_STX_ACCESS(stx,idx) >> 0x02)
#define XP_STX_WORD_INDEXED  (0x00)
#define XP_STX_BYTE_INDEXED  (0x01)
#define XP_STX_CHAR_INDEXED  (0x02)

#define XP_STX_WORD_OBJECT(stx,idx) \
	((xp_stx_word_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_BYTE_OBJECT(stx,idx) \
	((xp_stx_byte_object_t*)XP_STX_OBJECT(stx,idx))
#define XP_STX_CHAR_OBJECT(stx,idx) \
	((xp_stx_char_object_t*)XP_STX_OBJECT(stx,idx))

#define XP_STX_AT(stx,idx,n) \
	(((xp_stx_word_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_BYTEAT(stx,idx,n) \
	(((xp_stx_byte_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])
#define XP_STX_CHARAT(stx,idx,n) \
	(((xp_stx_char_t*)(XP_STX_OBJECT(stx,idx) + 1))[n])

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity);
void xp_stx_close (xp_stx_t* stx);

int xp_stx_bootstrap (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
