/*
 * $Id: parser.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_PARSER_H_
#define _QSE_STX_PARSER_H_

#include <qse/stx/stx.h>
#include <qse/stx/name.h>
#include <qse/stx/token.h>
#include <qse/bas/arr.h>

enum
{
	QSE_STX_PARSER_ERROR_NONE,

	/* system errors */
	QSE_STX_PARSER_ERROR_INPUT_FUNC,
	QSE_STX_PARSER_ERROR_INPUT,
	QSE_STX_PARSER_ERROR_MEMORY,

	/* lexical errors */
	QSE_STX_PARSER_ERROR_CHAR,
	QSE_STX_PARSER_ERROR_CHARLIT,
	QSE_STX_PARSER_ERROR_STRLIT,
	QSE_STX_PARSER_ERROR_LITERAL,

	/* syntatic error */
	QSE_STX_PARSER_ERROR_MESSAGE_SELECTOR,
	QSE_STX_PARSER_ERROR_ARGUMENT_NAME,
	QSE_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS,

	QSE_STX_PARSER_ERROR_PRIMITIVE_KEYWORD,
	QSE_STX_PARSER_ERROR_PRIMITIVE_NUMBER,
	QSE_STX_PARSER_ERROR_PRIMITIVE_NUMBER_RANGE,
	QSE_STX_PARSER_ERROR_PRIMITIVE_NOT_CLOSED,

	QSE_STX_PARSER_ERROR_TEMPORARIES_NOT_CLOSED,
	QSE_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES,
	QSE_STX_PARSER_ERROR_PSEUDO_VARIABLE,
	QSE_STX_PARSER_ERROR_PRIMARY,

	QSE_STX_PARSER_ERROR_NO_PERIOD,
	QSE_STX_PARSER_ERROR_NO_RPAREN,
	QSE_STX_PARSER_ERROR_BLOCK_ARGUMENT_NAME,
	QSE_STX_PARSER_ERROR_BLOCK_ARGUMENT_LIST,
	QSE_STX_PARSER_ERROR_BLOCK_NOT_CLOSED,

	QSE_STX_PARSER_ERROR_UNDECLARED_NAME,
	QSE_STX_PARSER_ERROR_TOO_MANY_LITERALS
};

enum
{
	/* input_func cmd */
	QSE_STX_PARSER_INPUT_OPEN,
	QSE_STX_PARSER_INPUT_CLOSE,
	QSE_STX_PARSER_INPUT_CONSUME,
	QSE_STX_PARSER_INPUT_REWIND
};

typedef struct qse_stx_parser_t qse_stx_parser_t;

struct qse_stx_parser_t
{
	qse_stx_t* stx;
	int error_code;

	qse_word_t method_class;
	qse_stx_name_t method_name;

	qse_char_t* temporaries[256]; /* TODO: different size? or dynamic? */
	qse_word_t argument_count;
	qse_word_t temporary_count;

	qse_word_t literals[256]; /* TODO: make it a dynamic array */
	qse_word_t literal_count;

	qse_arr_t bytecode;

	qse_stx_token_t token;
	qse_cint_t curc;
	qse_cint_t ungotc[5];
	qse_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);

	qse_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

qse_stx_parser_t* qse_stx_parser_open (qse_stx_parser_t* parser, qse_stx_t* stx);
void qse_stx_parser_close (qse_stx_parser_t* parser);

const qse_char_t* qse_stx_parser_error_string (qse_stx_parser_t* parser);
int qse_stx_parser_parse_method (
	qse_stx_parser_t* parser, qse_word_t method_class, void* input);

#ifdef __cplusplus
}
#endif

#endif
