/*
 * $Id: types.h,v 1.8 2005-01-08 13:40:14 bacon Exp $
 */

#ifndef _XP_TYPES_H_
#define _XP_TYPES_H_

#include <xp/itypes.h>

typedef xp_uint8_t  xp_byte_t;

#ifdef XP_HAVE_WCHAR_T
// TODO: make it configurable from outside
/*
#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;
*/

#define XP_CHAR_IS_WCHAR
typedef xp_wchar_t  xp_char_t;
typedef xp_wcint_t  xp_cint_t;

#else
#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;
#endif

#endif
