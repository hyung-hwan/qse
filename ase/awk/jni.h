/*
 * $Id: jni.h,v 1.13 2007/10/23 15:18:47 bacon Exp $
 *
 * {License}
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
JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj, jlong awkid);
JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj, jlong awkid);
JNIEXPORT void JNICALL Java_ase_awk_Awk_run (
	JNIEnv* env, jobject obj, jlong awkid, jstring mfn, jobjectArray args);
JNIEXPORT void JNICALL Java_ase_awk_Awk_stop (JNIEnv* env, jobject obj, jlong awkid);

JNIEXPORT void JNICALL Java_ase_awk_Awk_addfunc (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args);
JNIEXPORT void JNICALL Java_ase_awk_Awk_delfunc (
	JNIEnv* env, jobject obj, jstring name);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getmaxdepth (
	JNIEnv* env, jobject obj, jlong awkid, jint id);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (
	JNIEnv* env, jobject obj, jlong awkid, jint ids, jint depth);

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getoption (
	JNIEnv* env, jobject obj, jlong awkid);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setoption (
	JNIEnv* env, jobject obj, jlong awkid, jint options);

JNIEXPORT jboolean JNICALL Java_ase_awk_Awk_getdebug (
	JNIEnv* env, jobject obj, jlong awkid);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setdebug (
	JNIEnv* env, jobject obj, jlong awkid, jboolean debug);

JNIEXPORT void JNICALL Java_ase_awk_Awk_setword (
	JNIEnv* env, jobject obj, jlong awkid, jstring ow, jstring nw);

JNIEXPORT void JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);
JNIEXPORT void JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name);

JNIEXPORT jstring JNICALL Java_ase_awk_Awk_strftime (
	JNIEnv* env, jobject obj, jstring fmt, jlong sec);
JNIEXPORT jstring JNICALL Java_ase_awk_Awk_strfgmtime (
	JNIEnv* env, jobject obj, jstring fmt, jlong sec);
JNIEXPORT jint JNICALL Java_ase_awk_Awk_system (
	JNIEnv* env, jobject obj, jstring cmd);

JNIEXPORT void JNICALL Java_ase_awk_Context_stop (JNIEnv* env, jobject obj, jlong runid);

JNIEXPORT jlong JNICALL Java_ase_awk_Argument_getintval (JNIEnv* env, jobject obj, jlong runid, jlong valid);
JNIEXPORT jdouble JNICALL Java_ase_awk_Argument_getrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid);
JNIEXPORT jstring JNICALL Java_ase_awk_Argument_getstrval (JNIEnv* env, jobject obj, jlong runid, jlong valid);
JNIEXPORT jboolean JNICALL Java_ase_awk_Argument_isindexed (JNIEnv* env, jobject obj, jlong runid, jlong valid);
JNIEXPORT jobject JNICALL Java_ase_awk_Argument_getindexed (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring index);

JNIEXPORT void JNICALL Java_ase_awk_Return_setintval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jlong newval);
JNIEXPORT void JNICALL Java_ase_awk_Return_setrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jdouble newval);

#ifdef __cplusplus
}
#endif

#endif
