/*
 * $Id: parse.h 115 2008-03-03 11:13:15Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_AWK_PARSE_H_
#define _ASE_AWK_PARSE_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ase_awk_putsrcstr (ase_awk_t* awk, const ase_char_t* str);
int ase_awk_putsrcstrx (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len);

const ase_char_t* ase_awk_getglobalname (
	ase_awk_t* awk, ase_size_t idx, ase_size_t* len);
const ase_char_t* ase_awk_getkw (ase_awk_t* awk, const ase_char_t* kw);

int ase_awk_initglobals (ase_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
