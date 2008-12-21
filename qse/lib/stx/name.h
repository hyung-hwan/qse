/*
 * $Id: name.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_NAME_H_
#define _QSE_STX_NAME_H_

#include <qse/stx/stx.h>

struct qse_stx_name_t 
{
	qse_word_t   capacity;
	qse_word_t   size;
	qse_char_t*  buffer;
	qse_char_t   static_buffer[128];
	qse_bool_t __dynamic;
};

typedef struct qse_stx_name_t qse_stx_name_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_stx_name_t* qse_stx_name_open (
	qse_stx_name_t* name, qse_word_t capacity);
void qse_stx_name_close (qse_stx_name_t* name);

int qse_stx_name_addc (qse_stx_name_t* name, qse_cint_t c);
int qse_stx_name_adds (qse_stx_name_t* name, const qse_char_t* s);
void qse_stx_name_clear (qse_stx_name_t* name);
qse_char_t* qse_stx_name_yield (qse_stx_name_t* name, qse_word_t capacity);
int qse_stx_name_compare  (qse_stx_name_t* name, const qse_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
