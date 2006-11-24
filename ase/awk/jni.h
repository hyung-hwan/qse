/*
 * $Id: jni.h,v 1.9 2006-11-24 13:20:49 bacon Exp $
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
JNIEXPORT int JNICALL Java_ase_awk_Awk_setfilename (JNIEnv*, jobject, jlong, jstring);
JNIEXPORT int JNICALL Java_ase_awk_Awk_setofilename (JNIEnv*, jobject, jlong, jstring);

#ifdef __cplusplus
}
#endif

#endif
