/*
 * $Id: parser.h,v 1.1 2007/03/28 14:05:28 bacon Exp $
 */

#ifndef _ASE_STX_PARSER_H_
#define _ASE_STX_PARSER_H_

#include <ase/stx/stx.h>
#include <ase/stx/name.h>
#include <ase/stx/token.h>
#include <ase/bas/arr.h>

enum
{
	ASE_STX_PARSER_ERROR_NONE,

	/* system errors */
	ASE_STX_PARSER_ERROR_INPUT_FUNC,
	ASE_STX_PARSER_ERROR_INPUT,
	ASE_STX_PARSER_ERROR_MEMORY,

	/* lexical errors */
	ASE_STX_PARSER_ERROR_CHAR,
	ASE_STX_PARSER_ERROR_CHARLIT,
	ASE_STX_PARSER_ERROR_STRLIT,
	ASE_STX_PARSER_ERROR_LITERAL,

	/* syntatic error */
	ASE_STX_PARSER_ERROR_MESSAGE_SELECTOR,
	ASE_STX_PARSER_ERROR_ARGUMENT_NAME,
	ASE_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS,

	ASE_STX_PARSER_ERROR_PRIMITIVE_KEYWORD,
	ASE_STX_PARSER_ERROR_PRIMITIVE_NUMBER,
	ASE_STX_PARSER_ERROR_PRIMITIVE_NUMBER_RANGE,
	ASE_STX_PARSER_ERROR_PRIMITIVE_NOT_CLOSED,

	ASE_STX_PARSER_ERROR_TEMPORARIES_NOT_CLOSED,
	ASE_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES,
	ASE_STX_PARSER_ERROR_PSEUDO_VARIABLE,
	ASE_STX_PARSER_ERROR_PRIMARY,

	ASE_STX_PARSER_ERROR_NO_PERIOD,
	ASE_STX_PARSER_ERROR_NO_RPAREN,
	ASE_STX_PARSER_ERROR_BLOCK_ARGUMENT_NAME,
	ASE_STX_PARSER_ERROR_BLOCK_ARGUMENT_LIST,
	ASE_STX_PARSER_ERROR_BLOCK_NOT_CLOSED,

	ASE_STX_PARSER_ERROR_UNDECLARED_NAME,
	ASE_STX_PARSER_ERROR_TOO_MANY_LITERALS
};

enum
{
	/* input_func cmd */
	ASE_STX_PARSER_INPUT_OPEN,
	ASE_STX_PARSER_INPUT_CLOSE,
	ASE_STX_PARSER_INPUT_CONSUME,
	ASE_STX_PARSER_INPUT_REWIND
};

typedef struct ase_stx_parser_t ase_stx_parser_t;

struct ase_stx_parser_t
{
	ase_stx_t* stx;
	int error_code;

	ase_word_t method_class;
	ase_stx_name_t method_name;

	ase_char_t* temporaries[256]; /* TODO: different size? or dynamic? */
	ase_word_t argument_count;
	ase_word_t temporary_count;

	ase_word_t literals[256]; /* TODO: make it a dynamic array */
	ase_word_t literal_count;

	ase_arr_t bytecode;

	ase_stx_token_t token;
	ase_cint_t curc;
	ase_cint_t ungotc[5];
	ase_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);

	ase_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_stx_parser_t* ase_stx_parser_open (ase_stx_parser_t* parser, ase_stx_t* stx);
void ase_stx_parser_close (ase_stx_parser_t* parser);

const ase_char_t* ase_stx_parser_error_string (ase_stx_parser_t* parser);
int ase_stx_parser_parse_method (
	ase_stx_parser_t* parser, ase_word_t method_class, void* input);

#ifdef __cplusplus
}
#endif

#endif
