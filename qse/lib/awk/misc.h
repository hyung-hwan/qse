/*
 * $Id: misc.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_AWK_MISC_H_
#define _QSE_LIB_AWK_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

qse_char_t* qse_awk_strtok (
	qse_awk_run_t* run, const qse_char_t* s, 
	const qse_char_t* delim, qse_char_t** tok, qse_size_t* tok_len);

qse_char_t* qse_awk_strxtok (
	qse_awk_run_t* run, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_char_t** tok, qse_size_t* tok_len);

qse_char_t* qse_awk_strntok (
	qse_awk_run_t* run, const qse_char_t* s, 
	const qse_char_t* delim, qse_size_t delim_len,
	qse_char_t** tok, qse_size_t* tok_len);

qse_char_t* qse_awk_strxntok (
	qse_awk_run_t* run, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_size_t delim_len,
	qse_char_t** tok, qse_size_t* tok_len);

qse_char_t* qse_awk_strxntokbyrex (
	qse_awk_run_t* run, const qse_char_t* s, qse_size_t len,
	void* rex, qse_char_t** tok, qse_size_t* tok_len, int* errnum);


void* qse_awk_buildrex (
	qse_awk_t* awk, const qse_char_t* ptn, qse_size_t len, int* errnum);

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int option,
        const qse_char_t* str, qse_size_t len,
        const qse_char_t** match_ptr, qse_size_t* match_len, int* errnum);

#ifdef __cplusplus
}
#endif

#endif

