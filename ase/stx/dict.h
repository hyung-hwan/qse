/*
 * $Id: dict.h,v 1.3 2007/04/30 08:32:41 bacon Exp $
 */

#ifndef _ASE_STX_DICT_H_
#define _ASE_STX_DICT_H_

#include <ase/stx/stx.h>

#define ASE_STX_ASSOCIATION_SIZE  2
#define ASE_STX_ASSOCIATION_KEY   0
#define ASE_STX_ASSOCIATION_VALUE 1

struct ase_stx_association_t
{
	ase_stx_objhdr_t header;
	ase_word_t key;
	ase_word_t value;
};

typedef struct ase_stx_association_t ase_stx_association_t;

#ifdef __cplusplus
extern "C"
#endif

ase_word_t ase_stx_dict_lookup (
        ase_stx_t* stx, ase_word_t dict, const ase_char_t* key);
ase_word_t ase_stx_dict_get (
        ase_stx_t* stx, ase_word_t dict, ase_word_t key);
ase_word_t ase_stx_dict_put (
        ase_stx_t* stx, ase_word_t dict, ase_word_t key, ase_word_t value);
void ase_stx_dict_traverse (
        ase_stx_t* stx, ase_word_t dict,
        void (*func) (ase_stx_t*,ase_word_t,void*), void* data);


#ifdef __cplusplus
}
#endif

#endif
