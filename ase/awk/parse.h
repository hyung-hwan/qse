/*
 * $Id: parse.h,v 1.3 2006-10-24 04:10:12 bacon Exp $
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

#ifdef __cplusplus
}
#endif

#endif
