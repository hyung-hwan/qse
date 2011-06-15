/*
 * $Id: symbol.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_LIB_STX_SYM_H_
#define _QSE_LIB_STX_SYM_H_

/* The SystemSymbolTable is a variable word class. 
 * The info below is for the fixed part only */
#define QSE_STX_SYMTAB_SIZE  1
#define QSE_STX_SYMTAB_TALLY 0

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_newsymbol (
	qse_stx_t*        stx,
	const qse_char_t* name
);

#ifdef __cplusplus
}
#endif

#endif
