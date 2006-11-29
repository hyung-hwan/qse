/*
 * $Id: jni.h,v 1.13 2006-11-29 14:52:06 bacon Exp $
 */

#ifndef _ASE_AWK_JNI_H_
#define _ASE_AWK_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_ase_awk_Awk_open (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv* env, jobject obj);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_addbfn (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args);
JNIEXPORT jint JNICALL Java_ase_awk_Awk_delbfn (
	JNIEnv* env, jobject obj, jstring name);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong run_id, jstring name);
JNIEXPORT jint JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong run_id, jstring name);

#ifdef __cplusplus
}
#endif

#endif
