/*
 * $Id: ctype.h 194 2008-06-06 13:00:39Z baconevi $
 */

#ifndef _ASE_UTL_CTYPE_H_
#define _ASE_UTL_CTYPE_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_bool_t ase_isupper  (ase_cint_t c);
ase_bool_t ase_islower  (ase_cint_t c);
ase_bool_t ase_isalpha  (ase_cint_t c);
ase_bool_t ase_isdigit  (ase_cint_t c);
ase_bool_t ase_isxdigit (ase_cint_t c);
ase_bool_t ase_isalnum  (ase_cint_t c);
ase_bool_t ase_isspace  (ase_cint_t c);
ase_bool_t ase_isprint  (ase_cint_t c);
ase_bool_t ase_isgraph  (ase_cint_t c);
ase_bool_t ase_iscntrl  (ase_cint_t c);
ase_bool_t ase_ispunct  (ase_cint_t c);

ase_cint_t ase_toupper  (ase_cint_t c);
ase_cint_t ase_tolower  (ase_cint_t c);

#ifdef __cplusplus
}
#endif

#endif