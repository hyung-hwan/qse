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

#define ASE_CCLS_IS(ccls,c,type) ((ccls)->is((ccls)->data,c,type))
#define ASE_CCLS_TO(ccls,c,type) ((ccls)->to((ccls)->data,c,type))

#define ASE_CCLS_ISUPPER(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_UPPER)
#define ASE_CCLS_ISLOWER(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_LOWER)
#define ASE_CCLS_ISALPHA(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_ALPHA)
#define ASE_CCLS_ISDIGIT(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_DIGIT)
#define ASE_CCLS_ISXDIGIT(ccls,c) ASE_CCLS_IS(ccls,c,ASE_CCLS_XDIGIT)
#define ASE_CCLS_ISALNUM(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_ALNUM)
#define ASE_CCLS_ISSPACE(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_SPACE)
#define ASE_CCLS_ISPRINT(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_PRINT)
#define ASE_CCLS_ISGRAPH(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_GRAPH)
#define ASE_CCLS_ISCNTRL(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_CNTRL)
#define ASE_CCLS_ISPUNCT(ccls,c)  ASE_CCLS_IS(ccls,c,ASE_CCLS_PUNCT)
#define ASE_CCLS_TOUPPER(ccls,c)  ASE_CCLS_TO(ccls,c,ASE_CCLS_UPPER)
#define ASE_CCLS_TOLOWER(ccls,c)  ASE_CCLS_TO(ccls,c,ASE_CCLS_LOWER)

#ifdef __cplusplus
extern "C" {
#endif

extern ase_ccls_t* ase_ccls;

ase_bool_t ase_ccls_is (ase_cint_t c, int type);
ase_cint_t ase_ccls_to (ase_cint_t c, int type);

#ifdef __cplusplus
}
#endif

#endif
