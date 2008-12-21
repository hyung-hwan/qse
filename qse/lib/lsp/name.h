/*
 * $Id: name.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LSP_NAME_H_
#define _QSE_LSP_NAME_H_

#include <qse/types.h>
#include <qse/macros.h>

struct qse_lsp_name_t 
{
	qse_size_t  capa;
	qse_size_t  size;
	qse_char_t* buf;
	qse_char_t  static_buf[128];
	qse_lsp_t*  lsp;
	qse_bool_t __dynamic;
};

typedef struct qse_lsp_name_t qse_lsp_name_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_lsp_name_t* qse_lsp_name_open (
	qse_lsp_name_t* name, qse_size_t capa, qse_lsp_t* lsp);
void qse_lsp_name_close (qse_lsp_name_t* name);

int qse_lsp_name_addc (qse_lsp_name_t* name, qse_cint_t c);
int qse_lsp_name_adds (qse_lsp_name_t* name, const qse_char_t* s);
void qse_lsp_name_clear (qse_lsp_name_t* name);
int qse_lsp_name_compare (qse_lsp_name_t* name, const qse_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
