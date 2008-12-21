/*
 * $Id: stx.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_STX_H_
#define _QSE_STX_STX_H_

#include <qse/cmn/types.h>
#include <qse/cmn/macros.h>

typedef struct qse_stx_objhdr_t qse_stx_objhdr_t;
typedef struct qse_stx_object_t qse_stx_object_t;
typedef struct qse_stx_word_object_t qse_stx_word_object_t;
typedef struct qse_stx_byte_object_t qse_stx_byte_object_t;
typedef struct qse_stx_char_object_t qse_stx_char_object_t;
typedef struct qse_stx_memory_t qse_stx_memory_t;
typedef struct qse_stx_symtab_t qse_stx_symtab_t;
typedef struct qse_stx_t qse_stx_t;

/* common object structure */
struct qse_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
	qse_word_t access; 
	qse_word_t class;
};

struct qse_stx_object_t
{
	qse_stx_objhdr_t header;
};

struct qse_stx_word_object_t
{
	qse_stx_objhdr_t header;
	qse_word_t data[1];
};

struct qse_stx_byte_object_t
{
	qse_stx_objhdr_t header;
	qse_byte_t data[1];
};

struct qse_stx_char_object_t
{
	qse_stx_objhdr_t header;
	qse_char_t data[1];
};


struct qse_stx_memory_t
{
	qse_word_t capacity;
	qse_stx_object_t** slots;
	qse_stx_object_t** free;
	qse_bool_t __dynamic;
};

struct qse_stx_symtab_t
{
	qse_word_t* datum;
	qse_word_t  size;
	qse_word_t  capacity;
};

struct qse_stx_t
{
	qse_stx_memory_t memory;
	qse_stx_symtab_t symtab;

	qse_word_t nil;
	qse_word_t true;
	qse_word_t false;

	qse_word_t smalltalk;

	qse_word_t class_symbol;
	qse_word_t class_metaclass;
	qse_word_t class_association;

	qse_word_t class_object;
	qse_word_t class_class;
	qse_word_t class_array;
	qse_word_t class_bytearray;
	qse_word_t class_string;
	qse_word_t class_character;
	qse_word_t class_context;
	qse_word_t class_system_dictionary;
	qse_word_t class_method;
	qse_word_t class_smallinteger;

	qse_bool_t __dynamic;
	qse_bool_t __wantabort; /* TODO: make it a function pointer */
};

#define QSE_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define QSE_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define QSE_STX_FROM_SMALLINT(x) ((x) >> 1)

#define QSE_STX_IS_OINDEX(x)     (((x) & 0x01) == 0x00)
#define QSE_STX_TO_OINDEX(x)     (((x) << 1) | 0x00)
#define QSE_STX_FROM_OINDEX(x)   ((x) >> 1)

#define QSE_STX_NIL   QSE_STX_TO_OINDEX(0)
#define QSE_STX_TRUE  QSE_STX_TO_OINDEX(1)
#define QSE_STX_FALSE QSE_STX_TO_OINDEX(2)

#define QSE_STX_OBJECT(stx,idx) (((stx)->memory).slots[QSE_STX_FROM_OINDEX(idx)])
#define QSE_STX_CLASS(stx,idx)  (QSE_STX_OBJECT(stx,(idx))->header.class)
#define QSE_STX_ACCESS(stx,idx) (QSE_STX_OBJECT(stx,(idx))->header.access)
#define QSE_STX_DATA(stx,idx)   ((void*)(QSE_STX_OBJECT(stx,idx) + 1))

#define QSE_STX_TYPE(stx,idx) (QSE_STX_ACCESS(stx,idx) & 0x03)
#define QSE_STX_SIZE(stx,idx) (QSE_STX_ACCESS(stx,idx) >> 0x02)

#define QSE_STX_WORD_INDEXED  (0x00)
#define QSE_STX_BYTE_INDEXED  (0x01)
#define QSE_STX_CHAR_INDEXED  (0x02)

#define QSE_STX_IS_WORD_OBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_WORD_INDEXED)
#define QSE_STX_IS_BYTE_OBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_BYTE_INDEXED)
#define QSE_STX_IS_CHAR_OBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_CHAR_INDEXED)

#define QSE_STX_WORD_OBJECT(stx,idx) \
	((qse_stx_word_object_t*)QSE_STX_OBJECT(stx,idx))
#define QSE_STX_BYTE_OBJECT(stx,idx) \
	((qse_stx_byte_object_t*)QSE_STX_OBJECT(stx,idx))
#define QSE_STX_CHAR_OBJECT(stx,idx) \
	((qse_stx_char_object_t*)QSE_STX_OBJECT(stx,idx))

/*
#define QSE_STX_WORD_AT(stx,idx,n) \
	(((qse_word_t*)(QSE_STX_OBJECT(stx,idx) + 1))[n])
#define QSE_STX_BYTE_AT(stx,idx,n) \
	(((qse_byte_t*)(QSE_STX_OBJECT(stx,idx) + 1))[n])
#define QSE_STX_CHAR_AT(stx,idx,n) \
	(((qse_char_t*)(QSE_STX_OBJECT(stx,idx) + 1))[n])
*/
#define QSE_STX_WORD_AT(stx,idx,n) \
	(QSE_STX_WORD_OBJECT(stx,idx)->data[n])
#define QSE_STX_BYTE_AT(stx,idx,n) \
	(QSE_STX_BYTE_OBJECT(stx,idx)->data[n])
#define QSE_STX_CHAR_AT(stx,idx,n) \
	(QSE_STX_CHAR_OBJECT(stx,idx)->data[n])

#ifdef __cplusplus
extern "C" {
#endif

qse_stx_t* qse_stx_open (qse_stx_t* stx, qse_word_t capacity);
void qse_stx_close (qse_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
