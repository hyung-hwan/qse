/*
 * $Id: parse.h,v 1.1 2006-08-21 14:51:32 bacon Exp $
 */

#ifndef _XP_AWK_PARSE_H_
#define _XP_AWK_PARSE_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

int xp_awk_putsrcstr (xp_awk_t* awk, const xp_char_t* str);
int xp_awk_putsrcstrx (
	xp_awk_t* awk, const xp_char_t* str, xp_size_t len);

#ifdef __cplusplus
}
#endif

#endif
