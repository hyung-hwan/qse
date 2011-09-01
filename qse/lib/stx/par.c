/*
 * $Id$
 */

#if 0
#include "par.h"
#include "../cmn/mem.h"

enum
{
	TOKEN_END,
	TOKEN_CHARLIT,
	TOKEN_STRLIT,
	TOKEN_SYMLIT,
	TOKEN_NUMLIT,
	TOKEN_IDENT,
	TOKEN_BINARY,
	TOKEN_KEYWORD,
	TOKEN_PRIMITIVE,
	TOKEN_ASSIGN,
	TOKEN_COLON,
	TOKEN_RETURN,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_APAREN,
	TOKEN_PERIOD,
	TOKEN_SEMICOLON
};

static int __parse_method (
	qse_stc_t* stc, 
	qse_word_t method_class, void* input);
static int __finish_method (qse_stc_t* stc);

static int __parse_message_pattern (qse_stc_t* stc);
static int __parse_unary_pattern (qse_stc_t* stc);
static int __parse_binary_pattern (qse_stc_t* stc);
static int __parse_keyword_pattern (qse_stc_t* stc);

static int __parse_temporaries (qse_stc_t* stc);
static int __parse_primitive (qse_stc_t* stc);
static int __parse_statements (qse_stc_t* stc);
static int __parse_block_statements (qse_stc_t* stc);
static int __parse_statement (qse_stc_t* stc);
static int __parse_expression (qse_stc_t* stc);

static int __parse_assignment (
	qse_stc_t* stc, const qse_char_t* target);
static int __parse_basic_expression (
	qse_stc_t* stc, const qse_char_t* ident);
static int __parse_primary (
	qse_stc_t* stc, const qse_char_t* ident, qse_bool_t* is_super);
static int __parse_primary_ident (
	qse_stc_t* stc, const qse_char_t* ident, qse_bool_t* is_super);

static int __parse_block_constructor (qse_stc_t* stc);
static int __parse_message_continuation (
	qse_stc_t* stc, qse_bool_t is_super);
static int __parse_keyword_message (
	qse_stc_t* stc, qse_bool_t is_super);
static int __parse_binary_message (
	qse_stc_t* stc, qse_bool_t is_super);
static int __parse_unary_message (
	qse_stc_t* stc, qse_bool_t is_super);

static int __get_token (qse_stc_t* stc);
static int __get_ident (qse_stc_t* stc);
static int __get_numlit (qse_stc_t* stc, qse_bool_t negated);
static int __get_charlit (qse_stc_t* stc);
static int __get_strlit (qse_stc_t* stc);
static int __get_binary (qse_stc_t* stc);
static int __skip_spaces (qse_stc_t* stc);
static int __skip_comment (qse_stc_t* stc);
static int __get_char (qse_stc_t* stc);
static int __unget_char (qse_stc_t* stc, qse_cint_t c);
static int __open_input (qse_stc_t* stc, void* input);
static int __close_input (qse_stc_t* stc);

qse_stc_t* qse_stc_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_stx_t* stx)
{
	qse_stc_t* stc;

	stc = (qse_stc_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*stc) + xtnsize);		
	if (stc == QSE_NULL) return QSE_NULL;

	if (qse_stc_init (stc, mmgr, stx) <= -1)
	{
		QSE_MMGR_FREE (mmgr, stc);
		return QSE_NULL;
	}

	return stc;
}

void qse_stc_close (qse_stc_t* stc)
{
	qse_stc_fini (stc);
	QSE_MMGR_FREE (stc->mmgr, stc);
}

int qse_stc_init (qse_stc_t* stc, qse_mmgr_t* mmgr, qse_stx_t* stx)
{
	QSE_MEMSET (stc, 0, QSE_SIZEOF(*stc));
	stc->mmgr = mmgr;
	stc->stx = stx;

	if (qse_str_init (&stc->method_name, mmgr, 0) <= -1)
	{
		return -1;
	}

	if (qse_str_init (&stc->token.name, mmgr, 0) <= -1)
	{
		qse_str_fini (&stc->method_name);
		return -1;
	}
	stc->token.type = TOKEN_END;

/* TODO:create a bytearray data type...
 *      i think i can reuse qse_mbs_t ... by dropping the null-termination part...
qse_bar_t bryte array....
qse_lba_t linear byte array
 */
	if (qse_lda_init (
		&stc->bytecode, mmgr, 256, 
		QSE_SIZEOF(qse_byte_t), QSE_NULL) <= -1) 
	{
		qse_str_fini (&stc->method_name);
		qse_str_fini (&stc->token.name);
		return QSE_NULL;
	}

	stc->stx = stx;
	stc->error_code = QSE_STC_ERROR_NONE;

	stc->temporary_count = 0;
	stc->argument_count = 0;
	stc->literal_count = 0;

	stc->curc = QSE_CHAR_EOF;
	stc->ungotc_count = 0;

	stc->input_owner = QSE_NULL;
	stc->input_func = QSE_NULL;
	return stc;
}

void qse_stc_fini (qse_stc_t* stc)
{
	while (stc->temporary_count > 0) 
		QSE_MMGR_FREE (stc->mmgr, stc->temporaries[--stc->temporary_count]);
	stc->argument_count = 0;

	qse_lda_fini (&stc->bytecode);
	qse_str_fini (&stc->token.name);
	qse_str_fini (&stc->method_name);
}

#define GET_CHAR(stc) \
	do { if (__get_char(stc) == -1) return -1; } while (0)
#define UNGET_CHAR(stc,c) \
	do { if (__unget_char(stc,c) == -1) return -1; } while (0)
#define GET_TOKEN(stc) \
	do { if (__get_token(stc) == -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(stc,c) \
	do {  \
		if (qse_str_ccat (&(stc)->token.name, c) == -1) { \
			(stc)->error_code = QSE_STC_ERROR_MEMORY; \
			return -1; \
		} \
	} while (0)

#define CLEAR_TOKEN(stc) QSE_BLOCK ( \
	qse_str_clear (&(stc)->token.name); \
	stc->token.type = TOKEN_END; \
)

#define YIELD_TOKEN_TO(stc,var) QSE_BLOCK ( \
	var = qse_str_yieldptr (&(stc)->token.name, 0); \
)
	
