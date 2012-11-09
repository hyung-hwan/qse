/*
 * $Id$ 
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "awk.h"

static void free_fun (qse_htb_t* map, void* vptr, qse_size_t vlen)
{
	qse_awk_t* awk = *(qse_awk_t**)QSE_XTN(map);
	qse_awk_fun_t* f = (qse_awk_fun_t*)vptr;

	/* f->name doesn't have to be freed */
	/*QSE_AWK_FREE (awk, f->name);*/

	qse_awk_clrpt (awk, f->body);
	QSE_AWK_FREE (awk, f);
}

static void free_fnc (qse_htb_t* map, void* vptr, qse_size_t vlen)
{
	qse_awk_t* awk = *(qse_awk_t**)QSE_XTN(map);
	qse_awk_fnc_t* f = (qse_awk_fnc_t*)vptr;
	QSE_AWK_FREE (awk, f);
}

static int init_token (qse_mmgr_t* mmgr, qse_awk_tok_t* tok)
{
	tok->name = qse_str_open (mmgr, 0, 128);
	if (tok->name == QSE_NULL) return -1;
	
	tok->type = 0;
	tok->loc.file = QSE_NULL;
	tok->loc.line = 0;
	tok->loc.colm = 0;

	return 0;
}

static void fini_token (qse_awk_tok_t* tok)
{
	if (tok->name != QSE_NULL)
	{
		qse_str_close (tok->name);
		tok->name = QSE_NULL;
	}
}

static void clear_token (qse_awk_tok_t* tok)
{
	if (tok->name != QSE_NULL) qse_str_clear (tok->name);
	tok->type = 0;
	tok->loc.file = QSE_NULL;
	tok->loc.line = 0;
	tok->loc.colm = 0;
}

