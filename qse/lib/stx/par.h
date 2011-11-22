/*
 * $Id: stc.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_LIB_STX_PAR_H_
#define _QSE_LIB_STX_PAR_H_

#include "stx.h"

#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>

enum
{
	QSE_STC_ERROR_NONE,

	/* system errors */
	QSE_STC_ERROR_INPUT_FUNC,
	QSE_STC_ERROR_INPUT,
	QSE_STC_ERROR_MEMORY,

	/* lexical errors */
	QSE_STC_ERROR_CHAR,
	QSE_STC_ERROR_CHARLIT,
	QSE_STC_ERROR_STRLIT,
	QSE_STC_ERROR_LITERAL,

	/* syntatic error */
	QSE_STC_ERROR_MESSAGE_SELECTOR,
	QSE_STC_ERROR_ARGUMENT_NAME,
	QSE_STC_ERROR_TOO_MANY_ARGUMENTS,

	QSE_STC_ERROR_PRIMITIVE_KEYWORD,
	QSE_STC_ERROR_PRIMITIVE_NUMBER,
	QSE_STC_ERROR_PRIMITIVE_NUMBER_RANGE,
	QSE_STC_ERROR_PRIMITIVE_NOT_CLOSED,

	QSE_STC_ERROR_TEMPORARIES_NOT_CLOSED,
	QSE_STC_ERROR_TOO_MANY_TEMPORARIES,
	QSE_STC_ERROR_PSEUDO_VARIABLE,
	QSE_STC_ERROR_PRIMARY,

	QSE_STC_ERROR_NO_PERIOD,
	QSE_STC_ERROR_NO_RPAREN,
	QSE_STC_ERROR_BLOCK_ARGUMENT_NAME,
	QSE_STC_ERROR_BLOCK_ARGUMENT_LIST,
	QSE_STC_ERROR_BLOCK_NOT_CLOSED,

	QSE_STC_ERROR_UNDECLARED_NAME,
	QSE_STC_ERROR_TOO_MANY_LITERALS
};

enum
{
	/* input_func cmd */
	QSE_STC_INPUT_OPEN,
	QSE_STC_INPUT_CLOSE,
	QSE_STC_INPUT_CONSUME,
	QSE_STC_INPUT_REWIND
};

typedef struct qse_stc_t qse_stc_t;

struct qse_stc_t
{
	qse_mmgr_t* mmgr;

	qse_stx_t* stx;
	int error_code;

	qse_word_t method_class;
	qse_str_t method_name;

	qse_char_t* temporaries[256]; /* TODO: different size? or dynamic? */
	qse_word_t argument_count;
	qse_word_t temporary_count;

	qse_word_t literals[256]; /* TODO: make it a dynamic array */
	qse_word_t literal_count;

	qse_lda_t bytecode;

	struct
	{
		int type;
		/*
		qse_stx_int_t   ivalue;
		qse_stx_flt_t  fvalue;
		*/
		qse_str_t name;
	} token;

	qse_cint_t curc;
	qse_cint_t ungotc[5];
	qse_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);
};

#ifdef __cplusplus
extern "C" {
#endif

qse_stc_t* qse_stc_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	qse_stx_t*  stx
);

void qse_stc_close (
	qse_stc_t* stc
);

qse_stc_t* qse_stc_init (
	qse_stc_t* stc,
	qse_mmgr_t* mmgr,
	qse_stx_t* stx
);

void qse_stc_fini (
	qse_stc_t* stc
);


int qse_stc_parsemethod (
	qse_stc_t* stc,
	qse_word_t method_class,
	void*      input
);

#ifdef __cplusplus
}
#endif

#endif
