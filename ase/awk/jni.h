/*
 * $Id: jni.h,v 1.7 2006-10-24 04:10:12 bacon Exp $
 */

#ifndef _ASE_AWK_JNI_H_
#define _ASE_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_ase_awk_Awk_open (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif

#endif
