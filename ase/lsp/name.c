/*
 * $Id: name.c,v 1.13 2006-12-05 03:38:11 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_name_t* ase_lsp_name_open (
	ase_lsp_name_t* name, ase_size_t capa, ase_lsp_t* lsp)
{
	if (capa == 0) capa = ASE_COUNTOF(name->static_buf) - 1;

	if (name == ASE_NULL) 
	{
		name = (ase_lsp_name_t*)
			ASE_LSP_MALLOC (lsp, ASE_SIZEOF(ase_lsp_name_t));
		if (name == ASE_NULL) return ASE_NULL;
		name->__dynamic = ase_true;
	}
	else name->__dynamic = ase_false;
	
	if (capa < ASE_COUNTOF(name->static_buf)) 
	{
		name->buf = name->static_buf;
	}
	else 
	{
		name->buf = (ase_char_t*)
			ASE_LSP_MALLOC (lsp, (capa+1)*ASE_SIZEOF(ase_char_t));
		if (name->buf == ASE_NULL) 
		{
			if (name->__dynamic) ASE_LSP_FREE (lsp, name);
			return ASE_NULL;
		}
	}

	name->size   = 0;
	name->capa   = capa;
	name->buf[0] = ASE_T('\0');
	name->lsp    = lsp;

	return name;
}

void ase_lsp_name_close (ase_lsp_name_t* name)
{
	if (name->capa >= ASE_COUNTOF(name->static_buf)) 
	{
		ASE_LSP_ASSERT (name->lsp, name->buf != name->static_buf);
		ASE_LSP_FREE (name->lsp, name->buf);
	}
	if (name->__dynamic) ASE_LSP_FREE (name->lsp, name);
}

int ase_lsp_name_addc (ase_lsp_name_t* name, ase_cint_t c)
{
	if (name->size >= name->capa) 
	{
		/* double the capacity */
		ase_size_t new_capa = name->capa * 2;

		if (new_capa >= ASE_COUNTOF(name->static_buf)) 
		{
			ase_char_t* space;

			if (name->capa < ASE_COUNTOF(name->static_buf)) 
			{
				space = (ase_char_t*) ASE_LSP_MALLOC (
					name->lsp, (new_capa+1)*ASE_SIZEOF(ase_char_t));
				if (space == ASE_NULL) return -1;

				/* don't need to copy up to the terminating null */
				ASE_LSP_MEMCPY (name->lsp, space, name->buf, 
					name->capa*ASE_SIZEOF(ase_char_t));
			}
			else 
			{
				space = (ase_char_t*) ASE_LSP_REALLOC (
					name->lsp, name->buf, 
					(new_capa+1)*ASE_SIZEOF(ase_char_t));
				if (space == ASE_NULL) return -1;
			}

			name->buf = space;
		}

		name->capa = new_capa;
	}

	name->buf[name->size++] = c;
	name->buf[name->size]   = ASE_T('\0');
	return 0;
}

int ase_lsp_name_adds (ase_lsp_name_t* name, const ase_char_t* s)
{
	while (*s != ASE_T('\0')) 
	{
		if (ase_lsp_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void ase_lsp_name_clear (ase_lsp_name_t* name)
{
	name->size   = 0;
	name->buf[0] = ASE_T('\0');
}

int ase_lsp_name_compare (ase_lsp_name_t* name, const ase_char_t* str)
{
	ase_char_t* p = name->buf;
	ase_size_t index = 0;

	while (index < name->size) 
	{
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == ASE_T('\0'))? 0: -1;
}
