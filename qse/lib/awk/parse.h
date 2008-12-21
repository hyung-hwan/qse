/*
 * $Id: parse.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LIB_AWK_PARSE_H_
#define _ASE_LIB_AWK_PARSE_H_

/* these enums should match kwtab in parse.c */
enum kw_t
{
	KW_IN,
	KW_BEGIN,
	KW_END,
	KW_FUNCTION,
	KW_LOCAL,
	KW_GLOBAL,
	KW_IF,
	KW_ELSE,
	KW_WHILE,
	KW_FOR,
	KW_DO,
	KW_BREAK,
	KW_CONTINUE,
	KW_RETURN,
	KW_EXIT,
	KW_NEXT,
	KW_NEXTFILE,
	KW_NEXTOFILE,
	KW_DELETE,
	KW_RESET,
	KW_PRINT,
	KW_PRINTF,
	KW_GETLINE,
};

#ifdef __cplusplus
extern "C" {
#endif

int ase_awk_putsrcstr (ase_awk_t* awk, const ase_char_t* str);
int ase_awk_putsrcstrx (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len);

const ase_char_t* ase_awk_getglobalname (
	ase_awk_t* awk, ase_size_t idx, ase_size_t* len);
ase_cstr_t* ase_awk_getkw (ase_awk_t* awk, int id, ase_cstr_t* s);


int ase_awk_initglobals (ase_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
