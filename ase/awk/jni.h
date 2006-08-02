/*
 * $Id: jni.h,v 1.1 2006-08-02 15:03:49 bacon Exp $
 */

#ifndef _XP_AWK_JNI_H_
#define _XP_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_AWK_open (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_AWK_close (JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif

#endif
