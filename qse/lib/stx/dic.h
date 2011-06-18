/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_DIC_H_
#define _QSE_LIB_STX_DIC_H_

#define QSE_STX_ASSOCIATION_SIZE  2
#define QSE_STX_ASSOCIATION_KEY   0
#define QSE_STX_ASSOCIATION_VALUE 1

/* The SystemDictionary is a variable word class. 
 * The info below is for the fixed part only */
#define QSE_STX_SYSTEMDICTIONARY_SIZE  1
#define QSE_STX_SYSTEMDICTIONARY_TALLY 0

#ifdef __cplusplus
extern "C"
#endif

qse_word_t qse_stx_lookupdic (
	qse_stx_t*        stx,
	qse_word_t        dic,
	const qse_char_t* key
);

qse_word_t qse_stx_getdic (
	qse_stx_t* stx,
	qse_word_t dic,
	qse_word_t key
);

qse_word_t qse_stx_putdic (
	qse_stx_t* stx,
	qse_word_t dic,
	qse_word_t key,
	qse_word_t value
);

void qse_stx_walkdic (
	qse_stx_t* stx,
	qse_word_t dic,
	void (*func) (qse_stx_t*,qse_word_t,void*),
	void* data
);

#ifdef __cplusplus
}
#endif

#endif
