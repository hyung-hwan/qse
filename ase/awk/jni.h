/*
 * $Id: jni.h,v 1.21 2007-02-01 07:23:59 bacon Exp $
 */

#ifndef _ASE_AWK_JNI_H_
#define _ASE_AWK_JNI_H_

#if defined(__APPLE__) && defined(__MACH__)
#include <JavaVM/jni.h>
#else
#include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_ase_awk_Awk_open (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_run (
	JNIEnv* env, jobject obj, jstring mfn, jobjectArray args);

JNIEXPORT void JNICALL Java_ase_awk_Awk_addbfn (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args);
JNIEXPORT void JNICALL Java_ase_awk_Awk_delbfn (
	JNIEnv* env, jobject obj, jstring name);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getmaxdepth (
	JNIEnv* env, jobject obj, jint id);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (
	JNIEnv* env, jobject obj, jint ids, jint depth);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getoption (
	JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setoption (
	JNIEnv* env, jobject obj, jint options);

JNIEXPORT jboolean JNICALL Java_ase_awk_Awk_getdebug (
	JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setdebug (
	JNIEnv* env, jobject obj, jboolean debug);

JNIEXPORT void JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);

JNIEXPORT jobject JNICALL Java_ase_awk_Awk_strtonum (
	JNIEnv* env, jobject obj, jlong runid, jstring str);
JNIEXPORT jstring JNICALL Java_ase_awk_Awk_valtostr (
	JNIEnv* env, jobject obj, jlong runid, jobject val);

#ifdef __cplusplus
}
#endif

#endif
