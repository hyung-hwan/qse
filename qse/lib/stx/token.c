/*
 * $Id: token.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/token.h>
#include <qse/stx/misc.h>

qse_stx_token_t* qse_stx_token_open (
	qse_stx_token_t* token, qse_word_t capacity)
{
	if (token == QSE_NULL) {
		token = (qse_stx_token_t*)
			qse_malloc (qse_sizeof(qse_stx_token_t));
		if (token == QSE_NULL) return QSE_NULL;
		token->__dynamic = qse_true;
	}
	else token->__dynamic = qse_false;
	
	if (qse_stx_name_open(&token->name, capacity) == QSE_NULL) {
		if (token->__dynamic) qse_free (token);
		return QSE_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type      = QSE_STX_TOKEN_END;
	return token;
}

void qse_stx_token_close (qse_stx_token_t* token)
{
	qse_stx_name_close (&token->name);
	if (token->__dynamic) qse_free (token);
}

int qse_stx_token_addc (qse_stx_token_t* token, qse_cint_t c)
{
	return qse_stx_name_addc (&token->name, c);
}

int qse_stx_token_adds (qse_stx_token_t* token, const qse_char_t* s)
{
	return qse_stx_name_adds (&token->name, s);
}

void qse_stx_token_clear (qse_stx_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = QSE_STX_TOKEN_END;
	qse_stx_name_clear (&token->name);
}

qse_char_t* qse_stx_token_yield (qse_stx_token_t* token, qse_word_t capacity)
{
	qse_char_t* p;

	p = qse_stx_name_yield (&token->name, capacity);
	if (p == QSE_NULL) return QSE_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = QSE_STX_TOKEN_END;
	return p;
}

int qse_stx_token_compare_name (qse_stx_token_t* token, const qse_char_t* str)
{
	return qse_stx_name_compare (&token->name, str);
}
