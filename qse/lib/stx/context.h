/* 
 * $Id: context.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_CONTEXT_H_
#define _QSE_STX_CONTEXT_H_

#include <qse/stx/stx.h>

#define PUSH_OBJECT 0xA0
#define SEND_UNARY_MESSAGE 0xB0
#define HALT 0xFF

#define QSE_STX_CONTEXT_SIZE        4
#define QSE_STX_CONTEXT_IP          0
#define QSE_STX_CONTEXT_METHOD      1
#define QSE_STX_CONTEXT_ARGUMENTS   2
#define QSE_STX_CONTEXT_TEMPORARIES 3

struct qse_stx_context_t
{
	qse_stx_objhdr_t header;
	qse_word_t ip;
	qse_word_t method;
	qse_word_t arguments;
	qse_word_t temporaries;
};

typedef struct qse_stx_context_t qse_stx_context_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_new_context (qse_stx_t* stx, 
	qse_word_t method, qse_word_t args, qse_word_t temp);
int qse_stx_run_context (qse_stx_t* stx, qse_word_t context);

#ifdef __cplusplus
}
#endif

#endif
