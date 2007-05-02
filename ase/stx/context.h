/* 
 * $Id: context.h,v 1.3 2007/04/30 08:32:41 bacon Exp $
 */

#ifndef _ASE_STX_CONTEXT_H_
#define _ASE_STX_CONTEXT_H_

#include <ase/stx/stx.h>

#define PUSH_OBJECT 0xA0
#define SEND_UNARY_MESSAGE 0xB0
#define HALT 0xFF

#define ASE_STX_CONTEXT_SIZE        4
#define ASE_STX_CONTEXT_IP          0
#define ASE_STX_CONTEXT_METHOD      1
#define ASE_STX_CONTEXT_ARGUMENTS   2
#define ASE_STX_CONTEXT_TEMPORARIES 3

struct ase_stx_context_t
{
	ase_stx_objhdr_t header;
	ase_word_t ip;
	ase_word_t method;
	ase_word_t arguments;
	ase_word_t temporaries;
};

typedef struct ase_stx_context_t ase_stx_context_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_new_context (ase_stx_t* stx, 
	ase_word_t method, ase_word_t args, ase_word_t temp);
int ase_stx_run_context (ase_stx_t* stx, ase_word_t context);

#ifdef __cplusplus
}
#endif

#endif
