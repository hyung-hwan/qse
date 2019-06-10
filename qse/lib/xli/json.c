/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/xli/json.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "../cmn/mem-prv.h"

#define QSE_JSON_TOKEN_NAME_ALIGN 64

typedef struct qse_json_state_node_t qse_json_state_node_t;
struct qse_json_state_node_t
{
	qse_json_state_t state;
	union
	{
		struct
		{
			int got_value;
		} ia; /* in array */

		struct
		{
			/* 0: ready to get key (at the beginning or got comma), 
			 * 1: got key, 2: got colon, 3: got value */
			int state; 
		} id; /* in dictionary */
		struct
		{
			int escaped;
			int digit_count;
			/* acc is always of unicode type to handle \u and \U. 
			 * in the bch mode, it will get converted to a utf8 stream. */
			qse_wchar_t acc;
		} sv;
		struct
		{
			int escaped;
			int digit_count;
			/* for a character, no way to support the unicode character
			 * in the bch mode */
			qse_char_t acc; 
		} cv;
		struct
		{
			int dotted;
		} nv;
	} u;
	qse_json_state_node_t* next;
};

struct qse_json_t
{
	qse_mmgr_t* mmgr;
	qse_cmgr_t* cmgr;
	qse_json_prim_t prim;

	qse_json_errnum_t errnum;
	struct
	{
		qse_char_t backup[256];
		qse_char_t buf[256];
		/*qse_size_t len;*/
	} errmsg;

	struct
	{
		int trait;
	} cfg;

	qse_json_state_node_t state_top;
	qse_json_state_node_t* state_stack;

	qse_cstr_t tok;
	qse_size_t tok_capa;
};

/* ========================================================================= */

static void clear_token (qse_json_t* json)
{
	json->tok.len = 0;
	if (json->tok_capa > 0) json->tok.ptr[json->tok.len] = QSE_T('\0');
}

static int add_char_to_token (qse_json_t* json, qse_char_t ch)
{
	if (json->tok.len >= json->tok_capa)
	{
		qse_char_t* tmp;
		qse_size_t newcapa;

		newcapa = QSE_ALIGNTO_POW2(json->tok.len + 1, QSE_JSON_TOKEN_NAME_ALIGN);
		tmp = (qse_char_t*)qse_json_reallocmem(json, json->tok.ptr, newcapa * QSE_SIZEOF(*tmp));
		if (!tmp) return -1;

		json->tok_capa = newcapa;
		json->tok.ptr = tmp;
	}

	json->tok.ptr[json->tok.len++] = ch;
	json->tok.ptr[json->tok.len] = QSE_T('\0');
	return 0;
}

static QSE_INLINE qse_char_t unescape (qse_char_t c)
{
	switch (c)
	{
		case QSE_T('a'): return QSE_T('\a');
		case QSE_T('b'): return QSE_T('\b');
		case QSE_T('f'): return QSE_T('\f');
		case QSE_T('n'): return QSE_T('\n');
		case QSE_T('r'): return QSE_T('\r');
		case QSE_T('t'): return QSE_T('\t');
		case QSE_T('v'): return QSE_T('\v');
		default: return c;
	}
}

/* ========================================================================= */

static int push_state (qse_json_t* json, qse_json_state_t state)
{
	qse_json_state_node_t* ss;

	ss = (qse_json_state_node_t*)qse_json_callocmem(json, QSE_SIZEOF(*ss));
	if (!ss) return -1;

	ss->state = state;
	ss->next = json->state_stack;
	
	json->state_stack = ss;
	return 0;
}

static void pop_state (qse_json_t* json)
{
	qse_json_state_node_t* ss;

	ss = json->state_stack;
	QSE_ASSERT (ss != QSE_NULL && ss != &json->state_top);
	json->state_stack = ss->next;

	if (json->state_stack->state == QSE_JSON_STATE_IN_ARRAY)
	{
		json->state_stack->u.ia.got_value = 1;
	}
	else if (json->state_stack->state == QSE_JSON_STATE_IN_DIC)
	{
		json->state_stack->u.id.state++;
	}

/* TODO: don't free this. move it to the free list? */
	qse_json_freemem (json, ss);
}

static void pop_all_states (qse_json_t* json)
{
	while (json->state_stack != &json->state_top) pop_state (json);
}

