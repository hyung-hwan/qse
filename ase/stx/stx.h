/*
 * $Id: stx.h,v 1.46 2007-03-22 11:19:28 bacon Exp $
 */

#ifndef _ASE_STX_STX_H_
#define _ASE_STX_STX_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_stx_objhdr_t ase_stx_objhdr_t;
typedef struct ase_stx_object_t ase_stx_object_t;
typedef struct ase_stx_word_object_t ase_stx_word_object_t;
typedef struct ase_stx_byte_object_t ase_stx_byte_object_t;
typedef struct ase_stx_char_object_t ase_stx_char_object_t;
typedef struct ase_stx_memory_t ase_stx_memory_t;
typedef struct ase_stx_symtab_t ase_stx_symtab_t;
typedef struct ase_stx_t ase_stx_t;

/* common object structure */
struct ase_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	ase_word_t access; 
	ase_word_t class;
};

struct ase_stx_object_t
{
	ase_stx_objhdr_t header;
};

struct ase_stx_word_object_t
{
	ase_stx_objhdr_t header;
	ase_word_t data[1];
};

struct ase_stx_byte_object_t
{
	ase_stx_objhdr_t header;
	ase_byte_t data[1];
};

struct ase_stx_char_object_t
{
	ase_stx_objhdr_t header;
	ase_char_t data[1];
};


struct ase_stx_memory_t
{
	ase_word_t capacity;
	ase_stx_object_t** slots;
	ase_stx_object_t** free;
	ase_bool_t __dynamic;
};

struct ase_stx_symtab_t
{
	ase_word_t* datum;
	ase_word_t  size;
	ase_word_t  capacity;
};

struct ase_stx_t
{
	ase_stx_memory_t memory;
	ase_stx_symtab_t symtab;

	ase_word_t nil;
	ase_word_t true;
	ase_word_t false;

	ase_word_t smalltalk;

	ase_word_t class_symbol;
	ase_word_t class_metaclass;
	ase_word_t class_association;

	ase_word_t class_object;
	ase_word_t class_class;
	ase_word_t class_array;
	ase_word_t class_bytearray;
	ase_word_t class_string;
	ase_word_t class_character;
	ase_word_t class_context;
	ase_word_t class_system_dictionary;
	ase_word_t class_method;
	ase_word_t class_smallinteger;

	ase_bool_t __dynamic;
	ase_bool_t __wantabort; /* TODO: make it a function pointer */
};

#define ASE_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define ASE_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define ASE_STX_FROM_SMALLINT(x) ((x) >> 1)

#define ASE_STX_IS_OINDEX(x)     (((x) & 0x01) == 0x00)
#define ASE_STX_TO_OINDEX(x)     (((x) << 1) | 0x00)
#define ASE_STX_FROM_OINDEX(x)   ((x) >> 1)

#define ASE_STX_NIL   ASE_STX_TO_OINDEX(0)
#define ASE_STX_TRUE  ASE_STX_TO_OINDEX(1)
#define ASE_STX_FALSE ASE_STX_TO_OINDEX(2)

#define ASE_STX_OBJECT(stx,idx) (((stx)->memory).slots[ASE_STX_FROM_OINDEX(idx)])
#define ASE_STX_CLASS(stx,idx)  (ASE_STX_OBJECT(stx,(idx))->header.class)
#define ASE_STX_ACCESS(stx,idx) (ASE_STX_OBJECT(stx,(idx))->header.access)
#define ASE_STX_DATA(stx,idx)   ((void*)(ASE_STX_OBJECT(stx,idx) + 1))

#define ASE_STX_TYPE(stx,idx) (ASE_STX_ACCESS(stx,idx) & 0x03)
#define ASE_STX_SIZE(stx,idx) (ASE_STX_ACCESS(stx,idx) >> 0x02)

#define ASE_STX_WORD_INDEXED  (0x00)
#define ASE_STX_BYTE_INDEXED  (0x01)
#define ASE_STX_CHAR_INDEXED  (0x02)

#define ASE_STX_IS_WORD_OBJECT(stx,idx) \
	(ASE_STX_TYPE(stx,idx) == ASE_STX_WORD_INDEXED)
#define ASE_STX_IS_BYTE_OBJECT(stx,idx) \
	(ASE_STX_TYPE(stx,idx) == ASE_STX_BYTE_INDEXED)
#define ASE_STX_IS_CHAR_OBJECT(stx,idx) \
	(ASE_STX_TYPE(stx,idx) == ASE_STX_CHAR_INDEXED)

#define ASE_STX_WORD_OBJECT(stx,idx) \
	((ase_stx_word_object_t*)ASE_STX_OBJECT(stx,idx))
#define ASE_STX_BYTE_OBJECT(stx,idx) \
	((ase_stx_byte_object_t*)ASE_STX_OBJECT(stx,idx))
#define ASE_STX_CHAR_OBJECT(stx,idx) \
	((ase_stx_char_object_t*)ASE_STX_OBJECT(stx,idx))

/*
#define ASE_STX_WORD_AT(stx,idx,n) \
	(((ase_word_t*)(ASE_STX_OBJECT(stx,idx) + 1))[n])
#define ASE_STX_BYTE_AT(stx,idx,n) \
	(((ase_byte_t*)(ASE_STX_OBJECT(stx,idx) + 1))[n])
#define ASE_STX_CHAR_AT(stx,idx,n) \
	(((ase_char_t*)(ASE_STX_OBJECT(stx,idx) + 1))[n])
*/
#define ASE_STX_WORD_AT(stx,idx,n) \
	(ASE_STX_WORD_OBJECT(stx,idx)->data[n])
#define ASE_STX_BYTE_AT(stx,idx,n) \
	(ASE_STX_BYTE_OBJECT(stx,idx)->data[n])
#define ASE_STX_CHAR_AT(stx,idx,n) \
	(ASE_STX_CHAR_OBJECT(stx,idx)->data[n])

#ifdef __cplusplus
extern "C" {
#endif

ase_stx_t* ase_stx_open (ase_stx_t* stx, ase_word_t capacity);
void ase_stx_close (ase_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
