/*
 * $Id: str.h,v 1.4 2006-10-24 04:10:12 bacon Exp $
 */

#ifndef _ASE_AWK_STR_H_
#define _ASE_AWK_STR_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

#define ASE_AWK_STR_LEN(x)  ((x)->size)
#define ASE_AWK_STR_SIZE(x) ((x)->size + 1)
#define ASE_AWK_STR_CAPA(x) ((x)->capa)
#define ASE_AWK_STR_BUF(x)  ((x)->buf)
#define ASE_AWK_STR_CHAR(x,idx) ((x)->buf[idx])

typedef struct ase_awk_str_t ase_awk_str_t;

struct ase_awk_str_t
{
	ase_char_t* buf;
	ase_size_t size;
	ase_size_t capa;
	ase_awk_t* awk;
	ase_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_awk_str_t* ase_awk_str_open (
	ase_awk_str_t* str, ase_size_t capa, ase_awk_t* awk);

void ase_awk_str_close (ase_awk_str_t* str);

void ase_awk_str_forfeit (ase_awk_str_t* str);
void ase_awk_str_swap (ase_awk_str_t* str, ase_awk_str_t* str2);

ase_size_t ase_awk_str_cpy (ase_awk_str_t* str, const ase_char_t* s);

ase_size_t ase_awk_str_ncpy (
	ase_awk_str_t* str, const ase_char_t* s, ase_size_t len);

ase_size_t ase_awk_str_cat (ase_awk_str_t* str, const ase_char_t* s);

ase_size_t ase_awk_str_ncat (
	ase_awk_str_t* str, const ase_char_t* s, ase_size_t len);

ase_size_t ase_awk_str_ccat (ase_awk_str_t* str, ase_char_t c);

ase_size_t ase_awk_str_nccat (ase_awk_str_t* str, ase_char_t c, ase_size_t len);

void ase_awk_str_clear (ase_awk_str_t* str);

#ifdef __cplusplus
}
#endif

#endif