const qse_char_t* qse_stc_error_string (qse_stc_t* stc)
{
	static const qse_char_t* msg[] =
	{
		QSE_T("no error"),

		QSE_T("input fucntion not ready"),
		QSE_T("input function error"),
		QSE_T("out of memory"),

		QSE_T("invalid character"),
		QSE_T("incomplete character literal"),
		QSE_T("incomplete string literal"),
		QSE_T("incomplete literal"),

		QSE_T("message selector"),
		QSE_T("invalid argument name"),
		QSE_T("too many arguments"),

		QSE_T("invalid primitive type"),
		QSE_T("primitive number expected"),
		QSE_T("primitive number out of range"),
		QSE_T("primitive not closed"),

		QSE_T("temporary list not closed"),
		QSE_T("too many temporaries"),
		QSE_T("cannot redefine pseudo variable"),
		QSE_T("invalid primary/expression-start"),

		QSE_T("no period at end of statement"),
		QSE_T("no closing parenthesis"),
		QSE_T("block argument name missing"),
		QSE_T("block argument list not closed"),
		QSE_T("block not closed"),

		QSE_T("undeclared name"),
		QSE_T("too many literals")
	};

	if (stc->error_code >= 0 && 
	    stc->error_code < QSE_COUNTOF(msg)) return msg[stc->error_code];

	return QSE_T("unknown error");
}

static QSE_INLINE int __is_token_pseudo_variable (qse_stc_t* stc)
{
	return 
		stc->token.type == TOKEN_IDENT &&
		(qse_strcmp(QSE_STR_PTR(&stc->token.name), QSE_T("self")) == 0 ||
		 qse_strcmp(QSE_STR_PTR(&stc->token.name), QSE_T("super")) == 0 ||
		 qse_strcmp(QSE_STR_PTR(&stc->token.name), QSE_T("nil")) == 0 ||
		 qse_strcmp(QSE_STR_PTR(&stc->token.name), QSE_T("true")) == 0 ||
		 qse_strcmp(QSE_STR_PTR(&stc->token.name), QSE_T("false")) == 0);
}

static QSE_INLINE int __is_token_vertical_bar (qse_stc_t* stc)
{
	return 
		stc->token.type == TOKEN_BINARY &&
		QSE_STR_LEN(&stc->token.name) == 1 &&
		QSE_STR_CHAR(&stc->token.name,0) == QSE_T('|');
}

static QSE_INLINE int __is_token_primitive_opener (qse_stc_t* stc)
{
	return
		stc->token.type == TOKEN_BINARY &&
		QSE_STR_LEN(&stc->token.name) == 1 &&
		QSE_STR_CHAR(&stc->token.name,0) == QSE_T('<');
}

static QSE_INLINE int __is_token_primitive_closer (qse_stc_t* stc)
{
	return
		stc->token.type == TOKEN_BINARY &&
		QSE_STR_LEN(&stc->token.name) == 1 &&
		QSE_STR_CHAR(&stc->token.name,0) == QSE_T('>');
}

static QSE_INLINE qse_bool_t __is_binary_char (qse_cint_t c)
{
	/*
	 * binaryCharacter ::=
	 * 	'!' | '%' | '&' | '*' | '+' | ',' | 
	 * 	'/' | '<' | '=' | '>' | '?' | '@' | 
	 * 	'\' | '~' | '|' | '-'
	 */

	return
		c == QSE_T('!') || c == QSE_T('%') ||
		c == QSE_T('&') || c == QSE_T('*') ||
		c == QSE_T('+') || c == QSE_T(',') ||
		c == QSE_T('/') || c == QSE_T('<') ||
		c == QSE_T('=') || c == QSE_T('>') ||
		c == QSE_T('?') || c == QSE_T('@') ||
		c == QSE_T('\\') || c == QSE_T('|') ||
		c == QSE_T('~') || c == QSE_T('-');
}

static QSE_INLINE qse_bool_t __is_closing_char (qse_cint_t c)
{
	return 
		c == QSE_T('.') || c == QSE_T(']') ||
		c == QSE_T(')') || c == QSE_T(';') ||
		c == QSE_T('\"') || c == QSE_T('\'');
}

#define EMIT_CODE_TEST(stc,high,low) \
	do { if (__emit_code_test(stc,high,low) == -1) return -1; } while (0)

#define EMIT_CODE(stc,code) \
	do { if (__emit_code(stc,code) == -1) return -1; } while(0)

#define EMIT_PUSH_RECEIVER_VARIABLE(stc,pos) \
	do {  \
		if (__emit_stack_positional ( \
			stc, PUSH_RECEIVER_VARIABLE, pos) == -1) return -1; \
	} while (0)

#define EMIT_PUSH_TEMPORARY_LOCATION(stc,pos) \
	do {  \
		if (__emit_stack_positional ( \
			stc, PUSH_TEMPORARY_LOCATION, pos) == -1) return -1; \
	} while (0)

#define EMIT_PUSH_LITERAL_CONSTANT(stc,pos) \
	do { \
		if (__emit_stack_positional ( \
			stc, PUSH_LITERAL_CONSTANT, pos) == -1) return -1; \
	} while (0)


#define EMIT_PUSH_LITERAL_VARIABLE(stc,pos) \
	do { \
		if (__emit_stack_positional ( \
			stc, PUSH_LITERAL_VARIABLE, pos) == -1) return -1; \
	} while (0)

#define EMIT_STORE_RECEIVER_VARIABLE(stc,pos) \
	do { \
		if (__emit_stack_positional ( \
			stc, STORE_RECEIVER_VARIABLE, pos) == -1) return -1; \
	} while (0)	

#define EMIT_STORE_TEMPORARY_LOCATION(stc,pos) \
	do { \
		if (__emit_stack_positional ( \
			stc, STORE_TEMPORARY_LOCATION, pos) == -1) return -1; \
	} while (0)


#define EMIT_POP_STACK_TOP(stc) EMIT_CODE(stc, POP_STACK_TOP)
#define EMIT_DUPLICATE_STACK_TOP(stc) EMIT_CODE(stc, DUPLICATE_STACK_TOP)
#define EMIT_PUSH_ACTIVE_CONTEXT(stc) EMIT_CODE(stc, PUSH_ACTIVE_CONTEXT)
#define EMIT_RETURN_FROM_MESSAGE(stc) EMIT_CODE(stc, RETURN_FROM_MESSAGE)
#define EMIT_RETURN_FROM_BLOCK(stc) EMIT_CODE(stc, RETURN_FROM_BLOCK)

#define EMIT_SEND_TO_SELF(stc,nargs,selector) \
	do { \
		if (__emit_send_to_self(stc,nargs,selector) == -1) return -1; \
	} while (0)

#define EMIT_SEND_TO_SUPER(stc,nargs,selector) \
	do { \
		if (__emit_send_to_super(stc,nargs,selector) == -1) return -1; \
	} while (0)