qse_awk_t* qse_awk_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_awk_prm_t* prm)
{
	qse_awk_t* awk;

	static qse_htb_mancbs_t treefuncbs =
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT 
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_fun
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};

	static qse_htb_mancbs_t fncusercbs =
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT 
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_fnc
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};

	/* allocate the object */
	awk = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_awk_t) + xtnsize);
	if (awk == QSE_NULL) return QSE_NULL;

	/* zero out the object + extension area */
	QSE_MEMSET (awk, 0, QSE_SIZEOF(qse_awk_t) + xtnsize);

	/* remember the memory manager */
	awk->mmgr = mmgr;

	/* progagate the primitive functions */
	QSE_ASSERT (prm             != QSE_NULL);
	QSE_ASSERT (prm->sprintf    != QSE_NULL);
	QSE_ASSERT (prm->math.pow   != QSE_NULL);
	QSE_ASSERT (prm->math.sin   != QSE_NULL);
	QSE_ASSERT (prm->math.cos   != QSE_NULL);
	QSE_ASSERT (prm->math.tan   != QSE_NULL);
	QSE_ASSERT (prm->math.atan  != QSE_NULL);
	QSE_ASSERT (prm->math.atan2 != QSE_NULL);
	QSE_ASSERT (prm->math.log   != QSE_NULL);
	QSE_ASSERT (prm->math.log10 != QSE_NULL);
	QSE_ASSERT (prm->math.exp   != QSE_NULL);
	QSE_ASSERT (prm->math.sqrt  != QSE_NULL);
	if (prm             == QSE_NULL || 
	    prm->sprintf    == QSE_NULL ||
	    prm->math.pow   == QSE_NULL ||
	    prm->math.sin   == QSE_NULL ||
	    prm->math.cos   == QSE_NULL ||
	    prm->math.tan   == QSE_NULL ||
	    prm->math.atan  == QSE_NULL ||
	    prm->math.atan2 == QSE_NULL ||
	    prm->math.log   == QSE_NULL ||
	    prm->math.log10 == QSE_NULL ||
	    prm->math.exp   == QSE_NULL ||
	    prm->math.sqrt  == QSE_NULL)
	{
		QSE_AWK_FREE (awk, awk);
		return QSE_NULL;
	}
	awk->prm = *prm;

	if (init_token (mmgr, &awk->ptok) == -1) goto oops;
	if (init_token (mmgr, &awk->tok) == -1) goto oops;
	if (init_token (mmgr, &awk->ntok) == -1) goto oops;

	awk->sio.names = qse_htb_open (
		mmgr, QSE_SIZEOF(awk), 128, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (awk->sio.names == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->sio.names) = awk;
	qse_htb_setmancbs (awk->sio.names, 
		qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_KEY_COPIER)
	);
	awk->sio.inp = &awk->sio.arg;

	/* TODO: initial map size?? */
	awk->tree.funs = qse_htb_open (
		mmgr, QSE_SIZEOF(awk), 512, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (awk->tree.funs == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->tree.funs) = awk;
	qse_htb_setmancbs (awk->tree.funs, &treefuncbs);

	awk->parse.funs = qse_htb_open (
		mmgr, QSE_SIZEOF(awk), 256, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (awk->parse.funs == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->parse.funs) = awk;
	qse_htb_setmancbs (awk->parse.funs,
		qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_KEY_COPIER)
	);

	awk->parse.named = qse_htb_open (
		mmgr, QSE_SIZEOF(awk), 256, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (awk->parse.named == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->parse.named) = awk;
	qse_htb_setmancbs (awk->parse.named,
		qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_KEY_COPIER)
	);

	awk->parse.gbls = qse_lda_open (mmgr, QSE_SIZEOF(awk), 128);
	awk->parse.lcls = qse_lda_open (mmgr, QSE_SIZEOF(awk), 64);
	awk->parse.params = qse_lda_open (mmgr, QSE_SIZEOF(awk), 32);

	if (awk->parse.gbls == QSE_NULL ||
	    awk->parse.lcls == QSE_NULL ||
	    awk->parse.params == QSE_NULL) goto oops;

	*(qse_awk_t**)QSE_XTN(awk->parse.gbls) = awk;
	qse_lda_setscale (awk->parse.gbls, QSE_SIZEOF(qse_char_t));
	qse_lda_setcopier (awk->parse.gbls, QSE_LDA_COPIER_INLINE);

	*(qse_awk_t**)QSE_XTN(awk->parse.lcls) = awk;
	qse_lda_setscale (awk->parse.lcls, QSE_SIZEOF(qse_char_t));
	qse_lda_setcopier (awk->parse.lcls, QSE_LDA_COPIER_INLINE);

	*(qse_awk_t**)QSE_XTN(awk->parse.params) = awk;
	qse_lda_setscale (awk->parse.params, QSE_SIZEOF(qse_char_t));
	qse_lda_setcopier (awk->parse.params, QSE_LDA_COPIER_INLINE);

	awk->opt.trait = QSE_AWK_MODERN;
#if defined(__OS2__) || defined(_WIN32) || defined(__DOS__)
	awk->opt.trait |= QSE_AWK_CRLF;
#endif

	awk->errinf.num = QSE_AWK_ENOERR;
	awk->errinf.loc.line = 0;
	awk->errinf.loc.colm = 0;
	awk->errinf.loc.file = QSE_NULL;
	awk->errstr = qse_awk_dflerrstr;
	awk->stopall = 0;

	awk->tree.ngbls = 0;
	awk->tree.ngbls_base = 0;
	awk->tree.begin = QSE_NULL;
	awk->tree.begin_tail = QSE_NULL;
	awk->tree.end = QSE_NULL;
	awk->tree.end_tail = QSE_NULL;
	awk->tree.chain = QSE_NULL;
	awk->tree.chain_tail = QSE_NULL;
	awk->tree.chain_size = 0;

	awk->fnc.sys = QSE_NULL;
	awk->fnc.user = qse_htb_open (
		mmgr, QSE_SIZEOF(awk), 512, 70, QSE_SIZEOF(qse_char_t), 1
	);
	if (awk->fnc.user == QSE_NULL) goto oops;
	*(qse_awk_t**)QSE_XTN(awk->fnc.user) = awk;
	qse_htb_setmancbs (awk->fnc.user, &fncusercbs);

	awk->modtab = qse_rbt_open (mmgr, 0, QSE_SIZEOF(qse_char_t), 1);
	if (awk->modtab == QSE_NULL) goto  oops;
	qse_rbt_setmancbs (
		awk->modtab,
		qse_getrbtmancbs(QSE_RBT_MANCBS_INLINE_COPIERS)
	);

	if (qse_awk_initgbls (awk) <= -1) goto oops;

	return awk;

oops:
	if (awk->modtab) qse_rbt_close (awk->modtab);
	if (awk->fnc.user) qse_htb_close (awk->fnc.user);
	if (awk->parse.params) qse_lda_close (awk->parse.params);
	if (awk->parse.lcls) qse_lda_close (awk->parse.lcls);
	if (awk->parse.gbls) qse_lda_close (awk->parse.gbls);
	if (awk->parse.named) qse_htb_close (awk->parse.named);
	if (awk->parse.funs) qse_htb_close (awk->parse.funs);
	if (awk->tree.funs) qse_htb_close (awk->tree.funs);
	if (awk->sio.names) qse_htb_close (awk->sio.names);
	fini_token (&awk->ntok);
	fini_token (&awk->tok);
	fini_token (&awk->ptok);
	QSE_AWK_FREE (awk, awk);

	return QSE_NULL;
}

