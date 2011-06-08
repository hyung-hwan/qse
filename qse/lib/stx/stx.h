/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_STX_H_
#define _QSE_LIB_STX_STX_H_

#include <qse/stx/stx.h>

typedef qse_word_t qse_stx_objidx_t;
#define QSE_STX_OBJIDX_INVALID ((qse_stx_objidx_t)-1)

typedef struct qse_stx_objhdr_t   qse_stx_objhdr_t; /* object header */
typedef struct qse_stx_object_t   qse_stx_object_t; /* abstract object */

typedef struct qse_stx_object_t*  qse_stx_objptr_t; /* object pointer */
typedef struct qse_stx_byteobj_t* qse_stx_byteobjptr_t;
typedef struct qse_stx_charobj_t* qse_stx_charobjptr_t;
typedef struct qse_stx_wordobj_t* qse_stx_wordobjptr_t;

#include "hash.h"
#include "mem.h"
#include "obj.h"
#include "sym.h"
#include "dic.h"
#include "boot.h"

enum qse_stx_objtype_t
{
	QSE_STX_BYTEOBJ = 0,
	QSE_STX_CHAROBJ = 1,
	QSE_STX_WORDOBJ = 2
};
typedef enum qse_stx_objtype_t qse_stx_objtype_t;

struct qse_stx_objhdr_t
{
	/* access - type: 2; size: rest;
	 * type - word indexed: 00 byte indexed: 01 char indexed: 10
	 */
/* TODO: change the order depending on endian.... */
/* mark, type(pinter,wordindexed,indexable), reference-count */
/*
has pointer -> word, byte
variable -> variable, fixed;

word variable => 
char variable =>
byte variale =>

byte fixed => not possible
char fixed => not possible
word fixed
*/
	qse_word_t _mark: 1;
	qse_word_t _type: 2; 
	qse_word_t _variable: 1;
	qse_word_t _refcnt: (QSE_SIZEOF_WORD_T-4); 

	qse_word_t _size;
	qse_word_t _class;
	qse_word_t _backref;
};

struct qse_stx_object_t
{
	qse_stx_objhdr_t h;
};

struct qse_stx_wordobj_t
{
	qse_stx_objhdr_t h;
	qse_word_t   fld[1];
};

struct qse_stx_byteobj_t
{
	qse_stx_objhdr_t h;
	qse_byte_t       fld[1];
};

struct qse_stx_charobj_t
{
	qse_stx_objhdr_t h;
	qse_char_t       fld[1];
};

struct qse_stx_t
{
	QSE_DEFINE_COMMON_FIELDS (stx)

     /** error information */
	struct
	{
		qse_stx_errstr_t str;      /**< error string getter */
		qse_stx_errnum_t num;      /**< stores an error number */
		qse_char_t       msg[128]; /**< error message holder */
		qse_stx_loc_t    loc;      /**< location of the last error */
	} err;

	struct
	{
		qse_size_t capa;
		qse_stx_objptr_t* slot;	
		qse_stx_objptr_t* free;
	} mem;

	struct 
	{
		qse_size_t  capa;
		qse_size_t  size;
		qse_word_t* slot;
	} symtab;

	struct
	{
		qse_word_t nil;
		qse_word_t true;
		qse_word_t false;

		qse_word_t symtab; /* symbol table */
		qse_word_t sysdic; /* system dictionary */
	
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
	} ref;

	qse_bool_t __wantabort; /* TODO: make it a function pointer */
};

/** 
 * The QSE_STX_REFISINT macro determines if the object reference is encoded
 * of a small integer by checking if the bit 0 is on.
 */
#define QSE_STX_REFISINT(stx,x) (((x) & 0x01) == 0x01)
/**
 * The QSE_STX_INTTOREF macro encodes a small integer to an object reference.
 */
#define QSE_STX_INTTOREF(stx,x) (((x) << 1) | 0x01)
/**
 * The QSE_STX_REFTOINT macro decodes an object reference to a small integer.
 */
#define QSE_STX_REFTOINT(stx,x)  ((x) >> 1)

/**
 * The QSE_STX_REFISIDX macro determines if the object reference is encoded
 * of an object table index by checking if the bit 0 is off.
 */
#define QSE_STX_REFISIDX(stx,x) (((x) & 0x01) == 0x00)
/**
 * The QSE_STX_IDXTOREF macro encodes a object table index to an object
 * reference.
 */
#define QSE_STX_IDXTOREF(stx,x) (((x) << 1) | 0x00)
/** 
 * The QSE_STX_REFTOIDX macro decodes an object reference to a object 
 * table index.
 */
#define QSE_STX_REFTOIDX(stx,x) ((x) >> 1)

/* get the object pointer by the raw object table index */
#define QSE_STX_PTRBYIDX(stx,x) ((stx)->mem.slot[x])
/* get the object pointer by the encoded object table index */
#define QSE_STX_PTRBYREF(stx,x) QSE_STX_PTRBYIDX(stx,QSE_STX_REFTOIDX(stx,x))

