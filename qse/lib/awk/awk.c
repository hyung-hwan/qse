/*
 * $Id: awk.c 232 2009-07-14 08:06:14Z hyunghwan.chung $ 
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include "awk.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (awk)

#define SETERR(awk,code) qse_awk_seterrnum(awk,code)

#define SETERRARG(awk,code,line,arg,leng) \
	do { \
		qse_cstr_t errarg; \
		errarg.len = (leng); \
		errarg.ptr = (arg); \
		qse_awk_seterror ((awk), (code), (line), &errarg); \
	} while (0)

static void free_fun (qse_map_t* map, void* vptr, qse_size_t vlen)
{
	qse_awk_t* awk = *(qse_awk_t**)QSE_XTN(map);
	qse_awk_fun_t* f = (qse_awk_fun_t*)vptr;

	/* f->name doesn't have to be freed */
	/*QSE_AWK_FREE (awk, f->name);*/

	qse_awk_clrpt (awk, f->body);
	QSE_AWK_FREE (awk, f);
}

static void free_fnc (qse_map_t* map, void* vptr, qse_size_t vlen)
{
	qse_awk_t* awk = *(qse_awk_t**)QSE_XTN(map);
	qse_awk_fnc_t* f = (qse_awk_fnc_t*)vptr;
	QSE_AWK_FREE (awk, f);
}

static int init_token (qse_mmgr_t* mmgr, qse_awk_token_t* token)
{
	token->name = qse_str_open (mmgr, 0, 128);
	if (token->name == QSE_NULL) return -1;
	
	token->type = 0;
	token->line = 0;
	token->column = 0;

	return 0;
}

static void fini_token (qse_awk_token_t* token)
{
	if (token->name != QSE_NULL)
	{
		qse_str_close (token->name);
		token->name = QSE_NULL;
	}
}

