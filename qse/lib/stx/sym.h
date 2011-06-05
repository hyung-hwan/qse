/*
 * $Id: symbol.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_LIB_STX_SYM_H_
#define _QSE_LIB_STX_SYM_H_

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_newsymbol (
	qse_stx_t*        stx,
	qse_word_t        tabref,
	const qse_char_t* name
);

#ifdef __cplusplus
}
#endif

#endif
