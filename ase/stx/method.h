/*
 * $Id: method.h,v 1.9 2007-03-22 11:19:28 bacon Exp $
 */

#ifndef _ASE_STX_METHOD_H_
#define _ASE_STX_METHOD_H_

#include <ase/stx/stx.h>

#define ASE_STX_METHOD_SIZE           5
#define ASE_STX_METHOD_TEXT           0
#define ASE_STX_METHOD_SELECTOR       1
#define ASE_STX_METHOD_BYTECODES      2
#define ASE_STX_METHOD_TMPCOUNT       3
#define ASE_STX_METHOD_ARGCOUNT       4


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

struct ase_stx_method_t
{
	ase_stx_objhdr_t header;
	ase_word_t text;
	ase_word_t selector; /* is this necessary? */
	ase_word_t bytecodes;
	ase_word_t tmpcount;
	ase_word_t argcount;
	ase_word_t literals[1];
};

typedef struct ase_stx_method_t ase_stx_method_t;

#ifdef __cplusplus
extern "C"  {
#endif

#ifdef __cplusplus
}
#endif

#endif
