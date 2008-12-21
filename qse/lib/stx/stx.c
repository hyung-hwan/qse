/*
 * $Id: stx.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/stx.h>
#include <qse/stx/memory.h>
#include <qse/stx/misc.h>

qse_stx_t* qse_stx_open (qse_stx_t* stx, qse_word_t capacity)
{
	qse_word_t i;

	if (stx == QSE_NULL) {
		stx = (qse_stx_t*)qse_malloc (qse_sizeof(stx));
		if (stx == QSE_NULL) return QSE_NULL;
		stx->__dynamic = qse_true;
	}
	else stx->__dynamic = qse_false;

	if (qse_stx_memory_open (&stx->memory, capacity) == QSE_NULL) {
		if (stx->__dynamic) qse_free (stx);
		return QSE_NULL;
	}

	stx->symtab.size = 0;
	stx->symtab.capacity = 128; /* TODO: symbol table size */
	stx->symtab.datum = (qse_word_t*)qse_malloc (
		qse_sizeof(qse_word_t) * stx->symtab.capacity);
	if (stx->symtab.datum == QSE_NULL) {
		qse_stx_memory_close (&stx->memory);
		if (stx->__dynamic) qse_free (stx);
		return QSE_NULL;
	}

	stx->nil = QSE_STX_NIL;
	stx->true = QSE_STX_TRUE;
	stx->false = QSE_STX_FALSE;

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

	for (i = 0; i < stx->symtab.capacity; i++) {
		stx->symtab.datum[i] = stx->nil;
	}
	
	stx->__wantabort = qse_false;
	return stx;
}

void qse_stx_close (qse_stx_t* stx)
{
	qse_free (stx->symtab.datum);
	qse_stx_memory_close (&stx->memory);
	if (stx->__dynamic) qse_free (stx);
}

