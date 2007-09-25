/*
 * $Id: misc.h,v 1.2 2007/09/24 11:22:22 bacon Exp $
 *
 * {License}
 */

#ifndef _MISC_H_
#define _MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

char* unicode_to_multibyte (const wchar_t* in, int inlen, int* outlen);

#ifdef __cplusplus
}
#endif

#endif