/* ========================================================================= */

static int invoke_data_inst (qse_json_t* json, qse_json_inst_t inst)
{
	if (json->state_stack->state == QSE_JSON_STATE_IN_DIC && json->state_stack->u.id.state == 1) 
	{
		if (inst != QSE_JSON_INST_STRING)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("dictionary key not a string - %.*js"), (int)json->tok.len, json->tok.ptr);
			return -1;
		}

		inst = QSE_JSON_INST_KEY;
	}

	if (json->prim.instcb(json, inst, &json->tok) <= -1) return -1;
	return 0;
}

static int handle_string_value_char (qse_json_t* json, qse_cint_t c)
{
	int ret = 1;

	if (json->state_stack->u.sv.escaped == 3)
	{
		if (c >= '0' && c <= '7')
		{
			json->state_stack->u.sv.acc = json->state_stack->u.sv.acc * 8 + c - '0';
			json->state_stack->u.sv.digit_count++;
			if (json->state_stack->u.sv.digit_count >= json->state_stack->u.sv.escaped) goto add_sv_acc;
		}
		else
		{
			ret = 0;
			goto add_sv_acc;
		}
	}
	else if (json->state_stack->u.sv.escaped >= 2)
	{
		if (c >= '0' && c <= '9')
		{
			json->state_stack->u.sv.acc = json->state_stack->u.sv.acc * 16 + c - '0';
			json->state_stack->u.sv.digit_count++;
			if (json->state_stack->u.sv.digit_count >= json->state_stack->u.sv.escaped) goto add_sv_acc;
		}
		else if (c >= 'a' && c <= 'f')
		{
			json->state_stack->u.sv.acc = json->state_stack->u.sv.acc * 16 + c - 'a' + 10;
			json->state_stack->u.sv.digit_count++;
			if (json->state_stack->u.sv.digit_count >= json->state_stack->u.sv.escaped) goto add_sv_acc;
		}
		else if (c >= 'A' && c <= 'F')
		{
			json->state_stack->u.sv.acc = json->state_stack->u.sv.acc * 16 + c - 'A' + 10;
			json->state_stack->u.sv.digit_count++;
			if (json->state_stack->u.sv.digit_count >= json->state_stack->u.sv.escaped) goto add_sv_acc;
		}
		else
		{
			ret = 0;
		add_sv_acc:
		#if defined(QSE_OOCH_IS_BCH)
			/* convert the character to utf8 */
			{
				qse_mchar_t bcsbuf[QSE_BCSIZE_MAX];
				qse_size_t ucslen, bcslen;

				ucslen = 1;
				bcslen = QSE_COUNTOF(bcsbuf);
				if (qse_conv_uchars_to_bchars_with_cmgr(&json->state_stack->u.sv.acc, &ucslen, bcsbuf, &bcslen, qse_json_getcmgr(json)) <= -1)
				{
					qse_json_seterrfmt (json, QSE_JSON_EECERR, QSE_T("unable to convert %jc"), json->state_stack->u.sv.acc);
					return -1;
				}
				else
				{
					qse_size_t i;
					for (i = 0; i < bcslen; i++)
					{
						if (add_char_to_token(json, bcsbuf[i]) <= -1) return -1;
					}
				}
			}
		#else
			if (add_char_to_token(json, json->state_stack->u.sv.acc) <= -1) return -1;
		#endif
			json->state_stack->u.sv.escaped = 0;
		}
	}
	else if (json->state_stack->u.sv.escaped == 1)
	{
		if (c >= '0' && c <= '8') 
		{
			json->state_stack->u.sv.escaped = 3;
			json->state_stack->u.sv.digit_count = 0;
			json->state_stack->u.sv.acc = c - '0';
		}
		else if (c == 'x')
		{
			json->state_stack->u.sv.escaped = 2;
			json->state_stack->u.sv.digit_count = 0;
			json->state_stack->u.sv.acc = 0;
		}
		else if (c == 'u')
		{
			json->state_stack->u.sv.escaped = 4;
			json->state_stack->u.sv.digit_count = 0;
			json->state_stack->u.sv.acc = 0;
		}
		else if (c == 'U')
		{
			json->state_stack->u.sv.escaped = 8;
			json->state_stack->u.sv.digit_count = 0;
			json->state_stack->u.sv.acc = 0;
		}
		else
		{
			json->state_stack->u.sv.escaped = 0;
			if (add_char_to_token(json, unescape(c)) <= -1) return -1;
		}
	}
	else if (c == '\\')
	{
		json->state_stack->u.sv.escaped = 1;
	}
	else if (c == '\"')
	{
		pop_state (json);
		if (invoke_data_inst(json, QSE_JSON_INST_STRING) <= -1) return -1;
	}
	else
	{
		if (add_char_to_token(json, c) <= -1) return -1;
	}

	return ret;
}