#define EMIT_DO_PRIMITIVE(stc,no) \
	do { if (__emit_do_primitive(stc,no) == -1) return -1; } while(0)

static QSE_INLINE int __emit_code_test (
	qse_stc_t* stc, const qse_char_t* high, const qse_char_t* low)
{
	qse_printf (QSE_T("CODE: %s %s\n"), high, low);
	return 0;
}

static QSE_INLINE int __emit_code (qse_stc_t* stc, qse_byte_t code)
{
	if (qse_lda_adddatum(&stc->bytecode, &code) == QSE_NULL) {
		stc->error_code = QSE_STC_ERROR_MEMORY;
		return -1;
	}

	return 0;
}

static QSE_INLINE int __emit_stack_positional (
	qse_stc_t* stc, int opcode, int pos)
{
	qse_assert (pos >= 0x0 && pos <= 0xFF);

	if (pos <= 0x0F) {
		EMIT_CODE (stc, (opcode & 0xF0) | (pos & 0x0F));
	}
	else {
		EMIT_CODE (stc, (opcode >> 4) & 0x6F);
		EMIT_CODE (stc, pos & 0xFF);
	}

	return 0;
}

static QSE_INLINE int __emit_send_to_self (
	qse_stc_t* stc, int nargs, int selector)
{
	qse_assert (nargs >= 0x00 && nargs <= 0xFF);
	qse_assert (selector >= 0x00 && selector <= 0xFF);

	if (nargs <= 0x08 && selector <= 0x1F) {
		EMIT_CODE (stc, SEND_TO_SELF);
		EMIT_CODE (stc, (nargs << 5) | selector);
	}
	else {
		EMIT_CODE (stc, SEND_TO_SELF_EXTENDED);
		EMIT_CODE (stc, nargs);
		EMIT_CODE (stc, selector);
	}

	return 0;
}

static QSE_INLINE int __emit_send_to_super (
	qse_stc_t* stc, int nargs, int selector)
{
	qse_assert (nargs >= 0x00 && nargs <= 0xFF);
	qse_assert (selector >= 0x00 && selector <= 0xFF);

	if (nargs <= 0x08 && selector <= 0x1F) {
		EMIT_CODE (stc, SEND_TO_SUPER);
		EMIT_CODE (stc, (nargs << 5) | selector);
	}
	else {
		EMIT_CODE (stc, SEND_TO_SUPER_EXTENDED);
		EMIT_CODE (stc, nargs);
		EMIT_CODE (stc, selector);
	}

	return 0;
}

static QSE_INLINE int __emit_do_primitive (qse_stc_t* stc, int no)
{
	qse_assert (no >= 0x0 && no <= 0xFFF);

	EMIT_CODE (stc, DO_PRIMITIVE | ((no >> 8) & 0x0F));
	EMIT_CODE (stc, no & 0xFF);

	return 0;
}

static int __add_literal (qse_stc_t* stc, qse_word_t literal)
{
	qse_word_t i;

	for (i = 0; i < stc->literal_count; i++) {
		/* 
		 * it would remove redundancy of symbols and small integers. 
		 * more complex redundacy check may be done somewhere else 
		 * like in __add_string_literal.
		 */
		if (stc->literals[i] == literal) return i;
	}

	if (stc->literal_count >= QSE_COUNTOF(stc->literals)) {
		stc->error_code = QSE_STC_ERROR_TOO_MANY_LITERALS;
		return -1;
	}

	stc->literals[stc->literal_count++] = literal;
	return stc->literal_count - 1;
}

static int __add_character_literal (qse_stc_t* stc, qse_char_t ch)
{
	qse_word_t i, c, literal;
	qse_stx_t* stx = stc->stx;

	for (i = 0; i < stc->literal_count; i++) 
	{
		c = QSE_STX_ISSMALLINT(stc->literals[i])? 
			stx->class_smallinteger: QSE_STX_CLASS (stx, stc->literals[i]);
		if (c != stx->class_character) continue;

		if (ch == QSE_STX_CHAR_AT(stx,stc->literals[i],0)) return i;
	}

	literal = qse_stx_instantiate (
		stx, stx->class_character, &ch, QSE_NULL, 0);
	return __add_literal (stc, literal);
}

static int __add_string_literal (
	qse_stc_t* stc, const qse_char_t* str, qse_word_t size)
{
	qse_word_t i, c, literal;
	qse_stx_t* stx = stc->stx;

	for (i = 0; i < stc->literal_count; i++) {
		c = QSE_STX_ISSMALLINT(stc->literals[i])? 
			stx->class_smallinteger: QSE_STX_CLASS (stx, stc->literals[i]);
		if (c != stx->class_string) continue;

		if (qse_strxncmp (str, size, 
			QSE_STX_DATA(stx,stc->literals[i]), 
			QSE_STX_SIZE(stx,stc->literals[i])) == 0) return i;
	}

	literal = qse_stx_instantiate (
		stx, stx->class_string, QSE_NULL, str, size);
	return __add_literal (stc, literal);
}

static int __add_symbol_literal (
	qse_stc_t* stc, const qse_char_t* str, qse_word_t size)
{
	qse_stx_t* stx = stc->stx;
	return __add_literal (stc, qse_stx_new_symbolx(stx, str, size));
}

int qse_stc_parse_method (
	qse_stc_t* stc, qse_word_t method_class, void* input)
{
	int n;

	if (stc->input_func == QSE_NULL) { 
		stc->error_code = QSE_STC_ERROR_INPUT_FUNC;
		return -1;
	}

	stc->method_class = method_class;
	if (__open_input(stc, input) == -1) return -1;
	n = __parse_method (stc, method_class, input);
	if (__close_input(stc) == -1) return -1;

	return n;
}

static int __parse_method (
	qse_stc_t* stc, qse_word_t method_class, void* input)
{
	/*
	 * <method definition> ::= 
	 * 	<message pattern> [<temporaries>] [<primitive>] [<statements>]
	 */

	GET_CHAR (stc);
	GET_TOKEN (stc);

	qse_stx_name_clear (&stc->method_name);
	qse_lda_clear (&stc->bytecode);

	while (stc->temporary_count > 0) {
		qse_free (stc->temporaries[--stc->temporary_count]);
	}
	stc->argument_count = 0;
	stc->literal_count = 0;

	if (__parse_message_pattern(stc) == -1) return -1;
	if (__parse_temporaries(stc) == -1) return -1;
	if (__parse_primitive(stc) == -1) return -1;
	if (__parse_statements(stc) == -1) return -1;
	if (__finish_method (stc) == -1) return -1;

	return 0;
}

