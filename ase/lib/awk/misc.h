/*
 * $Id: misc.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LIB_AWK_MISC_H_
#define _ASE_LIB_AWK_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

ase_char_t* ase_awk_strtok (
	ase_awk_run_t* run, const ase_char_t* s, 
	const ase_char_t* delim, ase_char_t** tok, ase_size_t* tok_len);

ase_char_t* ase_awk_strxtok (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	const ase_char_t* delim, ase_char_t** tok, ase_size_t* tok_len);

ase_char_t* ase_awk_strntok (
	ase_awk_run_t* run, const ase_char_t* s, 
	const ase_char_t* delim, ase_size_t delim_len,
	ase_char_t** tok, ase_size_t* tok_len);

ase_char_t* ase_awk_strxntok (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	const ase_char_t* delim, ase_size_t delim_len,
	ase_char_t** tok, ase_size_t* tok_len);

ase_char_t* ase_awk_strxntokbyrex (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	void* rex, ase_char_t** tok, ase_size_t* tok_len, int* errnum);


void* ase_awk_buildrex (
	ase_awk_t* awk, const ase_char_t* ptn, ase_size_t len, int* errnum);

int ase_awk_matchrex (
	ase_awk_t* awk, void* code, int option,
        const ase_char_t* str, ase_size_t len,
        const ase_char_t** match_ptr, ase_size_t* match_len, int* errnum);

#ifdef __cplusplus
}
#endif

#endif