static int handle_character_value_char (qse_json_t* json, qse_cint_t c)
{
	/* The real JSON dones't support character literal. this is HCL's own extension. */
	int ret = 1;

	if (json->state_stack->u.cv.escaped == 3)
	{
		if (c >= '0' && c <= '7')
		{
			json->state_stack->u.cv.acc = json->state_stack->u.cv.acc * 8 + c - '0';
			json->state_stack->u.cv.digit_count++;
			if (json->state_stack->u.cv.digit_count >= json->state_stack->u.cv.escaped) goto add_cv_acc;
		}
		else
		{
			ret = 0;
			goto add_cv_acc;
		}
	}
	if (json->state_stack->u.cv.escaped >= 2)
	{
		if (c >= '0' && c <= '9')
		{
			json->state_stack->u.cv.acc = json->state_stack->u.cv.acc * 16 + c - '0';
			json->state_stack->u.cv.digit_count++;
			if (json->state_stack->u.cv.digit_count >= json->state_stack->u.cv.escaped) goto add_cv_acc;
		}
		else if (c >= 'a' && c <= 'f')
		{
			json->state_stack->u.cv.acc = json->state_stack->u.cv.acc * 16 + c - 'a' + 10;
			json->state_stack->u.cv.digit_count++;
			if (json->state_stack->u.cv.digit_count >= json->state_stack->u.cv.escaped) goto add_cv_acc;
		}
		else if (c >= 'A' && c <= 'F')
		{
			json->state_stack->u.cv.acc = json->state_stack->u.cv.acc * 16 + c - 'A' + 10;
			json->state_stack->u.cv.digit_count++;
			if (json->state_stack->u.cv.digit_count >= json->state_stack->u.cv.escaped) goto add_cv_acc;
		}
		else
		{
			ret = 0;
		add_cv_acc:
			if (add_char_to_token(json, json->state_stack->u.cv.acc) <= -1) return -1;
			json->state_stack->u.cv.escaped = 0;
		}
	}
	else if (json->state_stack->u.cv.escaped == 1)
	{
		if (c >= '0' && c <= '8') 
		{
			json->state_stack->u.cv.escaped = 3;
			json->state_stack->u.cv.digit_count = 0;
			json->state_stack->u.cv.acc = c - '0';
		}
		else if (c == 'x')
		{
			json->state_stack->u.cv.escaped = 2;
			json->state_stack->u.cv.digit_count = 0;
			json->state_stack->u.cv.acc = 0;
		}
		else if (c == 'u')
		{
			json->state_stack->u.cv.escaped = 4;
			json->state_stack->u.cv.digit_count = 0;
			json->state_stack->u.cv.acc = 0;
		}
		else if (c == 'U')
		{
			json->state_stack->u.cv.escaped = 8;
			json->state_stack->u.cv.digit_count = 0;
			json->state_stack->u.cv.acc = 0;
		}
		else
		{
			json->state_stack->u.cv.escaped = 0;
			if (add_char_to_token(json, unescape(c)) <= -1) return -1;
		}
	}
	else if (c == '\\')
	{
		json->state_stack->u.cv.escaped = 1;
	}
	else if (c == '\'')
	{
		pop_state (json);
		
		if (json->tok.len < 1)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("no character in a character literal"));
			return -1;
		}
		if (invoke_data_inst(json, QSE_JSON_INST_CHARACTER) <= -1) return -1;
	}
	else
	{
		if (add_char_to_token(json, c) <= -1) return -1;
	}

	if (json->tok.len > 1) 
	{
		qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("too many characters in a character literal - %.*js"), (int)json->tok.len, json->tok.ptr);
		return -1;
	}

	return ret;
}

