/*
 * $Id: types.h,v 1.7 2005-01-08 12:25:59 bacon Exp $
 */

#ifndef _XP_TYPES_H_
#define _XP_TYPES_H_

#ifdef _WIN32
#else
#include <xp/itypes.h>
#endif

typedef xp_uint8_t  xp_byte_t;

/*
#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;
*/

#define XP_CHAR_IS_WCHAR
typedef xp_wchar_t  xp_char_t;
typedef xp_wcint_t  xp_cint_t;

#endif
