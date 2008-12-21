/*
 * $Id: name.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

qse_lsp_name_t* qse_lsp_name_open (
	qse_lsp_name_t* name, qse_size_t capa, qse_lsp_t* lsp)
{
	if (capa == 0) capa = QSE_COUNTOF(name->static_buf) - 1;

	if (name == QSE_NULL) 
	{
		name = (qse_lsp_name_t*)
			QSE_LSP_ALLOC (lsp, QSE_SIZEOF(qse_lsp_name_t));
		if (name == QSE_NULL) return QSE_NULL;
		name->__dynamic = QSE_TRUE;
	}
	else name->__dynamic = QSE_FALSE;
	
	if (capa < QSE_COUNTOF(name->static_buf)) 
	{
		name->buf = name->static_buf;
	}
	else 
	{
		name->buf = (qse_char_t*)
			QSE_LSP_ALLOC (lsp, (capa+1)*QSE_SIZEOF(qse_char_t));
		if (name->buf == QSE_NULL) 
		{
			if (name->__dynamic) QSE_LSP_FREE (lsp, name);
			return QSE_NULL;
		}
	}

	name->size   = 0;
	name->capa   = capa;
	name->buf[0] = QSE_T('\0');
	name->lsp    = lsp;

	return name;
}

void qse_lsp_name_close (qse_lsp_name_t* name)
{
	if (name->capa >= QSE_COUNTOF(name->static_buf)) 
	{
		QSE_ASSERT (name->buf != name->static_buf);
		QSE_LSP_FREE (name->lsp, name->buf);
	}
	if (name->__dynamic) QSE_LSP_FREE (name->lsp, name);
}

int qse_lsp_name_addc (qse_lsp_name_t* name, qse_cint_t c)
{
	if (name->size >= name->capa) 
	{
		/* double the capacity */
		qse_size_t new_capa = name->capa * 2;

		if (new_capa >= QSE_COUNTOF(name->static_buf)) 
		{
			qse_char_t* space;

			if (name->capa < QSE_COUNTOF(name->static_buf)) 
			{
				space = (qse_char_t*) QSE_LSP_ALLOC (
					name->lsp, (new_capa+1)*QSE_SIZEOF(qse_char_t));
				if (space == QSE_NULL) return -1;

				/* don't need to copy up to the terminating null */
				QSE_MEMCPY (space, name->buf, name->capa*QSE_SIZEOF(qse_char_t));
			}
			else 
			{
				space = (qse_char_t*) QSE_LSP_REALLOC (
					name->lsp, name->buf, 
					(new_capa+1)*QSE_SIZEOF(qse_char_t));
				if (space == QSE_NULL) return -1;
			}

			name->buf = space;
		}

		name->capa = new_capa;
	}

	name->buf[name->size++] = c;
	name->buf[name->size]   = QSE_T('\0');
	return 0;
}

int qse_lsp_name_adds (qse_lsp_name_t* name, const qse_char_t* s)
{
	while (*s != QSE_T('\0')) 
	{
		if (qse_lsp_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void qse_lsp_name_clear (qse_lsp_name_t* name)
{
	name->size   = 0;
	name->buf[0] = QSE_T('\0');
}

int qse_lsp_name_compare (qse_lsp_name_t* name, const qse_char_t* str)
{
	qse_char_t* p = name->buf;
	qse_size_t index = 0;

	while (index < name->size) 
	{
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == QSE_T('\0'))? 0: -1;
}