static int handle_numeric_value_char (qse_json_t* json, qse_cint_t c)
{
	if (QSE_ISMDIGIT(c) || (json->tok.len == 0 && (c == '+' || c == '-')))
	{
		if (add_char_to_token(json, c) <= -1) return -1;
		return 1;
	}
	else if (!json->state_stack->u.nv.dotted && c == '.' &&
	         json->tok.len > 0 && QSE_ISMDIGIT(json->tok.ptr[json->tok.len - 1]))
	{
		if (add_char_to_token(json, c) <= -1) return -1;
		json->state_stack->u.nv.dotted = 1;
		return 1;
	}

	pop_state (json);

	QSE_ASSERT (json->tok.len > 0);
	if (!QSE_ISMDIGIT(json->tok.ptr[json->tok.len - 1]))
	{
		qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("invalid numeric value - %.*js"), (int)json->tok.len, json->tok.ptr);
		return -1;
	}
	if (invoke_data_inst(json, QSE_JSON_INST_NUMBER) <= -1) return -1;
	return 0; /* start over */
}

static int handle_word_value_char (qse_json_t* json, qse_cint_t c)
{
	qse_json_inst_t inst;

	if (QSE_ISMALPHA(c))
	{
		if (add_char_to_token(json, c) <= -1) return -1;
		return 1;
	}

	pop_state (json);

	if (qse_strxcmp(json->tok.ptr, json->tok.len, QSE_T("null")) == 0) inst = QSE_JSON_INST_NIL;
	else if (qse_strxcmp(json->tok.ptr, json->tok.len, QSE_T("true")) == 0) inst = QSE_JSON_INST_TRUE;
	else if (qse_strxcmp(json->tok.ptr, json->tok.len, QSE_T("false")) == 0) inst = QSE_JSON_INST_FALSE;
	else
	{
		qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("invalid word value - %.*js"), (int)json->tok.len, json->tok.ptr);
		return -1;
	}

	if (invoke_data_inst(json, inst) <= -1) return -1;
	return 0; /* start over */
}

/* ========================================================================= */

static int handle_start_char (qse_json_t* json, qse_cint_t c)
{
	if (c == '[')
	{
		if (push_state(json, QSE_JSON_STATE_IN_ARRAY) <= -1) return -1;
		json->state_stack->u.ia.got_value = 0;
		if (json->prim.instcb(json, QSE_JSON_INST_START_ARRAY, QSE_NULL) <= -1) return -1;
		return 1;
	}
	else if (c == '{')
	{
		if (push_state(json, QSE_JSON_STATE_IN_DIC) <= -1) return -1;
		json->state_stack->u.id.state = 0;
		if (json->prim.instcb(json, QSE_JSON_INST_START_DIC, QSE_NULL) <= -1) return -1;
		return 1;
	}
	else if (QSE_ISMSPACE(c)) 
	{
		/* do nothing */
		return 1;
	}
	else
	{
		qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("not starting with [ or { - %jc"), (qse_char_t)c);
		return -1;
	}
}

static int handle_char_in_array (qse_json_t* json, qse_cint_t c)
{
	if (c == ']')
	{
		if (json->prim.instcb(json, QSE_JSON_INST_END_ARRAY, QSE_NULL) <= -1) return -1;
		pop_state (json);
		return 1;
	}
	else if (c == ',')
	{
		if (!json->state_stack->u.ia.got_value)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("redundant comma in array - %jc"), (qse_char_t)c);
			return -1;
		}
		json->state_stack->u.ia.got_value = 0;
		return 1;
	}
	else if (QSE_ISMSPACE(c))
	{
		/* do nothing */
		return 1;
	}
	else
	{
		if (json->state_stack->u.ia.got_value)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("comma required in array - %jc"), (qse_char_t)c);
			return -1;
		}

		if (c == '\"')
		{
			if (push_state(json, QSE_JSON_STATE_IN_STRING_VALUE) <= -1) return -1;
			clear_token (json);
			return 1;
		}
		else if (c == '\'')
		{
			if (push_state(json, QSE_JSON_STATE_IN_CHARACTER_VALUE) <= -1) return -1;
			clear_token (json);
			return 1;
		}
		/* TOOD: else if (c == '#') HCL radixed number
		 */
		else if (QSE_ISMDIGIT(c) || c == '+' || c == '-')
		{
			if (push_state(json, QSE_JSON_STATE_IN_NUMERIC_VALUE) <= -1) return -1;
			clear_token (json);
			json->state_stack->u.nv.dotted = 0;
			return 0; /* start over */
		}
		else if (QSE_ISMALPHA(c))
		{
			if (push_state(json, QSE_JSON_STATE_IN_WORD_VALUE) <= -1) return -1;
			clear_token (json);
			return 0; /* start over */
		}
		else if (c == '[')
		{
			if (push_state(json, QSE_JSON_STATE_IN_ARRAY) <= -1) return -1;
			json->state_stack->u.ia.got_value = 0;
			if (json->prim.instcb(json, QSE_JSON_INST_START_ARRAY, QSE_NULL) <= -1) return -1;
			return 1;
		}
		else if (c == '{')
		{
			if (push_state(json, QSE_JSON_STATE_IN_DIC) <= -1) return -1;
			json->state_stack->u.id.state = 0;
			if (json->prim.instcb(json, QSE_JSON_INST_START_DIC, QSE_NULL) <= -1) return -1;
			return 1;
		}
		else
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("wrong character inside array - %jc[%d]"), (qse_char_t)c, (int)c);
			return -1;
		}
	}
}

