/*
 * $Id: str.h,v 1.1 2007-02-22 14:32:08 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_CMN_STR_H_
#define _ASE_CMN_STR_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_STR_LEN(x)      ((x)->size)
#define ASE_STR_SIZE(x)     ((x)->size + 1)
#define ASE_STR_CAPA(x)     ((x)->capa)
#define ASE_STR_BUF(x)      ((x)->buf)
#define ASE_STR_CHAR(x,idx) ((x)->buf[idx])

typedef struct ase_str_t ase_str_t;

struct ase_str_t
{
	ase_char_t* buf;
	ase_size_t  size;
	ase_size_t  capa;
	ase_bool_t  __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_str_t* ase_str_open (ase_str_t* str, ase_size_t capa, ase_t* awk);
void ase_str_close (ase_str_t* str);
void ase_str_clear (ase_str_t* str);

void ase_str_forfeit (ase_str_t* str);
void ase_str_swap (ase_str_t* str, ase_str_t* str2);

ase_size_t ase_str_cpy (ase_str_t* str, const ase_char_t* s);
ase_size_t ase_str_ncpy (ase_str_t* str, const ase_char_t* s, ase_size_t len);

ase_size_t ase_str_cat (ase_str_t* str, const ase_char_t* s);
ase_size_t ase_str_ncat (ase_str_t* str, const ase_char_t* s, ase_size_t len);
ase_size_t ase_str_ccat (ase_str_t* str, ase_char_t c);
ase_size_t ase_str_nccat (ase_str_t* str, ase_char_t c, ase_size_t len);

#ifdef __cplusplus
}
#endif

#endif
