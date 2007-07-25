/*
 * $Id: misc.h,v 1.1 2007/07/20 09:23:37 bacon Exp $
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