static int handle_char_in_dic (qse_json_t* json, qse_cint_t c)
{
	if (c == '}')
	{
		if (json->prim.instcb(json, QSE_JSON_INST_END_DIC, QSE_NULL) <= -1) return -1;
		pop_state (json);
		return 1;
	}
	else if (c == ':')
	{
		if (json->state_stack->u.id.state != 1)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("redundant colon in dictionary - %jc"), (qse_char_t)c);
			return -1;
		}
		json->state_stack->u.id.state++;
		return 1;
	}
	else if (c == ',')
	{
		if (json->state_stack->u.id.state != 3)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("redundant comma in dicitonary - %jc"), (qse_char_t)c);
			return -1;
		}
		json->state_stack->u.id.state = 0;
		return 1;
	}
	else if (QSE_ISMSPACE(c))
	{
		/* do nothing */
		return 1;
	}
	else
	{
		if (json->state_stack->u.id.state == 1)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("colon required in dicitonary - %jc"), (qse_char_t)c);
			return -1;
		}
		else if (json->state_stack->u.id.state == 3)
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("comma required in dicitonary - %jc"), (qse_char_t)c);
			return -1;
		}

		if (c == '\"')
		{
			if (push_state(json, QSE_JSON_STATE_IN_STRING_VALUE) <= -1) return -1;
			clear_token (json);
			return 1;
		}
		else if (c == '\'')
		{
			if (push_state(json, QSE_JSON_STATE_IN_CHARACTER_VALUE) <= -1) return -1;
			clear_token (json);
			return 1;
		}
		/* TOOD: else if (c == '#') HCL radixed number
		 */
		else if (QSE_ISMDIGIT(c) || c == '+' || c == '-')
		{
			if (push_state(json, QSE_JSON_STATE_IN_NUMERIC_VALUE) <= -1) return -1;
			clear_token (json);
			json->state_stack->u.nv.dotted = 0;
			return 0; /* start over */
		}
		else if (QSE_ISMALPHA(c))
		{
			if (push_state(json, QSE_JSON_STATE_IN_WORD_VALUE) <= -1) return -1;
			clear_token (json);
			return 0; /* start over */
		}
		else if (c == '[')
		{
			if (push_state(json, QSE_JSON_STATE_IN_ARRAY) <= -1) return -1;
			json->state_stack->u.ia.got_value = 0;
			if (json->prim.instcb(json, QSE_JSON_INST_START_ARRAY, QSE_NULL) <= -1) return -1;
			return 1;
		}
		else if (c == '{')
		{
			if (push_state(json, QSE_JSON_STATE_IN_DIC) <= -1) return -1;
			json->state_stack->u.id.state = 0;
			if (json->prim.instcb(json, QSE_JSON_INST_START_DIC, QSE_NULL) <= -1) return -1;
			return 1;
		}
		else
		{
			qse_json_seterrfmt (json, QSE_JSON_EINVAL, QSE_T("wrong character inside dictionary - %jc[%d]"), (qse_char_t)c, (int)c);
			return -1;
		}
	}
}

/* ========================================================================= */

