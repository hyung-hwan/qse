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

typedef struct qse_stx_byteobj_t qse_stx_byteobj_t;
typedef struct qse_stx_charobj_t qse_stx_charobj_t;
typedef struct qse_stx_wordobj_t qse_stx_wordobj_t;

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
	qse_word_t _refcnt: ((QSE_SIZEOF_WORD_T*8)-4); 

	qse_word_t _size;
	qse_word_t _class;
	qse_word_t _backref;
};

#include "hash.h"
#include "mem.h"
#include "obj.h"
#include "sym.h"
#include "dic.h"
#include "cls.h"
#include "boot.h"

struct qse_stx_object_t
{
	qse_stx_objhdr_t h;
};

struct qse_stx_wordobj_t
{
	qse_stx_objhdr_t h;
	qse_word_t       slot[1];
};

struct qse_stx_byteobj_t
{
	qse_stx_objhdr_t h;
	qse_byte_t       slot[1];
};

struct qse_stx_charobj_t
{
	qse_stx_objhdr_t h;
	qse_char_t       slot[1];
};

struct qse_stx_t
{
	qse_mmgr_t* mmgr;

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
		qse_word_t nil;
		qse_word_t true;
		qse_word_t false;

		qse_word_t symtab; /* symbol table */
		qse_word_t sysdic; /* system dictionary */
	
		qse_word_t class_symbol;
		qse_word_t class_metaclass;
		qse_word_t class_association;
	
		qse_word_t class_object;
		qse_word_t class_undefinedobject;
		qse_word_t class_class;
		qse_word_t class_array;
		qse_word_t class_bytearray;
		qse_word_t class_string;
		qse_word_t class_character;
		qse_word_t class_context;
		qse_word_t class_systemsymboltable;
		qse_word_t class_systemdictionary;
		qse_word_t class_method;
		qse_word_t class_smallinteger;
		qse_word_t class_parser;
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

/* pointer to the body of the object past the header */
#define QSE_STX_WORDPTR(stx,ref) \
	(((qse_stx_wordobj_t*)QSE_STX_PTRBYREF(stx,ref))->slot)
#define QSE_STX_BYTEPTR(stx,ref) \
	(((qse_stx_byteobj_t*)QSE_STX_PTRBYREF(stx,ref))->slot)
#define QSE_STX_CHARPTR(stx,ref) \
	(((qse_stx_charobj_t*)QSE_STX_PTRBYREF(stx,ref))->slot)

#define QSE_STX_WORDLEN(stx,ref) OBJSIZE(stx,ref)
#define QSE_STX_BYTELEN(stx,ref) OBJSIZE(stx,ref)
#define QSE_STX_CHARLEN(stx,ref) OBJSIZE(stx,ref)

#define QSE_STX_WORDAT(stx,ref,pos) (QSE_STX_WORDPTR(stx,ref)[pos])
#define QSE_STX_BYTEAT(stx,ref,pos) (QSE_STX_BYTEPTR(stx,ref)[pos])
#define QSE_STX_CHARAT(stx,ref,pos) (QSE_STX_CHARPTR(stx,ref)[pos])

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


#define BYTEPTR(stx,ref)    QSE_STX_BYTEPTR(stx,ref)
#define CHARPTR(stx,ref)    QSE_STX_CHARPTR(stx,ref)
#define WORDPTR(stx,ref)    QSE_STX_WORDPTR(stx,ref)
#define BYTELEN(stx,ref)    QSE_STX_BYTELEN(stx,ref)
#define CHARLEN(stx,ref)    QSE_STX_CHARLEN(stx,ref)
#define WORDLEN(stx,ref)    QSE_STX_WORDLEN(stx,ref)
#define BYTEAT(stx,ref,pos) QSE_STX_BYTEAT(stx,ref,pos)
#define CHARAT(stx,ref,pos) QSE_STX_CHARAT(stx,ref,pos)
#define WORDAT(stx,ref,pos) QSE_STX_WORDAT(stx,ref,pos)

#define BYTEOBJ             QSE_STX_BYTEOBJ
#define CHAROBJ             QSE_STX_CHAROBJ
#define WORDOBJ             QSE_STX_WORDOBJ

/* SOME INTERNAL MACRO DEFINITIONS */
#define SYMTAB_INIT_CAPA 256
#define SYSDIC_INIT_CAPA 256

#define ISNIL(stx,obj) ((obj) == (stx)->ref.nil)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