static int __finish_method (qse_stc_t* stc)
{
	qse_stx_t* stx = stc->stx;
	qse_stx_class_t* class_obj;
	qse_stx_method_t* method_obj;
	qse_word_t method, selector;

	qse_assert (stc->bytecode.size != 0);

	class_obj = (qse_stx_class_t*)
		QSE_STX_OBJPTR(stx, stc->method_class);

	if (class_obj->methods == stx->nil) {
		/* TODO: reconfigure method dictionary size */
		class_obj->methods = qse_stx_instantiate (
			stx, stx->class_system_dictionary, 
			QSE_NULL, QSE_NULL, 64);
	}
	qse_assert (class_obj->methods != stx->nil);

	selector = qse_stx_new_symbolx (
		stx, stc->method_name.buffer, stc->method_name.size);

	method = qse_stx_instantiate(stx, stx->class_method, 
		QSE_NULL, stc->literals, stc->literal_count);
	method_obj = (qse_stx_method_t*)QSE_STX_OBJPTR(stx, method);

	/* TODO: text saving must be optional */
	/*method_obj->text = qse_stx_instantiate (
		stx, stx->class_string, QSE_NULL, 
		stc->text, qse_strlen(stc->text));
	*/
	method_obj->selector = selector;
	method_obj->bytecodes = qse_stx_instantiate (
		stx, stx->class_bytearray, QSE_NULL, 
		stc->bytecode.buf, stc->bytecode.size);

	/* TODO: better way to store argument count & temporary count */
	method_obj->tmpcount = 
		QSE_STX_TO_SMALLINT(stc->temporary_count - stc->argument_count);
	method_obj->argcount = QSE_STX_TO_SMALLINT(stc->argument_count);

	qse_stx_dict_put (stx, class_obj->methods, selector, method);
	return 0;
}

static int __parse_message_pattern (qse_stc_t* stc)
{
	/* 
	 * <message pattern> ::= 
	 * 	<unary pattern> | <binary pattern> | <keyword pattern>
	 * <unary pattern> ::= unarySelector
	 * <binary pattern> ::= binarySelector <method argument>
	 * <keyword pattern> ::= (keyword  <method argument>)+
	 */
	int n;

	if (stc->token.type == TOKEN_IDENT) { 
		n = __parse_unary_pattern (stc);
	}
	else if (stc->token.type == TOKEN_BINARY) { 
		n = __parse_binary_pattern (stc);
	}
	else if (stc->token.type == TOKEN_KEYWORD) { 
		n = __parse_keyword_pattern (stc);
	}
	else {
		stc->error_code = QSE_STC_ERROR_MESSAGE_SELECTOR;
		n = -1;
	}

	stc->temporary_count = stc->argument_count;
	return n;
}

static int __parse_unary_pattern (qse_stc_t* stc)
{
	/* TODO: check if the method name exists */

	if (qse_str_cat (
		&stc->method_name, QSE_STR_PTR(&stc->token.name)) == (qse_size_t)-1) {
		stc->error_code = QSE_STC_ERROR_MEMORY;
		return -1;
	}

	GET_TOKEN (stc);
	return 0;
}

static int __parse_binary_pattern (qse_stc_t* stc)
{
	/* TODO: check if the method name exists */

	if (qse_str_cat (
		&stc->method_name,
		QSE_STR_PTR(&stc->token.name)) == (qse_size_t)-1) 
	{
		stc->error_code = QSE_STC_ERROR_MEMORY;
		return -1;
	}

	GET_TOKEN (stc);
	if (stc->token.type != TOKEN_IDENT) 
	{
		stc->error_code = QSE_STC_ERROR_ARGUMENT_NAME;
		return -1;
	}

	if (stc->argument_count >= QSE_COUNTOF(stc->temporaries)) 
	{
		stc->error_code = QSE_STC_ERROR_TOO_MANY_ARGUMENTS;
		return -1;
	}

	/* TODO: check for duplicate entries...in instvars */
	YIELD_TOKEN_TO (stc, stc->temporaries[stc->argument_count]); 
	if (stc->temporaries[stc->argument_count] == QSE_NULL) 
	{
		stc->error_code = QSE_STC_ERROR_MEMORY;
		return -1;
	}
	stc->argument_count++;

	GET_TOKEN (stc);
	return 0;
}

static int __parse_keyword_pattern (qse_stc_t* stc)
{
	do 
	{
		if (qse_str_cat (
			&stc->method_name, QSE_STR_PTR(&stc->token.name)) == (qse_size_t)-1) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (stc);
		if (stc->token.type != TOKEN_IDENT) 
		{
			stc->error_code = QSE_STC_ERROR_ARGUMENT_NAME;
			return -1;
		}

		if (__is_token_pseudo_variable(stc)) 
		{
			stc->error_code = QSE_STC_ERROR_PSEUDO_VARIABLE;
			return -1;
		}

		if (stc->argument_count >= QSE_COUNTOF(stc->temporaries)) 
		{
			stc->error_code = QSE_STC_ERROR_TOO_MANY_ARGUMENTS;
			return -1;
		}

		YIELD_TOKEN_TO (stc, stc->temporaries[stc->argument_count]);
		if (stc->temporaries[stc->argument_count] == QSE_NULL) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments */
		stc->argument_count++;

		GET_TOKEN (stc);
	} 
	while (stc->token.type == TOKEN_KEYWORD);

	/* TODO: check if the method name exists */
	/* if it exists, collapse arguments */
qse_printf (QSE_T("METHOD NAME ==> [%s]\n"), stc->method_name.buffer);

	return 0;
}