static int handle_char (qse_json_t* json, qse_cint_t c, qse_size_t nbytes)
{
	int x;

start_over:
	if (c == QSE_CHAR_EOF)
	{
		if (json->state_stack->state == QSE_JSON_STATE_START)
		{
			/* no input data */
			return 0;
		}
		else
		{
			qse_json_seterrnum (json, QSE_JSON_EFINIS);
			return -1;
		}
	}

	switch (json->state_stack->state)
	{
		case QSE_JSON_STATE_START:
			x = handle_start_char(json, c);
			break;

		case QSE_JSON_STATE_IN_ARRAY:
			x = handle_char_in_array(json, c);
			break;

		case QSE_JSON_STATE_IN_DIC:
			x = handle_char_in_dic(json, c);
			break;

		case QSE_JSON_STATE_IN_WORD_VALUE:
			x = handle_word_value_char(json, c);
			break;

		case QSE_JSON_STATE_IN_STRING_VALUE:
			x = handle_string_value_char(json, c);
			break;
			
		case QSE_JSON_STATE_IN_CHARACTER_VALUE:
			x = handle_character_value_char(json, c);
			break;

		case QSE_JSON_STATE_IN_NUMERIC_VALUE:
			x = handle_numeric_value_char(json, c);
			break;

		default:
			qse_json_seterrfmt (json, QSE_JSON_EINTERN, QSE_T("internal error - must not be called for state %d"), (int)json->state_stack->state);
			return -1;
	}

	if (x <= -1) return -1;
	if (x == 0) goto start_over;

	return 0;
}

/* ========================================================================= */

static int feed_json_data (qse_json_t* json, const qse_mchar_t* data, qse_size_t len, qse_size_t* xlen)
{
	const qse_mchar_t* ptr;
	const qse_mchar_t* end;

	ptr = data;
	end = ptr + len;

	while (ptr < end)
	{
		qse_cint_t c;
		qse_size_t bcslen;

	#if defined(QSE_OOCH_IS_UCH)
		qse_size_t ucslen;
		qse_char_t uc;
		int n;

		bcslen = end - ptr;
		ucslen = 1;

		n = qse_conv_bchars_to_uchars_with_cmgr(ptr, &bcslen, &uc, &ucslen, json->cmgr, 0);
		if (n <= -1)
		{
			if (n == -3)
			{
				/* incomplete sequence */
				*xlen = ptr - data;
				return 0; /* feed more for incomplete sequence */
			}

			/* advance 1 byte without proper conversion */
			uc = *ptr;
			bcslen = 1;
		}

		ptr += bcslen;
		c = uc;
	#else
		bcslen = 1;
		c = *ptr++;
	#endif


		/* handle a signle character */
		if (handle_char(json, c, bcslen) <= -1) goto oops;
	}

	*xlen = ptr - data;
	return 1;

oops:
	/* TODO: compute the number of processed bytes so far and return it via a parameter??? */
/*printf ("feed oops....\n");*/
	return -1;
}


/* ========================================================================= */

qse_json_t* qse_json_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_json_prim_t* prim, qse_json_errnum_t* errnum)
{
	qse_json_t* json;

	json = (qse_json_t*)QSE_MMGR_ALLOC(mmgr, QSE_SIZEOF(*json) + xtnsize);
	if (!json) 
	{
		if (errnum) *errnum = QSE_JSON_ENOMEM;
		return QSE_NULL;
	}

	QSE_MEMSET (json, 0, QSE_SIZEOF(*json) + xtnsize);
	json->mmgr = mmgr;
	json->cmgr = qse_getdflcmgr();
	json->prim = *prim;

	json->state_top.state = QSE_JSON_STATE_START;
	json->state_top.next = QSE_NULL;
	json->state_stack = &json->state_top;

	return json;
}

void qse_json_close (qse_json_t* json)
{
	pop_all_states (json);
	if (json->tok.ptr) qse_json_freemem (json, json->tok.ptr);
	QSE_MMGR_FREE (json->mmgr, json);
}

int qse_json_setoption (qse_json_t* json, qse_json_option_t id, const void* value)
{
	switch (id)
	{
		case QSE_JSON_TRAIT:
			json->cfg.trait = *(const int*)value;
			return 0;
	}

	qse_json_seterrnum (json, QSE_JSON_EINVAL);
	return -1;
}

