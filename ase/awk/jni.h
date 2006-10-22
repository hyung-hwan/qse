/*
 * $Id: jni.h,v 1.5 2006-10-22 11:40:51 bacon Exp $
 */

#ifndef _SSE_AWK_JNI_H_
#define _SSE_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIESSEORT void JNICALL Java_sse_awk_Awk_open (JNIEnv*, jobject);
JNIESSEORT void JNICALL Java_sse_awk_Awk_close (JNIEnv*, jobject);
JNIESSEORT void JNICALL Java_sse_awk_Awk_parse (JNIEnv*, jobject);
JNIESSEORT void JNICALL Java_sse_awk_Awk_run (JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif

#endif