qse_awk_t* qse_awk_open (qse_mmgr_t* mmgr, qse_size_t xtn, qse_awk_prm_t* prm)
{
	qse_awk_t* awk;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	/* allocate the object */
	awk = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_awk_t) + xtn);
	if (awk == QSE_NULL) return QSE_NULL;

	/* zero out the object */
	QSE_MEMSET (awk, 0, QSE_SIZEOF(qse_awk_t) + xtn);

	/* remember the memory manager */
	awk->mmgr = mmgr;

	/* progagate the primitive functions */
	QSE_ASSERT (prm          != QSE_NULL);
	QSE_ASSERT (prm->pow     != QSE_NULL);
	QSE_ASSERT (prm->sprintf != QSE_NULL);
	if (prm          == QSE_NULL || 
	    prm->pow     == QSE_NULL ||
	    prm->sprintf == QSE_NULL)
	{
		QSE_AWK_FREE (awk, awk);
		return QSE_NULL;
	}
	awk->prm = *prm;

	if (init_token (mmgr, &awk->token) == -1) goto oops;
	if (init_token (mmgr, &awk->atoken) == -1) goto oops;

	awk->wtab = qse_map_open (mmgr, QSE_SIZEOF(awk), 512, 70);
	if (awk->wtab == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->wtab) = awk;
	qse_map_setcopier (awk->wtab, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setcopier (awk->wtab, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (awk->wtab, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));
	qse_map_setscale (awk->wtab, QSE_MAP_VAL, QSE_SIZEOF(qse_char_t));

	awk->rwtab = qse_map_open (mmgr, QSE_SIZEOF(awk), 512, 70);
	if (awk->rwtab == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->rwtab) = awk;
	qse_map_setcopier (awk->rwtab, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setcopier (awk->rwtab, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (awk->rwtab, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));
	qse_map_setscale (awk->rwtab, QSE_MAP_VAL, QSE_SIZEOF(qse_char_t));

	/* TODO: initial map size?? */
	awk->tree.funs = qse_map_open (mmgr, QSE_SIZEOF(awk), 512, 70);
	if (awk->tree.funs == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->tree.funs) = awk;
	qse_map_setcopier (awk->tree.funs, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setfreeer (awk->tree.funs, QSE_MAP_VAL, free_fun);
	qse_map_setscale (awk->tree.funs, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	awk->parse.funs = qse_map_open (mmgr, QSE_SIZEOF(awk), 256, 70);
	if (awk->parse.funs == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->parse.funs) = awk;
	qse_map_setcopier (awk->parse.funs, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setcopier (awk->parse.funs, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (awk->parse.funs, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	awk->parse.named = qse_map_open (mmgr, QSE_SIZEOF(awk), 256, 70);
	if (awk->parse.named == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->parse.named) = awk;
	qse_map_setcopier (awk->parse.named, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setcopier (awk->parse.named, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (awk->parse.named, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	awk->parse.gbls = qse_lda_open (mmgr, QSE_SIZEOF(awk), 128);
	awk->parse.lcls = qse_lda_open (mmgr, QSE_SIZEOF(awk), 64);
	awk->parse.params = qse_lda_open (mmgr, QSE_SIZEOF(awk), 32);

	if (awk->parse.gbls == QSE_NULL ||
	    awk->parse.lcls == QSE_NULL ||
	    awk->parse.params == QSE_NULL) goto oops;

	*(qse_awk_t**)QSE_XTN(awk->parse.gbls) = awk;
	qse_lda_setcopier (awk->parse.gbls, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (awk->parse.gbls, QSE_SIZEOF(qse_char_t));

	*(qse_awk_t**)QSE_XTN(awk->parse.lcls) = awk;
	qse_lda_setcopier (awk->parse.lcls, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (awk->parse.lcls, QSE_SIZEOF(qse_char_t));

	*(qse_awk_t**)QSE_XTN(awk->parse.params) = awk;
	qse_lda_setcopier (awk->parse.params, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (awk->parse.params, QSE_SIZEOF(qse_char_t));

	awk->option = QSE_AWK_CLASSIC;
	awk->errinf.num = QSE_AWK_ENOERR;
	awk->errinf.lin = 0;
	awk->errstr = qse_awk_dflerrstr;
	awk->stopall = QSE_FALSE;

	awk->parse.nlcls_max = 0;

	awk->tree.ngbls = 0;
	awk->tree.ngbls_base = 0;
	awk->tree.begin = QSE_NULL;
	awk->tree.begin_tail = QSE_NULL;
	awk->tree.end = QSE_NULL;
	awk->tree.end_tail = QSE_NULL;
	awk->tree.chain = QSE_NULL;
	awk->tree.chain_tail = QSE_NULL;
	awk->tree.chain_size = 0;

	awk->ptoken.type = 0;
	awk->ptoken.line = 0;
	awk->ptoken.column = 0;

	awk->src.lex.curc = QSE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	awk->fnc.sys = QSE_NULL;
	awk->fnc.user = qse_map_open (mmgr, QSE_SIZEOF(awk), 512, 70);
	if (awk->fnc.user == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->fnc.user) = awk;
	qse_map_setcopier (awk->fnc.user, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setfreeer (awk->fnc.user, QSE_MAP_VAL, free_fnc); 
	qse_map_setscale (awk->fnc.user, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_BLOCK_PARSE, 0);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_BLOCK_RUN, 0);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_EXPR_PARSE, 0);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_EXPR_RUN, 0);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_REX_BUILD, 0);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_REX_MATCH, 0);

	if (qse_awk_initgbls (awk) <= -1) goto oops;

	return awk;


oops:
	if (awk->fnc.user) qse_map_close (awk->fnc.user);
	if (awk->parse.params) qse_lda_close (awk->parse.params);
	if (awk->parse.lcls) qse_lda_close (awk->parse.lcls);
	if (awk->parse.gbls) qse_lda_close (awk->parse.gbls);
	if (awk->parse.named) qse_map_close (awk->parse.named);
	if (awk->parse.funs) qse_map_close (awk->parse.funs);
	if (awk->tree.funs) qse_map_close (awk->tree.funs);
	if (awk->rwtab) qse_map_close (awk->rwtab);
	if (awk->wtab) qse_map_close (awk->wtab);
	fini_token (&awk->atoken);
	fini_token (&awk->token);
	QSE_AWK_FREE (awk, awk);

	return QSE_NULL;
}

int qse_awk_close (qse_awk_t* awk)
{
	if (qse_awk_clear (awk) <= -1) return -1;
	/*qse_awk_clrfnc (awk);*/
	qse_map_close (awk->fnc.user);

	qse_lda_close (awk->parse.params);
	qse_lda_close (awk->parse.lcls);
	qse_lda_close (awk->parse.gbls);
	qse_map_close (awk->parse.named);
	qse_map_close (awk->parse.funs);

	qse_map_close (awk->tree.funs);
	qse_map_close (awk->rwtab);
	qse_map_close (awk->wtab);

	fini_token (&awk->atoken);
	fini_token (&awk->token);

	/* QSE_AWK_ALLOC, QSE_AWK_FREE, etc can not be used 
	 * from the next line onwards */
	QSE_AWK_FREE (awk, awk);
	return 0;
}

int qse_awk_clear (qse_awk_t* awk)
{
	awk->stopall = QSE_FALSE;

	QSE_MEMSET (&awk->src.ios, 0, QSE_SIZEOF(awk->src.ios));
	awk->src.lex.curc = QSE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	QSE_ASSERT (QSE_LDA_SIZE(awk->parse.gbls) == awk->tree.ngbls);
	/* delete all non-builtin global variables */
	qse_lda_delete (
		awk->parse.gbls, awk->tree.ngbls_base, 
		QSE_LDA_SIZE(awk->parse.gbls) - awk->tree.ngbls_base);

	qse_lda_clear (awk->parse.lcls);
	qse_lda_clear (awk->parse.params);
	qse_map_clear (awk->parse.named);
	qse_map_clear (awk->parse.funs);

	awk->parse.nlcls_max = 0; 
	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	/* clear parse trees */	
	awk->tree.ok = 0;
	/*awk->tree.ngbls_base = 0;
	awk->tree.ngbls = 0;	 */
	awk->tree.ngbls = awk->tree.ngbls_base;

	awk->tree.cur_fun.ptr = QSE_NULL;
	awk->tree.cur_fun.len = 0;
	qse_map_clear (awk->tree.funs);

	if (awk->tree.begin != QSE_NULL) 
	{
		/*QSE_ASSERT (awk->tree.begin->next == QSE_NULL);*/
		qse_awk_clrpt (awk, awk->tree.begin);
		awk->tree.begin = QSE_NULL;
		awk->tree.begin_tail = QSE_NULL;	
	}

	if (awk->tree.end != QSE_NULL) 
	{
		/*QSE_ASSERT (awk->tree.end->next == QSE_NULL);*/
		qse_awk_clrpt (awk, awk->tree.end);
		awk->tree.end = QSE_NULL;
		awk->tree.end_tail = QSE_NULL;	
	}

	while (awk->tree.chain != QSE_NULL) 
	{
		qse_awk_chain_t* next = awk->tree.chain->next;

		if (awk->tree.chain->pattern != QSE_NULL)
			qse_awk_clrpt (awk, awk->tree.chain->pattern);
		if (awk->tree.chain->action != QSE_NULL)
			qse_awk_clrpt (awk, awk->tree.chain->action);
		QSE_AWK_FREE (awk, awk->tree.chain);
		awk->tree.chain = next;
	}

	awk->tree.chain_tail = QSE_NULL;	
	awk->tree.chain_size = 0;

	return 0;
}

qse_awk_prm_t* qse_awk_getprm (qse_awk_t* awk)
{
	return &awk->prm;
}

int qse_awk_getoption (qse_awk_t* awk)
{
	return awk->option;
}

void qse_awk_setoption (qse_awk_t* awk, int opt)
{
	awk->option = opt;
}

void qse_awk_stopall (qse_awk_t* awk)
{
	awk->stopall = QSE_TRUE;
}

int qse_awk_getword (qse_awk_t* awk, 
	const qse_char_t* okw, qse_size_t olen,
	const qse_char_t** nkw, qse_size_t* nlen)
{
	qse_map_pair_t* p;

	p = qse_map_search (awk->wtab, okw, olen);
	if (p == QSE_NULL) return -1;

	*nkw = ((qse_cstr_t*)p->vptr)->ptr;
	*nlen = ((qse_cstr_t*)p->vptr)->len;

	return 0;
}

int qse_awk_unsetword (qse_awk_t* awk, const qse_char_t* kw, qse_size_t len)
{
	qse_map_pair_t* p;

	p = qse_map_search (awk->wtab, kw, len);
	if (p == QSE_NULL)
	{
		SETERRARG (awk, QSE_AWK_ENOENT, 0, kw, len);
		return -1;
	}

	qse_map_delete (awk->rwtab, QSE_MAP_VPTR(p), QSE_MAP_VLEN(p));
	qse_map_delete (awk->wtab, kw, len);
	return 0;
}

void qse_awk_unsetallwords (qse_awk_t* awk)
{
	qse_map_clear (awk->wtab);
	qse_map_clear (awk->rwtab);
}

int qse_awk_setword (qse_awk_t* awk, 
	const qse_char_t* okw, qse_size_t olen,
	const qse_char_t* nkw, qse_size_t nlen)
{
	if (nkw == QSE_NULL || nlen == 0)
	{
		if (okw == QSE_NULL || olen == 0)
		{
			/* clear the entire table */
			qse_awk_unsetallwords (awk);
			return 0;
		}

		return qse_awk_unsetword (awk, okw, olen);
	}
	else if (okw == QSE_NULL || olen == 0)
	{
		SETERR (awk, QSE_AWK_EINVAL);
		return -1;
	}

	/* set the word */
	if (qse_map_upsert (awk->wtab, 
		(qse_char_t*)okw, olen, (qse_char_t*)nkw, nlen) == QSE_NULL)
	{
		SETERR (awk, QSE_AWK_ENOMEM);
		return -1;
	}

	if (qse_map_upsert (awk->rwtab, 
		(qse_char_t*)nkw, nlen, (qse_char_t*)okw, olen) == QSE_NULL)
	{
		qse_map_delete (awk->wtab, okw, olen);
		SETERR (awk, QSE_AWK_ENOMEM);
		return -1;
	}
 
	return 0;
}

