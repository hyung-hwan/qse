/*
 * $Id: func.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_AWK_FUNC_H_
#define _QSE_LIB_AWK_FUNC_H_

typedef struct qse_awk_bfn_t qse_awk_bfn_t;

struct qse_awk_bfn_t
{
	struct
	{
		qse_char_t* ptr;
		qse_size_t  len;
	} name;

	int valid; /* the entry is valid when this option is set */

	struct
	{
		qse_size_t min;
		qse_size_t max;
		qse_char_t* spec;
	} arg;

	int (*handler) (qse_awk_run_t*, const qse_char_t*, qse_size_t);

	/*qse_awk_bfn_t* next;*/
};

#ifdef __cplusplus
extern "C" {
#endif

qse_awk_bfn_t* qse_awk_getbfn (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