int qse_awk_close (qse_awk_t* awk)
{
	qse_awk_ecb_t* ecb;
	int i;

	if (qse_awk_clear (awk) <= -1) return -1;
	/*qse_awk_clrfnc (awk);*/

	for (ecb = awk->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (awk);

	qse_rbt_close (awk->modtab);
	qse_htb_close (awk->fnc.user);

	qse_lda_close (awk->parse.params);
	qse_lda_close (awk->parse.lcls);
	qse_lda_close (awk->parse.gbls);
	qse_htb_close (awk->parse.named);
	qse_htb_close (awk->parse.funs);

	qse_htb_close (awk->tree.funs);
	qse_htb_close (awk->sio.names);

	fini_token (&awk->ntok);
	fini_token (&awk->tok);
	fini_token (&awk->ptok);

	/* destroy dynamically allocated options */
	for (i = 0; i < QSE_COUNTOF(awk->opt.mod); i++)
	{
		if (awk->opt.mod[i].ptr) 
			QSE_MMGR_FREE (awk->mmgr, awk->opt.mod[i].ptr);
	}

	/* QSE_AWK_ALLOC, QSE_AWK_FREE, etc can not be used 
	 * from the next line onwards */
	QSE_AWK_FREE (awk, awk);
	return 0;
}

static qse_rbt_walk_t unload_module (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_awk_t* awk = (qse_awk_t*)ctx;
	qse_awk_mod_data_t* md;

	md = QSE_RBT_VPTR(pair);	
	if (md->mod.unload) md->mod.unload (&md->mod, awk);
	awk->prm.modclose (awk, md->handle);

	return QSE_RBT_WALK_FORWARD;
}

int qse_awk_clear (qse_awk_t* awk)
{
	qse_awk_ecb_t* ecb;

     for (ecb = awk->ecb; ecb; ecb = ecb->next)
          if (ecb->clear) ecb->clear (awk);

	awk->stopall = 0;

	clear_token (&awk->tok);
	clear_token (&awk->ntok);
	clear_token (&awk->ptok);

	qse_rbt_walk (awk->modtab, unload_module, awk);
	qse_rbt_clear (awk->modtab);

	QSE_ASSERT (QSE_LDA_SIZE(awk->parse.gbls) == awk->tree.ngbls);
	/* delete all non-builtin global variables */
	qse_lda_delete (
		awk->parse.gbls, awk->tree.ngbls_base, 
		QSE_LDA_SIZE(awk->parse.gbls) - awk->tree.ngbls_base);

	qse_lda_clear (awk->parse.lcls);
	qse_lda_clear (awk->parse.params);
	qse_htb_clear (awk->parse.named);
	qse_htb_clear (awk->parse.funs);

	awk->parse.nlcls_max = 0; 
	awk->parse.depth.block = 0;
	awk->parse.depth.loop = 0;
	awk->parse.depth.expr = 0;
	awk->parse.depth.incl = 0;

	/* clear parse trees */	
	/*awk->tree.ngbls_base = 0;
	awk->tree.ngbls = 0;	 */
	awk->tree.ngbls = awk->tree.ngbls_base;

	awk->tree.cur_fun.ptr = QSE_NULL;
	awk->tree.cur_fun.len = 0;
	qse_htb_clear (awk->tree.funs);

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

	QSE_ASSERT (awk->sio.inp == &awk->sio.arg);
	/* this table must not be cleared here as there can be a reference
	 * to an entry of this table from errinf.fil when qse_awk_parse() 
	 * failed. this table is cleared in qse_awk_parse().
	 * qse_htb_clear (awk->sio.names);
	 */

	awk->sio.last.c = QSE_CHAR_EOF;
	awk->sio.last.line = 0;
	awk->sio.last.colm = 0;
	awk->sio.last.file = QSE_NULL;
	awk->sio.nungots = 0;

	awk->sio.arg.line = 1;
	awk->sio.arg.colm = 1;
	awk->sio.arg.b.pos = 0;
	awk->sio.arg.b.len = 0;

	return 0;
}

qse_mmgr_t* qse_awk_getmmgr (qse_awk_t* awk)
{
	return awk->mmgr;
}

void* qse_awk_getxtn (qse_awk_t* awk)
{
	return QSE_XTN (awk);
}

qse_awk_prm_t* qse_awk_getprm (qse_awk_t* awk)
{
	return &awk->prm;
}

int qse_awk_setopt (qse_awk_t* awk, qse_awk_opt_t id, const void* value)
{
	switch (id)
	{
		case QSE_AWK_TRAIT:
			awk->opt.trait = *(const int*)value;
			return 0;

		case QSE_AWK_MODPREFIX:
		case QSE_AWK_MODPOSTFIX:
		{
			qse_xstr_t tmp;
			int idx;

			idx = id - QSE_AWK_MODPREFIX;
			if (value)
			{
				tmp.ptr = qse_strdup (value, awk->mmgr);
				if (tmp.ptr == QSE_NULL)
				{
					qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
					return -1;
				}
				tmp.len = qse_strlen (tmp.ptr);
			}
			else
			{
				tmp.ptr = QSE_NULL;
				tmp.len = 0;
			}

			if (awk->opt.mod[idx].ptr)
				QSE_MMGR_FREE (awk->mmgr, awk->opt.mod[idx].ptr);

			awk->opt.mod[idx] = tmp;
			return 0;
		}

		case QSE_AWK_DEPTH_INCLUDE:
		case QSE_AWK_DEPTH_BLOCK_PARSE:
		case QSE_AWK_DEPTH_BLOCK_RUN:
		case QSE_AWK_DEPTH_EXPR_PARSE:
		case QSE_AWK_DEPTH_EXPR_RUN:
		case QSE_AWK_DEPTH_REX_BUILD:
		case QSE_AWK_DEPTH_REX_MATCH:
			awk->opt.depth.a[id - QSE_AWK_DEPTH_INCLUDE] = *(const qse_size_t*)value;
			return 0;
	}

	qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
	return -1;
}

int qse_awk_getopt (qse_awk_t* awk, qse_awk_opt_t  id, void* value)
{
	switch  (id)
	{
		case QSE_AWK_TRAIT:
			*(int*)value = awk->opt.trait;
			return 0;

		case QSE_AWK_MODPREFIX:
		case QSE_AWK_MODPOSTFIX:
			*(const qse_char_t**)value = awk->opt.mod[id - QSE_AWK_MODPREFIX].ptr;
			return 0;

		case QSE_AWK_DEPTH_INCLUDE:
		case QSE_AWK_DEPTH_BLOCK_PARSE:
		case QSE_AWK_DEPTH_BLOCK_RUN:
		case QSE_AWK_DEPTH_EXPR_PARSE:
		case QSE_AWK_DEPTH_EXPR_RUN:
		case QSE_AWK_DEPTH_REX_BUILD:
		case QSE_AWK_DEPTH_REX_MATCH:
			*(qse_size_t*)value = awk->opt.depth.a[id - QSE_AWK_DEPTH_INCLUDE];
			return 0;
	};

	qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
	return -1;
}

void qse_awk_stopall (qse_awk_t* awk)
{
	awk->stopall = 1;
	qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
}

qse_awk_ecb_t* qse_awk_popecb (qse_awk_t* awk)
{
	qse_awk_ecb_t* top = awk->ecb;
	if (top) awk->ecb = top->next;
	return top;
}

void qse_awk_pushecb (qse_awk_t* awk, qse_awk_ecb_t* ecb)
{
	ecb->next = awk->ecb;
	awk->ecb = ecb;
}