int qse_json_getoption (qse_json_t* json, qse_json_option_t id, void* value)
{
	switch (id)
	{
		case QSE_JSON_TRAIT:
			*(int*)value = json->cfg.trait;
			return 0;
	};

	qse_json_seterrnum (json, QSE_JSON_EINVAL);
	return -1;
}


void* qse_json_getxtn (qse_json_t* json)
{
	return (void*)(json + 1);
}

qse_mmgr_t* qse_json_getmmgr (qse_json_t* json)
{
	return json->mmgr;
}

qse_cmgr_t* qse_json_getcmgr (qse_json_t* json)
{
	return json->cmgr;
}

void qse_json_setcmgr (qse_json_t* json, qse_cmgr_t* cmgr)
{
	json->cmgr = cmgr;
}

qse_json_errnum_t qse_json_geterrnum (qse_json_t* json)
{
	return json->errnum;
}

const qse_char_t* qse_json_geterrmsg (qse_json_t* json)
{
	const qse_char_t* __errstr[] =
	{
		QSE_T("no error"),
		QSE_T("other error"),
		QSE_T("not implemented"),
		QSE_T("subsystem error"),
		QSE_T("internal error"),
		
		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("unexpected end of data")
	};
	return (json->errmsg.buf[0] == QSE_T('\0'))? __errstr[json->errnum]: json->errmsg.buf;

}

const qse_char_t* qse_json_backuperrmsg (qse_json_t* json)
{
	qse_strxcpy (json->errmsg.backup, QSE_COUNTOF(json->errmsg.backup), qse_json_geterrmsg(json));
	return json->errmsg.backup;
}

void qse_json_seterrnum (qse_json_t* json, qse_json_errnum_t errnum)
{
	/*if (json->shuterr) return; */
	json->errnum = errnum;
	json->errmsg.buf[0] = QSE_T('\0');
}

void qse_json_seterrfmt (qse_json_t* json, qse_json_errnum_t errnum, const qse_char_t* errfmt, ...)
{
	va_list ap;

	json->errnum = errnum;
	va_start (ap, errfmt);
	qse_strxvfmt(json->errmsg.buf, QSE_COUNTOF(json->errmsg.buf), errfmt, ap);
	va_end (ap);
}

/* ========================================================================= */

void* qse_json_allocmem (qse_json_t* json, qse_size_t size)
{
	void* ptr;

	ptr = QSE_MMGR_ALLOC(json->mmgr, size);
	if (!ptr) qse_json_seterrnum (json, QSE_JSON_ENOMEM);
	return ptr;
}

void* qse_json_callocmem (qse_json_t* json, qse_size_t size)
{
	void* ptr;

	ptr = QSE_MMGR_ALLOC(json->mmgr, size);
	if (!ptr) qse_json_seterrnum (json, QSE_JSON_ENOMEM);
	else QSE_MEMSET (ptr, 0, size);
	return ptr;
}

void* qse_json_reallocmem (qse_json_t* json, void* ptr, qse_size_t size)
{
	ptr = QSE_MMGR_REALLOC(json->mmgr, ptr, size);
	if (!ptr) qse_json_seterrnum (json, QSE_JSON_ENOMEM);
	return ptr;
}

void qse_json_freemem (qse_json_t* json, void* ptr)
{
	QSE_MMGR_FREE (json->mmgr, ptr);
}

/* ========================================================================= */

qse_json_state_t qse_json_getstate (qse_json_t* json)
{
	return json->state_stack->state;
}

void qse_json_reset (qse_json_t* json)
{
	/* TODO: reset XXXXXXXXXXXXXXXXXXXXXXXXXXXxxxxx */
	pop_all_states (json);
	QSE_ASSERT (json->state_stack == &json->state_top);
	json->state_stack->state = QSE_JSON_STATE_START;
}

int qse_json_feed (qse_json_t* json, const void* ptr, qse_size_t len, qse_size_t* xlen)
{
	int x;
	qse_size_t total, ylen;
	const qse_mchar_t* buf;

	buf = (const qse_mchar_t*)ptr;
	total = 0;
	while (total < len)
	{
		x = feed_json_data(json, &buf[total], len - total, &ylen);
		if (x <= -1) return -1;

		total += ylen;
		if (x == 0) break; /* incomplete sequence encountered */
	}

	*xlen = total;
	return 0;
}
