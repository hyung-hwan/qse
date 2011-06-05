/*
 * $Id: stx.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include "stx.h"
#include "mem.h"
#include "boot.h"
#include "../cmn/mem.h"

qse_stx_t* qse_stx_init (qse_stx_t* stx, qse_mmgr_t* mmgr, qse_size_t memcapa)
{
	QSE_MEMSET (stx, 0, QSE_SIZEOF(*stx));
	stx->mmgr = mmgr;

	/* initialize object memory subsystem */
	if (qse_stx_initmem (stx, memcapa) <= -1) return QSE_NULL;

	/* perform initial bootstrapping */
/* TODO: if image file is available, load it.... */
	if (qse_stx_boot (stx) <= -1)
	{
		qse_stx_finimem (stx);
		return QSE_NULL;
	}

#if 0
	if (qse_stx_initsymtab (stx, 128) <= -1)
	{
		qse_stx_finimem (stx);
		return QSE_NULL;
	}

	stx->symtab.size = 0;
	stx->symtab.capacity = 128; /* TODO: symbol table size */
	stx->symtab.datum = (qse_word_t*)qse_malloc (
		qse_sizeof(qse_word_t) * stx->symtab.capacity);
	if (stx->symtab.datum == QSE_NULL) 
	{
		qse_stx_memory_close (&stx->memory);
		return QSE_NULL;
	}

	stx->ref.nil = QSE_STX_NIL;
	stx->ref.true = QSE_STX_TRUE;
	stx->ref.false = QSE_STX_FALSE;

	stx->smalltalk = QSE_STX_NIL;

	stx->class_symbol = QSE_STX_NIL;
	stx->class_metaclass = QSE_STX_NIL;
	stx->class_association = QSE_STX_NIL;

	stx->class_object = QSE_STX_NIL;
	stx->class_class = QSE_STX_NIL;
	stx->class_array = QSE_STX_NIL;
	stx->class_bytearray = QSE_STX_NIL;
	stx->class_string = QSE_STX_NIL;
	stx->class_character = QSE_STX_NIL;
	stx->class_context = QSE_STX_NIL;
	stx->class_system_dictionary = QSE_STX_NIL;
	stx->class_method = QSE_STX_NIL;
	stx->class_smallinteger = QSE_STX_NIL;

	for (i = 0; i < stx->symtab.capacity; i++) 
	{
		stx->symtab.datum[i] = stx->nil;
	}
#endif
	
	return stx;
}

void qse_stx_fini (qse_stx_t* stx)
{
	qse_stx_finimem (stx);
}

qse_stx_t* qse_stx_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t memcapa)
{
	qse_stx_t* stx;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();
		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");
		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	stx = (qse_stx_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_stx_t) + xtnsize
	);
	if (stx == QSE_NULL) return QSE_NULL;

	if (qse_stx_init (stx, mmgr, memcapa) == QSE_NULL)
	{
		QSE_MMGR_FREE (stx->mmgr, stx);
		return QSE_NULL;
	}

	return stx;
}

void qse_stx_close (qse_stx_t* stx)
{
	qse_stx_fini (stx);
	QSE_MMGR_FREE (stx->mmgr, stx);
}

