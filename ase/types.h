/*
 * $Id: types.h,v 1.5 2005-01-02 15:43:46 bacon Exp $
 */

#ifndef _XP_TYPES_H_
#define _XP_TYPES_H_

#include <xp/itypes.h>

typedef xp_uint8_t  xp_byte_t;

typedef xp_int8_t   xp_mchar_t;
typedef xp_int32_t  xp_mcint_t;

typedef xp_int16_t  xp_wchar_t;
typedef xp_int32_t  xp_wcint_t;

#define XP_CHAR_IS_MCHAR
typedef xp_mchar_t  xp_char_t;
typedef xp_mcint_t  xp_cint_t;

#endif