#define QSE_STX_OBJTYPE(stx,ref)  (QSE_STX_PTRBYREF(stx,ref)->h._type)
#define QSE_STX_OBJSIZE(stx,ref)  (QSE_STX_PTRBYREF(stx,ref)->h._size)
#define QSE_STX_OBJCLASS(stx,ref) (QSE_STX_PTRBYREF(stx,ref)->h._class)

#define QSE_STX_WORDAT(stx,ref,pos) \
	(((qse_stx_wordobjptr_t)QSE_STX_PTRBYREF(stx,ref))->fld[pos])
#define QSE_STX_BYTEAT(stx,ref,pos) \
	(((qse_stx_byteobjptr_t)QSE_STX_PTRBYREF(stx,ref))->fld[pos])
#define QSE_STX_CHARAT(stx,ref,pos) \
	(((qse_stx_charobjptr_t)QSE_STX_PTRBYREF(stx,ref))->fld[pos])

/* REDEFINITION DROPPING PREFIX FOR INTERNAL USE */
#define REFISINT(stx,x)     QSE_STX_REFISINT(stx,x)
#define INTTOREF(stx,x)     QSE_STX_INTTOREF(stx,x)
#define REFTOINT(stx,x)     QSE_STX_REFTOINT(stx,x)

#define REFISIDX(stx,x)     QSE_STX_REFISIDX(stx,x)
#define IDXTOREF(stx,x)     QSE_STX_IDXTOREF(stx,x)
#define REFTOIDX(stx,x)     QSE_STX_REFTOIDX(stx,x)

#define PTRBYREF(stx,x)     QSE_STX_PTRBYREF(stx,x)
#define PTRBYIDX(stx,x)     QSE_STX_PTRBYIDX(stx,x)

#define OBJTYPE(stx,ref)    QSE_STX_OBJTYPE(stx,ref)
#define OBJCLASS(stx,ref)   QSE_STX_OBJCLASS(stx,ref)
#define OBJSIZE(stx,ref)    QSE_STX_OBJSIZE(stx,ref)

#define BYTEAT(stx,ref,pos) QSE_STX_BYTEAT(stx,ref,pos)
#define CHARAT(stx,ref,pos) QSE_STX_CHARAT(stx,ref,pos)
#define WORDAT(stx,ref,pos) QSE_STX_WORDAT(stx,ref,pos)

#define BYTEOBJ             QSE_STX_BYTEOBJ
#define CHAROBJ             QSE_STX_CHAROBJ
#define WORDOBJ             QSE_STX_WORDOBJ

#if 0
/* hardcoded object reference */
#define QSE_STX_NIL(stx)   QSE_STX_IDXTOREF(stx,0)
#define QSE_STX_TRUE(stx)  QSE_STX_IDXTOREF(stx,1)
#define QSE_STX_FALSE(stx) QSE_STX_IDXTOREF(stx,2)

#define QSE_STX_DATA(stx,idx)     ((void*)(QSE_STX_OBJPTR(stx,idx) + 1))
#endif


#if 0
#define QSE_STX_WORD_INDEXED  (0x00)
#define QSE_STX_BYTE_INDEXED  (0x01)
#define QSE_STX_CHAR_INDEXED  (0x02)

/* this type has nothing to do with
 * the indexability of the object... */
enum qse_stx_objtype_t
{
	QSE_STX_OBJTYPE_BYTE,
	QSE_STX_OBJTYPE_CHAR,
	QSE_STX_OBJTYPE_WORD
};

#define QSE_STX_ISWORDOBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_WORD_INDEXED)
#define QSE_STX_ISBYTEOBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_BYTE_INDEXED)
#define QSE_STX_ISCHAROBJECT(stx,idx) \
	(QSE_STX_TYPE(stx,idx) == QSE_STX_CHAR_INDEXED)

#define QSE_STX_WORDOBJPTR(stx,idx) \
	((qse_stx_word_object_t*)QSE_STX_OBJPTR(stx,idx))
#define QSE_STX_BYTEOBJPTR(stx,idx) \
	((qse_stx_byte_object_t*)QSE_STX_OBJPTR(stx,idx))
#define QSE_STX_CHAROBJPTR(stx,idx) \
	((qse_stx_char_object_t*)QSE_STX_OBJPTR(stx,idx))

#define QSE_STX_WORD_AT(stx,idx,n) \
	(QSE_STX_WORD_OBJECT(stx,idx)->data[n])
#define QSE_STX_BYTE_AT(stx,idx,n) \
	(QSE_STX_BYTE_OBJECT(stx,idx)->data[n])
#define QSE_STX_CHAR_AT(stx,idx,n) \
	(QSE_STX_CHAR_OBJECT(stx,idx)->data[n])

#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 
 */

#ifdef __cplusplus
}
#endif

#endif
