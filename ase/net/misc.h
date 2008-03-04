/*
 * $Id: misc.h 117 2008-03-03 11:20:05Z baconevi $
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
