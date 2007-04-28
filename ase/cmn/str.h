/*
 * $Id: str.h,v 1.1 2007/03/28 14:05:21 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_CMN_STR_H_
#define _ASE_CMN_STR_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

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
	ase_mmgr_t* mmgr;
	ase_bool_t  __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_size_t ase_strlen (const ase_char_t* str);

ase_size_t ase_strcpy (
	ase_char_t* buf, const ase_char_t* str);
ase_size_t ase_strxcpy (
	ase_char_t* buf, ase_size_t bsz, const ase_char_t* str);
ase_size_t ase_strncpy (
	ase_char_t* buf, const ase_char_t* str, ase_size_t len);
ase_size_t ase_strxncpy (
    ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len);

ase_size_t ase_strxncat (
    ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len);

int ase_strcmp (const ase_char_t* s1, const ase_char_t* s2);
int ase_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);
int ase_strcasecmp (
	const ase_char_t* s1, const ase_char_t* s2, ase_ccls_t* ccls);
int ase_strxncasecmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2, ase_ccls_t* ccls);

ase_char_t* ase_strdup (const ase_char_t* str, ase_mmgr_t* mmgr);
ase_char_t* ase_strxdup (
	const ase_char_t* str, ase_size_t len, ase_mmgr_t* mmgr);
ase_char_t* ase_strxdup2 (
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2, ase_mmgr_t* mmgr);

ase_char_t* ase_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz);

ase_str_t* ase_str_open (ase_str_t* str, ase_size_t capa, ase_mmgr_t* mmgr);
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
