/*
 * $Id: jni.h,v 1.6 2006-10-22 12:39:29 bacon Exp $
 */

#ifndef _SSE_AWK_JNI_H_
#define _SSE_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_sse_awk_Awk_open (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_sse_awk_Awk_close (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_sse_awk_Awk_parse (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_sse_awk_Awk_run (JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif

#endif
