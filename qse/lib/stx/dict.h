/*
 * $Id: dict.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_DICT_H_
#define _QSE_STX_DICT_H_

#include <qse/stx/stx.h>

#define QSE_STX_ASSOCIATION_SIZE  2
#define QSE_STX_ASSOCIATION_KEY   0
#define QSE_STX_ASSOCIATION_VALUE 1

struct qse_stx_association_t
{
	qse_stx_objhdr_t header;
	qse_word_t key;
	qse_word_t value;
};

typedef struct qse_stx_association_t qse_stx_association_t;

#ifdef __cplusplus
extern "C"
#endif

qse_word_t qse_stx_dict_lookup (
        qse_stx_t* stx, qse_word_t dict, const qse_char_t* key);
qse_word_t qse_stx_dict_get (
        qse_stx_t* stx, qse_word_t dict, qse_word_t key);
qse_word_t qse_stx_dict_put (
        qse_stx_t* stx, qse_word_t dict, qse_word_t key, qse_word_t value);
void qse_stx_dict_traverse (
        qse_stx_t* stx, qse_word_t dict,
        void (*func) (qse_stx_t*,qse_word_t,void*), void* data);


#ifdef __cplusplus
}
#endif

#endif
