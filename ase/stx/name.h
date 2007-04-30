/*
 * $Id: name.h,v 1.1.1.1 2007/03/28 14:05:28 bacon Exp $
 */

#ifndef _ASE_STX_NAME_H_
#define _ASE_STX_NAME_H_

#include <ase/stx/stx.h>

struct ase_stx_name_t 
{
	ase_word_t   capacity;
	ase_word_t   size;
	ase_char_t*  buffer;
	ase_char_t   static_buffer[128];
	ase_bool_t __dynamic;
};

typedef struct ase_stx_name_t ase_stx_name_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_stx_name_t* ase_stx_name_open (
	ase_stx_name_t* name, ase_word_t capacity);
void ase_stx_name_close (ase_stx_name_t* name);

int ase_stx_name_addc (ase_stx_name_t* name, ase_cint_t c);
int ase_stx_name_adds (ase_stx_name_t* name, const ase_char_t* s);
void ase_stx_name_clear (ase_stx_name_t* name);
ase_char_t* ase_stx_name_yield (ase_stx_name_t* name, ase_word_t capacity);
int ase_stx_name_compare  (ase_stx_name_t* name, const ase_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
