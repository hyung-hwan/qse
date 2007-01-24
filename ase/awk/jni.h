/*
 * $Id: jni.h,v 1.16 2007-01-24 11:54:16 bacon Exp $
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

JNIEXPORT void JNICALL Java_ase_awk_Awk_addbfn (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args);
JNIEXPORT void JNICALL Java_ase_awk_Awk_delbfn (
	JNIEnv* env, jobject obj, jstring name);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getmaxdepth (
	JNIEnv* env, jobject obj, jint id);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (
	JNIEnv* env, jobject obj, jint ids, jint depth);

JNIEXPORT void JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);
JNIEXPORT jobject JNICALL Java_ase_awk_Awk_strtonum (
	JNIEnv* env, jobject obj, jlong runid, jstring str);

#ifdef __cplusplus
}
#endif

#endif
