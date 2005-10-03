/*
 * $Id: sce.h,v 1.3 2005-10-03 04:13:12 bacon Exp $
 */

#ifndef _XP_SCE_SCE_H_
#define _XP_SCE_SCE_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_sce_objhdr_t   xp_sce_objhdr_t;
typedef struct xp_sce_obj_t      xp_sce_obj_t;
typedef struct xp_sce_word_obj_t xp_sce_word_obj_t;
typedef struct xp_sce_byte_obj_t xp_sce_byte_obj_t;
typedef struct xp_sce_char_obj_t xp_sce_char_obj_t;
typedef struct xp_sce_int_obj_t  xp_sce_int_obj_t;
typedef struct xp_sce_real_obj_t xp_sce_real_obj_t;
typedef struct xp_sce_mem_t      xp_sce_mem_t;
typedef struct xp_sce_t xp_sce_t;

/* common obj structure */
struct xp_sce_objhdr_t
{
	/*
	 * lower 4 bytes: atomic types...
	 * CHAR
	 * INT
	 * REAL
	 *
	 * the rest of the bytes: number of elements in the array.
	 * this value will be 1 for atomic types 
	 *
	 * // array types 
	 * ARRAY OF ATTOMIC TYPES 
	 */
	xp_word_t access; 
};

struct xp_sce_obj_t
{
	xp_sce_objhdr_t hdr;
};

struct xp_sce_word_obj_t
{
	xp_sce_objhdr_t hdr;
	xp_word_t data[1];
};

struct xp_sce_byte_obj_t
{
	xp_sce_objhdr_t hdr;
	xp_byte_t data[1];
};

struct xp_sce_char_obj_t
{
	xp_sce_objhdr_t hdr;
	xp_char_t data[1];
};

struct xp_sce_int_obj_t
{
	xp_sce_objhdr_t hdr;
	xp_long_t data[1];
};

struct xp_sce_real_obj_t
{
	xp_sce_objhdr_t hdr;
	xp_real_t data[1];
};

struct xp_sce_mem_t
{
	xp_word_t capacity;
	xp_sce_obj_t** slots;
	xp_sce_obj_t** free;
	xp_bool_t __malloced;
};

struct xp_sce_t
{
	xp_sce_mem_t mem;
	xp_bool_t __malloced;
};

#define XP_SCE_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_SCE_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_SCE_FROM_SMALLINT(x) ((x) >> 1)

#define XP_SCE_IS_OINDEX(x)     (((x) & 0x01) == 0x00)
#define XP_SCE_TO_OINDEX(x)     (((x) << 1) | 0x00)
#define XP_SCE_FROM_OINDEX(x)   ((x) >> 1)

#define XP_SCE_NIL XP_SCE_TO_OINDEX(0)

#define XP_SCE_OBJ(sce,idx)    (((sce)->mem).slots[XP_SCE_FROM_OINDEX(idx)])
#define XP_SCE_ACCESS(sce,idx) (XP_SCE_OBJ(sce,(idx))->hdr.access)
#define XP_SCE_DATA(sce,idx)   ((void*)(XP_SCE_OBJ(sce,idx) + 1))

#define XP_SCE_TYPE(sce,idx) (XP_SCE_ACCESS(sce,idx) & 0x0F)
#define XP_SCE_SIZE(sce,idx) (XP_SCE_ACCESS(sce,idx) >> 0x04)

#define XP_SCE_WORD_INDEXED  (0x00)
#define XP_SCE_BYTE_INDEXED  (0x01)
#define XP_SCE_CHAR_INDEXED  (0x02)
#define XP_SCE_INT_INDEXED   (0x03)
#define XP_SCE_REAL_INDEXED  (0x04)

#define XP_SCE_IS_WORD_OBJ(sce,idx) \
	(XP_SCE_TYPE(sce,idx) == XP_SCE_WORD_INDEXED)
#define XP_SCE_IS_BYTE_OBJ(sce,idx) \
	(XP_SCE_TYPE(sce,idx) == XP_SCE_BYTE_INDEXED)
#define XP_SCE_IS_CHAR_OBJ(sce,idx) \
	(XP_SCE_TYPE(sce,idx) == XP_SCE_CHAR_INDEXED)
#define XP_SCE_IS_INT_OBJ(sce,idx) \
	(XP_SCE_TYPE(sce,idx) == XP_SCE_INT_INDEXED)
#define XP_SCE_IS_REAL_OBJ(sce,idx) \
	(XP_SCE_TYPE(sce,idx) == XP_SCE_REAL_INDEXED)

#define XP_SCE_WORD_OBJ(sce,idx) \
	((xp_sce_word_obj_t*)XP_SCE_OBJ(sce,idx))
#define XP_SCE_BYTE_OBJ(sce,idx) \
	((xp_sce_byte_obj_t*)XP_SCE_OBJ(sce,idx))
#define XP_SCE_CHAR_OBJ(sce,idx) \
	((xp_sce_char_obj_t*)XP_SCE_OBJ(sce,idx))
#define XP_SCE_INT_OBJ(sce,idx) \
	((xp_sce_int_obj_t*)XP_SCE_OBJ(sce,idx))
#define XP_SCE_REAL_OBJ(sce,idx) \
	((xp_sce_real_obj_t*)XP_SCE_OBJ(sce,idx))

#define XP_SCE_WORD_AT(sce,idx,n) (XP_SCE_WORD_OBJ(sce,idx)->data[n])
#define XP_SCE_BYTE_AT(sce,idx,n) (XP_SCE_BYTE_OBJ(sce,idx)->data[n])
#define XP_SCE_CHAR_AT(sce,idx,n) (XP_SCE_CHAR_OBJ(sce,idx)->data[n])
#define XP_SCE_INT_AT(sce,idx,n)  (XP_SCE_INT_OBJ(sce,idx)->data[n])
#define XP_SCE_REAL_AT(sce,idx,n) (XP_SCE_REAL_OBJ(sce,idx)->data[n])

#ifdef __cplusplus
extern "C" {
#endif

xp_sce_t* xp_sce_open (xp_sce_t* sce, xp_word_t capacity);
void xp_sce_close (xp_sce_t* sce);

#ifdef __cplusplus
}
#endif

#endif
