/*
 * $Id: method.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_METHOD_H_
#define _QSE_STX_METHOD_H_

#include <qse/stx/stx.h>

#define QSE_STX_METHOD_SIZE           5
#define QSE_STX_METHOD_TEXT           0
#define QSE_STX_METHOD_SELECTOR       1
#define QSE_STX_METHOD_BYTECODES      2
#define QSE_STX_METHOD_TMPCOUNT       3
#define QSE_STX_METHOD_ARGCOUNT       4


/* dolphin smalltalk's flags representation
 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1 0
-------------------------------------------------------------------------------------------------
| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 1|  |  |  | 0| 0| 0| 1|
-------------------------------------------------------------------------------------------------
\----------|-----------/ \----------|----------/ \---------|-----------/  |	     \---|--/  |
     extraIndex			arg Count		temp Count	  |	       flags   |
									  |		       |
									Block flag	SmallInteger flag"
*/

struct qse_stx_method_t
{
	qse_stx_objhdr_t header;
	qse_word_t text;
	qse_word_t selector; /* is this necessary? */
	qse_word_t bytecodes;
	qse_word_t tmpcount;
	qse_word_t argcount;
	qse_word_t literals[1];
};

typedef struct qse_stx_method_t qse_stx_method_t;

#ifdef __cplusplus
extern "C"  {
#endif

#ifdef __cplusplus
}
#endif

#endif
