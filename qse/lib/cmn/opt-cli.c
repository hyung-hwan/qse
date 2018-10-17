/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/cmn/opt.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>

/* 
 * PARAM_BUNCH should be always 2^n if you don't want to the % operator
 */
#define PARAM_BUNCH 8

static const qse_char_t* __optsta[] =
{
	QSE_T("--"),
	QSE_NULL
};

static const qse_char_t* __optasn = QSE_T("=");
static qse_char_t __emptyval[] = { QSE_T('\0') };

static qse_char_t* __begins_with (const qse_char_t* str, const qse_char_t* prefixes[])
{
	const qse_char_t** pp = prefixes;

	while (*pp) 
	{
		if (qse_strbeg(str, *pp)) return (qse_char_t*)*pp;
		pp++;
	}

	return QSE_NULL;
}

int qse_parsecli (qse_cli_t* cli, qse_mmgr_t* mmgr, int argc, qse_char_t* const argv[], qse_cli_data_t* data)
{
	int index = 1;
	qse_cli_opt_t* opt;
	const qse_char_t** optsta;
	const qse_char_t* optasn;

	cli->mmgr = mmgr;
	cli->data = *data;
	cli->verb = argv[0];
	cli->nparams = 0;
	cli->params = QSE_NULL;

	optsta = cli->data.optsta;
	optasn = cli->data.optasn;
	if (!optsta) optsta = __optsta;
	if (!optasn) optasn = __optasn;

	/* initialize value for each opt */
	for (opt = cli->data.opts; opt->name; opt++) 
	{
		opt->value = QSE_NULL;
	}

	while (index < argc) 
	{
		const qse_char_t* ip;
		qse_char_t* ap = argv[index++];
		qse_char_t* value;

		ip = __begins_with(ap, optsta);
		if (ip) 
		{
			/* opt */
			qse_cstr_t name;

			ap = qse_strtok (ap + qse_strlen(ip), optasn, &name);
			if (name.len <= 0) continue;

			for (opt = cli->data.opts; opt->name; opt++) 
			{
				if (qse_strxcmp(name.ptr, name.len, opt->name) == 0) break;
			}

			value = QSE_NULL;
			if (ap == QSE_NULL) 
			{
				if (index < argc && !__begins_with(argv[index], optsta)) 
				{
					value = argv[index++];
				}
			}
			else 
			{
				/* beware that this changes the original text */
				name.ptr[name.len] = QSE_T('\0'); 
				value = ap;
			}

			if (opt->name == QSE_NULL) 
			{
				if (cli->data.errcb(cli, QSE_CLI_ERROR_INVALID_OPTNAME, name.ptr, value) <= -1) 
				{
					qse_clearcli (cli);
					return -1;
				}
			}
			else 
			{
				if (value && !(opt->optflags & (QSE_CLI_REQUIRE_OPTVAL | QSE_CLI_DISCRETIONARY_OPTVAL))) 
				{
					if (cli->data.errcb(cli, QSE_CLI_ERROR_REDUNDANT_OPTVAL, name.ptr, value) <= -1) 
					{
						qse_clearcli (cli);
						return -1;
					}
				}
				else if (!value && (opt->optflags & QSE_CLI_REQUIRE_OPTVAL)) 
				{
					if (cli->data.errcb(cli, QSE_CLI_ERROR_MISSING_OPTVAL, name.ptr, value) <= -1) 
					{
						qse_clearcli (cli);
						return -1;
					}
				}
				else if (!value) 
				{
					opt->value = __emptyval;
				}
				else
				{
					opt->value = value;
				}
			}
		}
		else 
		{
			if ((cli->nparams & (PARAM_BUNCH - 1)) == 0) 
			{
				qse_char_t** t;

				t = (qse_char_t**)QSE_MMGR_REALLOC(cli->mmgr, cli->params, (cli->nparams + PARAM_BUNCH) * QSE_SIZEOF(qse_char_t*));
				if (!t) 
				{
					if (cli->data.errcb(cli, QSE_CLI_ERROR_MEMORY, ap, QSE_NULL) <= -1) 
					{
						qse_clearcli (cli);
						return -1;
					}
					else continue;
				}
				cli->params = t;
			}

			cli->params[cli->nparams++] = ap;
		}
	}

	for (opt = cli->data.opts; opt->name != QSE_NULL; opt++) 
	{
		if ((opt->optflags & QSE_CLI_REQUIRE_OPTNAME) && opt->value == QSE_NULL) 
		{
			if (cli->data.errcb(cli, QSE_CLI_ERROR_MISSING_OPTNAME, opt->name, QSE_NULL) <= -1) 
			{
				qse_clearcli (cli);
				return -1;
			}
		}
	}

	return 0;
}

void qse_clearcli (qse_cli_t* cli)
{
	if (cli->params) 
	{
		QSE_ASSERT (cli->nparams > 0);
		QSE_MMGR_FREE (cli->mmgr, cli->params);
		cli->nparams = 0;
		cli->params = QSE_NULL;
	}
}
 
qse_char_t* qse_getcliverb (qse_cli_t* cli)
{
	return cli->verb;
}

qse_char_t* qse_getclioptval (qse_cli_t* cli, const qse_char_t* opt)
{
	qse_cli_opt_t* q = cli->data.opts;

	while (q->name) 
	{
		if (q->value && qse_strcmp(opt, q->name) == 0) return q->value;
		q++;
	}

	return QSE_NULL;
}

qse_char_t* qse_getcliparam (qse_cli_t* cli, int index)
{
	QSE_ASSERT (index >= 0 && index < cli->nparams);
	return cli->params[index];
}
