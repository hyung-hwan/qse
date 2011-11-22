/*
 * $Id: token.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_TOKEN_H_
#define _QSE_STX_TOKEN_H_

#include <qse/stx/stx.h>
#include <qse/stx/name.h>

enum
{
	QSE_STX_TOKEN_END,
	QSE_STX_TOKEN_CHARLIT,
	QSE_STX_TOKEN_STRLIT,
	QSE_STX_TOKEN_SYMLIT,
	QSE_STX_TOKEN_NUMLIT,
	QSE_STX_TOKEN_IDENT,
	QSE_STX_TOKEN_BINARY,
	QSE_STX_TOKEN_KEYWORD,
	QSE_STX_TOKEN_PRIMITIVE,
	QSE_STX_TOKEN_ASSIGN,
	QSE_STX_TOKEN_COLON,
	QSE_STX_TOKEN_RETURN,
	QSE_STX_TOKEN_LBRACKET,
	QSE_STX_TOKEN_RBRACKET,
	QSE_STX_TOKEN_LPAREN,
	QSE_STX_TOKEN_RPAREN,
	QSE_STX_TOKEN_APAREN,
	QSE_STX_TOKEN_PERIOD,
	QSE_STX_TOKEN_SEMICOLON
};

struct qse_stx_token_t 
{
	int type;

	/*
	qse_stx_int_t  ivalue;
	qse_stx_flt_t  fvalue;
	*/
	qse_stx_name_t name;
	qse_bool_t __dynamic;
};

typedef struct qse_stx_token_t qse_stx_token_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_stx_token_t* qse_stx_token_open (
	qse_stx_token_t* token, qse_word_t capacity);
void qse_stx_token_close (qse_stx_token_t* token);

int qse_stx_token_addc (qse_stx_token_t* token, qse_cint_t c);
int qse_stx_token_adds (qse_stx_token_t* token, const qse_char_t* s);
void qse_stx_token_clear (qse_stx_token_t* token);
qse_char_t* qse_stx_token_yield (qse_stx_token_t* token, qse_word_t capacity);
int qse_stx_token_compare_name (qse_stx_token_t* token, const qse_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
