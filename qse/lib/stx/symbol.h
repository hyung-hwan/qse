/*
 * $Id: symbol.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_SYMBOL_H_
#define _QSE_STX_SYMBOL_H_

#include <qse/stx/stx.h>

#define QSE_STX_SYMLINK_SIZE   2
#define QSE_STX_SYMLINK_LINK   0
#define QSE_STX_SYMLINK_SYMBOL 1

struct qse_stx_symlink_t
{
	qse_stx_objhdr_t header;
	qse_word_t link;
	qse_word_t symbol;
};

typedef struct qse_stx_symlink_t qse_stx_symlink_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_new_symbol_link (qse_stx_t* stx, qse_word_t sym);

qse_word_t qse_stx_new_symbol (
	qse_stx_t* stx, const qse_char_t* name);
qse_word_t qse_stx_new_symbolx (
	qse_stx_t* stx, const qse_char_t* name, qse_word_t len);
void qse_stx_traverse_symbol_table (
        qse_stx_t* stx, void (*func) (qse_stx_t*,qse_word_t,void*), void* data);

#ifdef __cplusplus
}
#endif

#endif
