/*
 * $Id: jni.h,v 1.3 2006-08-06 08:15:29 bacon Exp $
 */

#ifndef _XP_AWK_JNI_H_
#define _XP_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_open (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_close (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_parse (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_run (JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif

#endif
