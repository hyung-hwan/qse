/*
 * $Id: method.h,v 1.8 2005-09-13 11:15:41 bacon Exp $
 */

#ifndef _XP_STX_METHOD_H_
#define _XP_STX_METHOD_H_

#include <xp/stx/stx.h>

#define XP_STX_METHOD_SIZE           5
#define XP_STX_METHOD_TEXT           0
#define XP_STX_METHOD_SELECTOR       1
#define XP_STX_METHOD_BYTECODES      2
#define XP_STX_METHOD_TMPCOUNT       3
#define XP_STX_METHOD_ARGCOUNT       4


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

struct xp_stx_method_t
{
	xp_stx_objhdr_t header;
	xp_word_t text;
	xp_word_t selector; /* is this necessary? */
	xp_word_t bytecodes;
	xp_word_t tmpcount;
	xp_word_t argcount;
	xp_word_t literals[1];
};

typedef struct xp_stx_method_t xp_stx_method_t;

#ifdef __cplusplus
extern "C"  {
#endif

#ifdef __cplusplus
}
#endif

#endif
