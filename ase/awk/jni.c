/*
 * $Id: jni.c,v 1.4 2006-08-06 15:02:55 bacon Exp $
 */

#include "jni.h"
#include "awk.h"
#include "sa.h"

#define EXCEPTION_AWK "xpkit/xpj/awk/AwkException"
#define FIELD_AWK     "__awk"

static xp_ssize_t __read_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);
static xp_ssize_t __write_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

static xp_ssize_t __call_java_read_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size);
static xp_ssize_t __call_java_write_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size);

typedef struct srcio_data_t srcio_data_t;

struct srcio_data_t
{
	JNIEnv* env;
	jobject obj;
};


JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_open (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid;
	jthrowable except;
	xp_awk_t* awk;
	int opt;
	
	class = (*env)->GetObjectClass(env, obj);

	awk = xp_awk_open ();
	if (awk == NULL)
	{
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "cannot create awk"); 
		return;
	}

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	(*env)->SetLongField (env, obj, fid, (jlong)awk);

	opt = XP_AWK_EXPLICIT | XP_AWK_UNIQUE | XP_AWK_DBLSLASHES |
		XP_AWK_SHADING | XP_AWK_IMPLICIT | XP_AWK_SHIFT | 
		XP_AWK_EXTIO | XP_AWK_BLOCKLESS;
	xp_awk_setopt (awk, opt);

xp_printf (XP_T("__awk(native) done => %u, 0x%X\n"), awk, awk);
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_close (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid;
	
	class = (*env)->GetObjectClass(env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	xp_awk_close ((xp_awk_t*) (*env)->GetLongField (env, obj, fid));
	(*env)->SetLongField (env, obj, fid, (jlong)0);

xp_printf (XP_T("close (native) done\n"));
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_parse (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid;
	jthrowable except;

	xp_awk_t* awk;
	xp_awk_srcios_t srcios;
	srcio_data_t srcio_data;

	class = (*env)->GetObjectClass (env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	awk = (xp_awk_t*) (*env)->GetLongField (env, obj, fid);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = &srcio_data;

	if (xp_awk_parse (awk, &srcios) == -1)
	{
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "ERROR ....");
xp_printf (XP_T("parse error -> line [%d] %s\n"), xp_awk_getsrcline(awk), xp_awk_geterrstr(awk));
		return;
	}
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_run (JNIEnv* env, jobject obj)
{
}

static xp_ssize_t __read_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count)
{
	srcio_data_t* srcio_data;

	srcio_data = (srcio_data_t*)arg;

	if (cmd == XP_AWK_IO_OPEN || cmd == XP_AWK_IO_CLOSE) return 0;
	if (cmd == XP_AWK_IO_READ)
	{
		return __call_java_read_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static xp_ssize_t __write_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count)
{
	srcio_data_t* srcio_data;

	srcio_data = (srcio_data_t*)arg;

	if (cmd == XP_AWK_IO_OPEN || cmd == XP_AWK_IO_CLOSE) return 0;
	if (cmd == XP_AWK_IO_WRITE)
	{
		return __call_java_write_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static xp_ssize_t __call_java_read_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	xp_ssize_t i, n;
	jchar* tmp;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "read_source", "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, 1024);
	if (array == NULL) return -1;

	n = (*env)->CallIntMethod (env, obj, mid, array, 1024);

// TODO: how to handle error..
// TODO: what is xp_char_t is xp_mchar_t? use UTF8 ???
	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < n && i < size; i++) buf[i] = (xp_char_t)tmp[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	return i;
}

static xp_ssize_t __call_java_write_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	xp_ssize_t i;
	jchar* tmp;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "write_source", "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) return -1;

// TODO: how to handle error..
// TODO: what is xp_char_t is xp_mchar_t? use UTF8 ???
	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)buf[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	return (*env)->CallIntMethod (env, obj, mid, array, size);
}
