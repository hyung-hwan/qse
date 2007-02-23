/*
 * $id$
 */

#ifndef _ASE_UTL_CTYPE_H_
#define _ASE_UTL_CTYPE_H_

#include <ase/types.h>
#include <ase/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_bool_t ase_isupper  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_islower  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isalpha  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isdigit  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isxdigit (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isalnum  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isspace  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isprint  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_isgraph  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_iscntrl  (ase_ccls_t* ccls, ase_cint_t c);
ase_bool_t ase_ispunct  (ase_ccls_t* ccls, ase_cint_t c);

ase_cint_t ase_toupper  (ase_ccls_t* ccls, ase_cint_t c);
ase_cint_t ase_tolower  (ase_ccls_t* ccls, ase_cint_t c);

#ifdef __cplusplus
}
#endif

#endif
