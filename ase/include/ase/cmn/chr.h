/*
 * $Id: ctype.h 223 2008-06-26 06:44:41Z baconevi $
 */

#ifndef _ASE_CMN_CHR_H_
#define _ASE_CMN_CHR_H_

#include <ase/types.h>
#include <ase/macros.h>

/* gets a pointer to the default memory manager */
#define ASE_CCLS_GETDFL()  (ase_ccls)

/* sets a pointer to the default memory manager */
#define ASE_CCLS_SETDFL(m) ((ase_ccls)=(m))

#define ASE_CCLS_ISUPPER(ccls,c)  (ccls)->is_upper((ccls)->data,c)
#define ASE_CCLS_ISLOWER(ccls,c)  (ccls)->is_lower((ccls)->data,c)
#define ASE_CCLS_ISALPHA(ccls,c)  (ccls)->is_alpha((ccls)->data,c)
#define ASE_CCLS_ISDIGIT(ccls,c)  (ccls)->is_digit((ccls)->data,c)
#define ASE_CCLS_ISXDIGIT(ccls,c) (ccls)->is_xdigit((ccls)->data,c)
#define ASE_CCLS_ISALNUM(ccls,c)  (ccls)->is_alnum((ccls)->data,c)
#define ASE_CCLS_ISSPACE(ccls,c)  (ccls)->is_space((ccls)->data,c)
#define ASE_CCLS_ISPRINT(ccls,c)  (ccls)->is_print((ccls)->data,c)
#define ASE_CCLS_ISGRAPH(ccls,c)  (ccls)->is_graph((ccls)->data,c)
#define ASE_CCLS_ISCNTRL(ccls,c)  (ccls)->is_cntrl((ccls)->data,c)
#define ASE_CCLS_ISPUNCT(ccls,c)  (ccls)->is_punct((ccls)->data,c)
#define ASE_CCLS_TOUPPER(ccls,c)  (ccls)->to_upper((ccls)->data,c)
#define ASE_CCLS_TOLOWER(ccls,c)  (ccls)->to_lower((ccls)->data,c)

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