static int __parse_temporaries (qse_stc_t* stc)
{
	/* 
	 * <temporaries> ::= '|' <temporary variable list> '|'
	 * <temporary variable list> ::= identifier*
	 */

	if (!__is_token_vertical_bar(stc)) return 0;

	GET_TOKEN (stc);
	while (stc->token.type == TOKEN_IDENT) {
		if (stc->temporary_count >= QSE_COUNTOF(stc->temporaries)) {
			stc->error_code = QSE_STC_ERROR_TOO_MANY_TEMPORARIES;
			return -1;
		}

		if (__is_token_pseudo_variable(stc)) {
			stc->error_code = QSE_STC_ERROR_PSEUDO_VARIABLE;
			return -1;
		}

		YIELD_TOKEN_TO (stc, stc->temporaries[stc->temporary_count]);
		if (stc->temporaries[stc->temporary_count] == QSE_NULL) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments/temporaries */
		stc->temporary_count++;

		GET_TOKEN (stc);
	}
	if (!__is_token_vertical_bar(stc)) {
		stc->error_code = QSE_STC_ERROR_TEMPORARIES_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (stc);
	return 0;
}

static int __parse_primitive (qse_stc_t* stc)
{
	/* 
	 * <primitive> ::= '<' 'primitive:' number '>'
	 */

	int prim_no;

	if (!__is_token_primitive_opener(stc)) return 0;
	GET_TOKEN (stc);

	if (stc->token.type != TOKEN_KEYWORD ||
	    qse_strcmp (QSE_STR_PTR(&stc->token.name), QSE_T("primitive:")) != 0) 
	{
		stc->error_code = QSE_STC_ERROR_PRIMITIVE_KEYWORD;
		return -1;
	}

	GET_TOKEN (stc); /* TODO: only integer */
	if (stc->token.type != TOKEN_NUMLIT) 
	{
		stc->error_code = QSE_STC_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

/*TODO: more checks the validity of the primitive number */
	if (!qse_stristype(QSE_STR_PTR(&stc->token.name), qse_isdigit)) 
	{
		stc->error_code = QSE_STC_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

	QSE_STRTOI (prim_no, QSE_STR_PTR(&stc->token.name), QSE_NULL, 10);
	if (prim_no < 0 || prim_no > 0xFF) 
	{
		stc->error_code = QSE_STC_ERROR_PRIMITIVE_NUMBER_RANGE;
		return -1;
	}

	EMIT_DO_PRIMITIVE (stc, prim_no);

	GET_TOKEN (stc);
	if (!__is_token_primitive_closer(stc)) 
	{
		stc->error_code = QSE_STC_ERROR_PRIMITIVE_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (stc);
	return 0;
}

static int __parse_statements (qse_stc_t* stc)
{
	/*
	 * <statements> ::= (ORIGINAL->maybe wrong)
	 * 	(<return statement> ['.'] ) |
	 * 	(<expression> ['.' [<statements>]])
	 * <statements> ::= (REVISED->correct?)
	 * 	<statement> ['. [<statements>]]
	 */

	while (stc->token.type != TOKEN_END) {
		if (__parse_statement (stc) == -1) return -1;

		if (stc->token.type == TOKEN_PERIOD) {
			GET_TOKEN (stc);
			continue;
		}

		if (stc->token.type != TOKEN_END) {
			stc->error_code = QSE_STC_ERROR_NO_PERIOD;
			return -1;
		}
	}

	EMIT_CODE (stc, RETURN_RECEIVER);
	return 0;
}

static int __parse_block_statements (qse_stc_t* stc)
{
	while (stc->token.type != TOKEN_RBRACKET && 
	       stc->token.type != TOKEN_END) {

		if (__parse_statement(stc) == -1) return -1;
		if (stc->token.type != TOKEN_PERIOD) break;
		GET_TOKEN (stc);
	}

	return 0;
}

static int __parse_statement (qse_stc_t* stc)
{
	/* 
	 * <statement> ::= <return statement> | <expression>
	 * <return statement> ::= returnOperator <expression> 
	 * returnOperator ::= '^'
	 */

	if (stc->token.type == TOKEN_RETURN) {
		GET_TOKEN (stc);
		if (__parse_expression(stc) == -1) return -1;
		EMIT_RETURN_FROM_MESSAGE (stc);
	}
	else {
		if (__parse_expression(stc) == -1) return -1;
	}

	return 0;
}

static int __parse_expression (qse_stc_t* stc)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 * <assignment target> ::= identifier
	 * assignmentOperator ::=  ':='
	 */
	qse_stx_t* stx = stc->stx;

	if (stc->token.type == TOKEN_IDENT) 
	{
		qse_char_t* ident;

		YIELD_TOKEN_TO (stc, ident);
		if (ident == QSE_NULL) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (stc);
		if (stc->token.type == TOKEN_ASSIGN) 
		{
			GET_TOKEN (stc);
			if (__parse_assignment(stc, ident) == -1) 
			{
				qse_free (ident);
				return -1;
			}
		}
		else 
		{
			if (__parse_basic_expression(stc, ident) == -1) 
			{
				qse_free (ident);
				return -1;
			}
		}

		qse_free (ident);
	}
	else 
	{
		if (__parse_basic_expression(stc, QSE_NULL) == -1) return -1;
	}

	return 0;
}

static int __parse_basic_expression (
	qse_stc_t* stc, const qse_char_t* ident)
{
	/*
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 */
	qse_bool_t is_super;

	if (__parse_primary(stc, ident, &is_super) == -1) return -1;
	if (stc->token.type != TOKEN_END &&
	    stc->token.type != TOKEN_PERIOD) {
		if (__parse_message_continuation(stc, is_super) == -1) return -1;
	}
	return 0;
}

static int __parse_assignment (
	qse_stc_t* stc, const qse_char_t* target)
{
	/*
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 */

	qse_word_t i;
	qse_stx_t* stx = stc->stx;

	for (i = stc->argument_count; i < stc->temporary_count; i++) {
		if (qse_strcmp (target, stc->temporaries[i]) == 0) {
			if (__parse_expression(stc) == -1) return -1;
			EMIT_STORE_TEMPORARY_LOCATION (stc, i);
			return 0;
		}
	}

	if (qse_stx_get_instance_variable_index (
		stx, stc->method_class, target, &i) == 0) {
		if (__parse_expression(stc) == -1) return -1;
		EMIT_STORE_RECEIVER_VARIABLE (stc, i);
		return 0;
	}

	if (qse_stx_lookup_class_variable (
		stx, stc->method_class, target) != stx->nil) {
		if (__parse_expression(stc) == -1) return -1;

		/* TODO */
		EMIT_CODE_TEST (stc, QSE_T("ASSIGN_CLASSVAR #"), target);
		//EMIT_STORE_CLASS_VARIABLE (stc, target);
		return 0;
	}

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	stc->error_code = QSE_STC_ERROR_UNDECLARED_NAME;
	return -1;
}

static int __parse_primary (
	qse_stc_t* stc, const qse_char_t* ident, qse_bool_t* is_super)
{
	/*
	 * <primary> ::=
	 * 	identifier | <literal> | 
	 * 	<block constructor> | ( '('<expression>')' )
	 */

	qse_stx_t* stx = stc->stx;

	if (ident == QSE_NULL) {
		int pos;
		qse_word_t literal;

		*is_super = qse_false;

		if (stc->token.type == TOKEN_IDENT) {
			if (__parse_primary_ident(stc, 
				QSE_STR_PTR(&stc->token.name), is_super) == -1) return -1;
			GET_TOKEN (stc);
		}
		else if (stc->token.type == TOKEN_CHARLIT) {
			pos = __add_character_literal(
				stc, QSE_STR_PTR(&stc->token.name)[0]);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (stc, pos);
			GET_TOKEN (stc);
		}
		else if (stc->token.type == TOKEN_STRLIT) {
			pos = __add_string_literal (stc,
				QSE_STR_PTR(&stc->token.name), QSE_STR_LEN(&stc->token.name));
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (stc, pos);
			GET_TOKEN (stc);
		}
		else if (stc->token.type == TOKEN_NUMLIT) {
			/* TODO: other types of numbers, negative numbers, etc */
			qse_word_t tmp;
			QSE_STRTOI (tmp, QSE_STR_PTR(&stc->token.name), QSE_NULL, 10);
			literal = QSE_STX_TO_SMALLINT(tmp);
			pos = __add_literal(stc, literal);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (stc, pos);
			GET_TOKEN (stc);
		}
		else if (stc->token.type == TOKEN_SYMLIT) {
			pos = __add_symbol_literal (stc,
				QSE_STR_PTR(&stc->token.name), QSE_STR_LEN(&stc->token.name));
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (stc, pos);
			GET_TOKEN (stc);
		}
		else if (stc->token.type == TOKEN_LBRACKET) {
			GET_TOKEN (stc);
			if (__parse_block_constructor(stc) == -1) return -1;
		}
		else if (stc->token.type == TOKEN_APAREN) {
			/* TODO: array literal */
		}
		else if (stc->token.type == TOKEN_LPAREN) {
			GET_TOKEN (stc);
			if (__parse_expression(stc) == -1) return -1;
			if (stc->token.type != TOKEN_RPAREN) {
				stc->error_code = QSE_STC_ERROR_NO_RPAREN;
				return -1;
			}
			GET_TOKEN (stc);
		}
		else {
			stc->error_code = QSE_STC_ERROR_PRIMARY;
			return -1;
		}
	}
	else {
		/*if (__parse_primary_ident(stc, QSE_STR_PTR(&stc->token.name)) == -1) return -1;*/
		if (__parse_primary_ident(stc, ident, is_super) == -1) return -1;
	}

	return 0;
}

static int __parse_primary_ident (
	qse_stc_t* stc, const qse_char_t* ident, qse_bool_t* is_super)
{
	qse_word_t i;
	qse_stx_t* stx = stc->stx;

	*is_super = qse_false;

	if (qse_strcmp(ident, QSE_T("self")) == 0) 
	{
		EMIT_CODE (stc, PUSH_RECEIVER);
		return 0;
	}
	else if (qse_strcmp(ident, QSE_T("super")) == 0) 
	{
		*is_super = qse_true;
		EMIT_CODE (stc, PUSH_RECEIVER);
		return 0;
	}
	else if (qse_strcmp(ident, QSE_T("nil")) == 0) 
	{
		EMIT_CODE (stc, PUSH_NIL);
		return 0;
	}
	else if (qse_strcmp(ident, QSE_T("true")) == 0) 
	{
		EMIT_CODE (stc, PUSH_TRUE);
		return 0;
	}
	else if (qse_strcmp(ident, QSE_T("false")) == 0) 
	{
		EMIT_CODE (stc, PUSH_FALSE);
		return 0;
	}

	/* Refer to __parse_assignment for identifier lookup */

	for (i = 0; i < stc->temporary_count; i++) 
	{
		if (qse_strcmp(ident, stc->temporaries[i]) == 0) 
		{
			EMIT_PUSH_TEMPORARY_LOCATION (stc, i);
			return 0;
		}
	}

	if (qse_stx_get_instance_variable_index (
		stx, stc->method_class, ident, &i) == 0) 
	{
		EMIT_PUSH_RECEIVER_VARIABLE (stc, i);
		return 0;
	}	

	/* TODO: what is the best way to look up a class variable? */
	/* 1. Use the class containing it and using its position */
	/* 2. Use a primitive method after pushing the name as a symbol */
	/* 3. Implement a vm instruction to do it */
/*
	if (qse_stx_lookup_class_variable (
		stx, stc->method_class, ident) != stx->nil) {
		//EMIT_LOOKUP_CLASS_VARIABLE (stc, ident);
		return 0;
	}
*/

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	stc->error_code = QSE_STC_ERROR_UNDECLARED_NAME;
	return -1;
}

static int __parse_block_constructor (qse_stc_t* stc)
{
	/*
	 * <block constructor> ::= '[' <block body> ']'
	 * <block body> ::= [<block argument>* '|']
	 * 	[<temporaries>] [<statements>]
	 * <block argument> ::= ':'  identifier
	 */

	if (stc->token.type == TOKEN_COLON) 
	{
		do 
		{
			GET_TOKEN (stc);

			if (stc->token.type != TOKEN_IDENT) 
			{
				stc->error_code = QSE_STC_ERROR_BLOCK_ARGUMENT_NAME;
				return -1;
			}

			/* TODO : store block arguments */
			GET_TOKEN (stc);
		} 
		while (stc->token.type == TOKEN_COLON);
			
		if (!__is_token_vertical_bar(stc)) 
		{
			stc->error_code = QSE_STC_ERROR_BLOCK_ARGUMENT_LIST;
			return -1;
		}

		GET_TOKEN (stc);
	}

	/* TODO: create a block closure */
	if (__parse_temporaries(stc) == -1) return -1;
	if (__parse_block_statements(stc) == -1) return -1;

	if (stc->token.type != TOKEN_RBRACKET) 
	{
		stc->error_code = QSE_STC_ERROR_BLOCK_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (stc);

	/* TODO: do special treatment for block closures */

	return 0;
}

static int __parse_message_continuation (
	qse_stc_t* stc, qse_bool_t is_super)
{
	/*
	 * <messages> ::=
	 * 	(<unary message>+ <binary message>* [<keyword message>] ) |
	 * 	(<binary message>+ [<keyword message>] ) |
	 * 	<keyword message>
	 * <cascaded messages> ::= (';' <messages>)*
	 */
	if (__parse_keyword_message(stc, is_super) == -1) return -1;

	while (stc->token.type == TOKEN_SEMICOLON) 
	{
		EMIT_CODE_TEST (stc, QSE_T("DoSpecial(DUP_RECEIVER(CASCADE))"), QSE_T(""));
		GET_TOKEN (stc);

		if (__parse_keyword_message(stc, qse_false) == -1) return -1;
		EMIT_CODE_TEST (stc, QSE_T("DoSpecial(POP_TOP)"), QSE_T(""));
	}

	return 0;
}

static int __parse_keyword_message (qse_stc_t* stc, qse_bool_t is_super)
{
	/*
	 * <keyword message> ::= (keyword <keyword argument> )+
	 * <keyword argument> ::= <primary> <unary message>* <binary message>*
	 */
	qse_str_t name;
	qse_word_t pos;
	qse_bool_t is_super2;
	int nargs = 0, n;

	if (__parse_binary_message (stc, is_super) == -1) return -1;
	if (stc->token.type != TOKEN_KEYWORD) return 0;

	if (qse_str_init (&name, stc->mmgr, 0) <= -1) 
	{
		stc->error_code = QSE_STC_ERROR_MEMORY;
		return -1;
	}
	
	do 
	{
		if (qse_str_cat (&name, QSE_STR_PTR(&stc->token.name)) == (qse_size_t)-1) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			goto oops;
		}

		if (__get_token(stc) == -1) goto oops;
		if (__parse_primary(stc, QSE_NULL, &is_super2) == -1) goto oops;
		if (__parse_binary_message(stc, is_super2) == -1) goto oops;

		nargs++;
		/* TODO: check if it has too many arguments.. */
	} 
	while (stc->token.type == TOKEN_KEYWORD);

	pos = __add_symbol_literal (stc, name.buffer, name.size);
	if (pos <= -1) goto oops;

	n = (is_super)?
		__emit_send_to_super(stc,nargs,pos):
		__emit_send_to_self(stc,nargs,pos);
	if (n <= -1) goto oops;

	qse_str_fini (&name);
	return 0;

oops:
	qse_str_fini (&name);
	return -1;
}

static int __parse_binary_message (qse_stc_t* stc, qse_bool_t is_super)
{
	/*
	 * <binary message> ::= binarySelector <binary argument>
	 * <binary argument> ::= <primary> <unary message>*
	 */
	qse_word_t pos;
	qse_bool_t is_super2;
	int n;

	if (__parse_unary_message (stc, is_super) == -1) return -1;

	while (stc->token.type == TOKEN_BINARY) 
	{

		YIELD_TOKEN_TO (stc, op);
		if (op == QSE_NULL) 
		{
			stc->error_code = QSE_STC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (stc);
		if (__parse_primary(stc, QSE_NULL, &is_super2) == -1) 
		{
			qse_free (op);
			return -1;
		}

		if (__parse_unary_message(stc, is_super2) == -1) 
		{
			qse_free (op);
			return -1;
		}

		pos = __add_symbol_literal (stc, op, qse_strlen(op));
		if (pos == -1) 
		{
			qse_free (op);
			return -1;
		}

		n = (is_super)?
			__emit_send_to_super(stc,2,pos):
			__emit_send_to_self(stc,2,pos);
		if (n == -1) 
		{
			qse_free (op);
			return -1;
		}

		qse_free (op);
	}

	return 0;
}

static int __parse_unary_message (qse_stc_t* stc, qse_bool_t is_super)
{
	/* <unary message> ::= unarySelector */

	qse_word_t pos;
	int n;

	while (stc->token.type == TOKEN_IDENT) 
	{
		pos = __add_symbol_literal (stc,
			QSE_STR_PTR(&stc->token.name),
			QSE_STR_LEN(&stc->token.name));
		if (pos == -1) return -1;

		n = (is_super)?
			__emit_send_to_super(stc,0,pos):
			__emit_send_to_self(stc,0,pos);
		if (n == -1) return -1;

		GET_TOKEN (stc);
	}

	return 0;
}

static int __get_token (qse_stc_t* stc)
{
	qse_cint_t c;

	do {
		if (__skip_spaces(stc) == -1) return -1;
		if (stc->curc == QSE_T('"')) {
			GET_CHAR (stc);
			if (__skip_comment(stc) == -1) return -1;
		}
		else break;
	} while (1);

	c = stc->curc;

	CLEAR_TOKEN ();

	if (c == QSE_CHAR_EOF) {
		stc->token.type = TOKEN_END;
	}
	else if (qse_isalpha(c)) {
		if (__get_ident(stc) == -1) return -1;
	}
	else if (qse_isdigit(c)) {
		if (__get_numlit(stc, qse_false) == -1) return -1;
	}
	else if (c == QSE_T('$')) {
		GET_CHAR (stc);
		if (__get_charlit(stc) == -1) return -1;
	}
	else if (c == QSE_T('\'')) {
		GET_CHAR (stc);
		if (__get_strlit(stc) == -1) return -1;
	}
	else if (c == QSE_T(':')) {
		stc->token.type = TOKEN_COLON;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);

		c = stc->curc;
		if (c == QSE_T('=')) {
			stc->token.type = TOKEN_ASSIGN;
			ADD_TOKEN_CHAR(stc, c);
			GET_CHAR (stc);
		}
	}
	else if (c == QSE_T('^')) {
		stc->token.type = TOKEN_RETURN;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T('[')) {
		stc->token.type = TOKEN_LBRACKET;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T(']')) {
		stc->token.type = TOKEN_RBRACKET;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T('(')) {
		stc->token.type = TOKEN_LPAREN;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T(')')) {
		stc->token.type = TOKEN_RPAREN;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T('#')) {
		/*ADD_TOKEN_CHAR(stc, c);*/
		GET_CHAR (stc);

		c = stc->curc;
		if (c == QSE_CHAR_EOF) {
			stc->error_code = QSE_STC_ERROR_LITERAL;
			return -1;
		}
		else if (c == QSE_T('(')) {
			ADD_TOKEN_CHAR(stc, c);
			stc->token.type = TOKEN_APAREN;
			GET_CHAR (stc);
		}
		else if (c == QSE_T('\'')) {
			GET_CHAR (stc);
			if (__get_strlit(stc) == -1) return -1;
			stc->token.type = TOKEN_SYMLIT;
		}
		else if (!__is_closing_char(c) && !qse_isspace(c)) {
			do {
				ADD_TOKEN_CHAR(stc, c);
				GET_CHAR (stc);
				c = stc->curc;
			} while (!__is_closing_char(c) && !qse_isspace(c));

			stc->token.type = TOKEN_SYMLIT;
		}
		else {
			stc->error_code = QSE_STC_ERROR_LITERAL;
			return -1;
		}
	}
	else if (c == QSE_T('.')) {
		stc->token.type = TOKEN_PERIOD;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (c == QSE_T(';')) {
		stc->token.type = TOKEN_SEMICOLON;
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
	}
	else if (__is_binary_char(c)) {
		if (__get_binary(stc) == -1) return -1;
	}
	else {
		stc->error_code = QSE_STC_ERROR_CHAR;
		return -1;
	}

//qse_printf (QSE_T("TOKEN: %s\n"), QSE_STR_PTR(&stc->token.name));
	return 0;
}

static int __get_ident (qse_stc_t* stc)
{
	/*
	 * identifier ::= letter (letter | digit)*
	 * keyword ::= identifier ':'
	 */

	qse_cint_t c = stc->curc;
	stc->token.type = TOKEN_IDENT;

	do {
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
		c = stc->curc;
	} while (qse_isalnum(c));

	if (c == QSE_T(':')) {
		ADD_TOKEN_CHAR (stc, c);
		stc->token.type = TOKEN_KEYWORD;
		GET_CHAR (stc);
	}

	return 0;
}

static int __get_numlit (qse_stc_t* stc, qse_bool_t negated)
{
	/* 
	 * <number literal> ::= ['-'] <number>
	 * <number> ::= integer | float | scaledDecimal
	 * integer ::= decimalInteger  | radixInteger
	 * decimalInteger ::= digits
	 * digits ::= digit+
	 * radixInteger ::= radixSpecifier  'r' radixDigits
	 * radixSpecifier := digits
	 * radixDigits ::= (digit | uppercaseAlphabetic)+
	 * float ::=  mantissa [exponentLetter exponent]
	 * mantissa ::= digits'.' digits
	 * exponent ::= ['-']decimalInteger
	 * exponentLetter ::= 'e' | 'd' | 'q'
	 * scaledDecimal ::= scaledMantissa 's' [fractionalDigits]
	 * scaledMantissa ::= decimalInteger | mantissa
	 * fractionalDigits ::= decimalInteger
	 */

	qse_cint_t c = stc->curc;
	stc->token.type = TOKEN_NUMLIT;

	do {
		ADD_TOKEN_CHAR(stc, c);
		GET_CHAR (stc);
		c = stc->curc;
	} while (qse_isalnum(c));

	/* TODO; more */
	return 0;
}

static int __get_charlit (qse_stc_t* stc)
{
	/* 
	 * character_literal ::= '$' character
	 * character ::= "Any character in the implementation-defined character set"
	 */

	qse_cint_t c = stc->curc; /* even a new-line or white space would be taken */
	if (c == QSE_CHAR_EOF) {
		stc->error_code = QSE_STC_ERROR_CHARLIT;
		return -1;
	}	

	stc->token.type = TOKEN_CHARLIT;
	ADD_TOKEN_CHAR(stc, c);
	GET_CHAR (stc);
	return 0;
}

static int __get_strlit (qse_stc_t* stc)
{
	/* 
	 * string_literal ::= stringDelimiter stringBody stringDelimiter
	 * stringBody ::= (nonStringDelimiter | (stringDelimiter stringDelimiter)*)
	 * stringDelimiter ::= '''    "a single quote"
	 */

	/* TODO: C-like string */

	qse_cint_t c = stc->curc;
	stc->token.type = TOKEN_STRLIT;

	do {
		do {
			ADD_TOKEN_CHAR (stc, c);
			GET_CHAR (stc);
			c = stc->curc;

			if (c == QSE_CHAR_EOF) {
				stc->error_code = QSE_STC_ERROR_STRLIT;
				return -1;
			}
		} while (c != QSE_T('\''));

		GET_CHAR (stc);
		c = stc->curc;
	} while (c == QSE_T('\''));

	return 0;
}

static int __get_binary (qse_stc_t* stc)
{
	/* 
	 * binarySelector ::= binaryCharacter+
	 */

	qse_cint_t c = stc->curc;
	ADD_TOKEN_CHAR (stc, c);

	if (c == QSE_T('-')) {
		GET_CHAR (stc);
		c = stc->curc;
		if (qse_isdigit(c)) return __get_numlit(stc,qse_true);
	}
	else {
		GET_CHAR (stc);
		c = stc->curc;
	}

	/* up to 2 characters only */
	if (__is_binary_char(c)) {
		ADD_TOKEN_CHAR (stc, c);
		GET_CHAR (stc);
		c = stc->curc;
	}

	/* or up to any occurrences */
	/*
	while (__is_binary_char(c)) {
		ADD_TOKEN_CHAR (stc, c);
		GET_CHAR (stc);
		c = stc->curc;
	}
	*/

	stc->token.type = TOKEN_BINARY;
	return 0;
}

static int __skip_spaces (qse_stc_t* stc)
{
	while (qse_isspace(stc->curc)) GET_CHAR (stc);
	return 0;
}

static int __skip_comment (qse_stc_t* stc)
{
	while (stc->curc != QSE_T('"')) GET_CHAR (stc);
	GET_CHAR (stc);
	return 0;
}

static int __get_char (qse_stc_t* stc)
{
	qse_cint_t c;

	if (stc->ungotc_count > 0) {
		stc->curc = stc->ungotc[stc->ungotc_count--];
	}
	else {
		if (stc->input_func (
			QSE_STC_INPUT_CONSUME, 
			stc->input_owner, (void*)&c) == -1) {
			stc->error_code = QSE_STC_ERROR_INPUT;
			return -1;
		}
		stc->curc = c;
	}
	return 0;
}

static int __unget_char (qse_stc_t* stc, qse_cint_t c)
{
	if (stc->ungotc_count >= QSE_COUNTOF(stc->ungotc)) return -1;
	stc->ungotc[stc->ungotc_count++] = c;
	return 0;
}

static int __open_input (qse_stc_t* stc, void* input)
{
	if (stc->input_func(
		QSE_STC_INPUT_OPEN, 
		(void*)&stc->input_owner, input) == -1) {
		stc->error_code = QSE_STC_ERROR_INPUT;
		return -1;
	}

	stc->error_code = QSE_STC_ERROR_NONE;
	stc->curc = QSE_CHAR_EOF;
	stc->ungotc_count = 0;
	return 0;
}

static int __close_input (qse_stc_t* stc)
{
	if (stc->input_func(
		QSE_STC_INPUT_CLOSE, 
		stc->input_owner, QSE_NULL) == -1) {
		stc->error_code = QSE_STC_ERROR_INPUT;
		return -1;
	}

	return 0;
}

#endif
