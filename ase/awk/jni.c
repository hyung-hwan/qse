/*
 * $Id: jni.c,v 1.8 2006-10-15 15:45:41 bacon Exp $
 */

#include <xp/awk/jni.h>
#include <xp/awk/awk.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <stdio.h>

#define EXCEPTION_AWK "xpkit/xpj/awk/AwkException"
#define FIELD_AWK     "__awk"

enum
{
	SOURCE_READ = 1,
	SOURCE_WRITE = 2
};

/* TODO: what if xp_char_t is xp_mchar_t??? */

static xp_ssize_t __read_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);
static xp_ssize_t __write_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);
static xp_ssize_t __process_extio_console (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);
static xp_ssize_t __process_extio_file (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

typedef struct srcio_data_t srcio_data_t;
typedef struct runio_data_t runio_data_t;

struct srcio_data_t
{
	JNIEnv* env;
	jobject obj;
};

struct runio_data_t
{
	JNIEnv* env;
	jobject obj;
};

static void* __awk_malloc (xp_size_t n, void* custom_data)
{
	return malloc (n);
}

static void* __awk_realloc (void* ptr, xp_size_t n, void* custom_data)
{
	return realloc (ptr, n);
}

static void __awk_free (void* ptr, void* custom_data)
{
	free (ptr);
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_open (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid;
	jthrowable except;
	xp_awk_t* awk;
	xp_awk_syscas_t syscas;
	int opt;
	
	class = (*env)->GetObjectClass(env, obj);

	memset (&syscas, 0, sizeof(syscas));
	syscas.malloc = __awk_malloc;
	syscas.realloc = __awk_realloc;
	syscas.free = __awk_free;

	syscas.is_upper  = iswupper;
	syscas.is_lower  = iswlower;
	syscas.is_alpha  = iswalpha;
	syscas.is_digit  = iswdigit;
	syscas.is_xdigit = iswxdigit;
	syscas.is_alnum  = iswalnum;
	syscas.is_space  = iswspace;
	syscas.is_print  = iswprint;
	syscas.is_graph  = iswgraph;
	syscas.is_cntrl  = iswcntrl;
	syscas.is_punct  = iswpunct;
	syscas.to_upper  = towupper;
	syscas.to_lower  = towlower;

	syscas.memcpy = memcpy;
	syscas.memset = memset;
#ifdef _WIN32
	syscas.sprintf = _snwprintf;
	syscas.dprintf = wprintf;
	syscas.abort = abort;
#else
	/* TODO: */
	syscas.sprintf = XXXXX;
	syscas.dprintf = XXXXX;
	syscas.abort = abort;
#endif

	awk = xp_awk_open (&syscas);
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

printf ("__awk(native) done => %u, 0x%X\n", awk, awk);
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

printf ("close (native) done\n");
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
printf ("parse error.......\n");
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "Parse Error ...");
printf ("parse error -> line [%d] %S\n", xp_awk_getsrcline(awk), xp_awk_geterrstr(xp_awk_geterrnum(awk)));
		return;
	}
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_run (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid;
	jthrowable except;

	xp_awk_t* awk;
	xp_awk_runios_t runios;
	runio_data_t runio_data;

	class = (*env)->GetObjectClass (env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	awk = (xp_awk_t*) (*env)->GetLongField (env, obj, fid);

	runio_data.env = env;
	runio_data.obj = obj;

	runios.pipe = XP_NULL;
	runios.coproc = XP_NULL;
	runios.file = __process_extio_file;
	runios.console = __process_extio_console;
	runios.custom_data = &runio_data;

	if (xp_awk_run (awk, &runios, XP_NULL, XP_NULL) == -1)
	{
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "Run Error ...");
		return;
	}
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_Awk_set_1extio_1handle (
	JNIEnv* env, jobject obj, jlong extio, jobject handle)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)extio;
	epa->handle = (void*)handle;
}

static xp_ssize_t __call_java_open_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "open_source", "(I)I");
	if (mid == 0) return -1;

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __call_java_close_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "close_source", "(I)I");
	if (mid == 0) return -1;

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __call_java_read_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "read_source", "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) return -1;

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < ret; i++) buf[i] = (xp_char_t)tmp[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	return i;
}

static xp_ssize_t __call_java_write_source (
	JNIEnv* env, jobject obj, xp_char_t* buf, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "write_source", "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) return -1;

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)buf[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __call_java_open_extio (
	JNIEnv* env, jobject obj, char* meth, const xp_char_t* name)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	if (name == XP_NULL)
	{
		mid = (*env)->GetMethodID (env, class, meth, "()I");
		if (mid == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid);
	}
	else
	{
		jstring name_str;

		mid = (*env)->GetMethodID (
			env, class, meth, "(Ljava/lang/String;)I");
		if (mid == 0) return -1;

		name_str = (*env)->NewString (env, name, xp_awk_strlen(name));
		if (name_str == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid, name_str);
	}

	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __call_java_close_extio (
	JNIEnv* env, jobject obj, char* meth, const xp_char_t* name)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	if (name == XP_NULL)
	{
		mid = (*env)->GetMethodID (env, class, meth, "()I");
		if (mid == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid);
	}
	else
	{
		jstring name_str;

		mid = (*env)->GetMethodID (
			env, class, meth, "(Ljava/lang/String;)I");
		if (mid == 0) return -1;

		name_str = (*env)->NewString (env, name, xp_awk_strlen(name));
		if (name_str == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid, name_str);
	}

	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __call_java_read_extio (
	JNIEnv* env, jobject obj, char* meth, xp_char_t* buf, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, meth, "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) return -1;

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < ret; i++) buf[i] = (xp_char_t)tmp[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	return ret;
}

static xp_ssize_t __call_java_write_extio (
	JNIEnv* env, jobject obj, char* meth, xp_char_t* data, xp_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	xp_ssize_t i;
	jchar* tmp;
	jint ret;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, meth, "([CI)I");
	if (mid == 0) return -1;

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) return -1;

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)data[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static xp_ssize_t __read_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == XP_AWK_IO_OPEN) 
	{
		return __call_java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		return __call_java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == XP_AWK_IO_READ)
	{
		return __call_java_read_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static xp_ssize_t __write_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == XP_AWK_IO_OPEN) 
	{
		return __call_java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		return __call_java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == XP_AWK_IO_WRITE)
	{
		return __call_java_write_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static xp_ssize_t __process_extio_console (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	if (cmd == XP_AWK_IO_OPEN)
	{
		return __call_java_open_extio (
			runio_data->env, runio_data->obj, 
			"open_console", XP_NULL);
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		return __call_java_close_extio (
			runio_data->env, runio_data->obj, 
			"close_console", XP_NULL);
	}

	else if (cmd == XP_AWK_IO_READ)
	{
		return __call_java_read_extio (
			runio_data->env, runio_data->obj, "read_console",
			data, size);
	}
	else if (cmd == XP_AWK_IO_WRITE)
	{
		return __call_java_write_extio (
			runio_data->env, runio_data->obj, "write_console",
			data, size);
	}
#if 0
	else if (cmd == XP_AWK_IO_FLUSH)
	{
		return __call_java_flush_extio (
			runio_data->env, runio_data->obj, "flush_console",
			data, size);
	}
	else if (cmd == XP_AWK_IO_NEXT)
	{
		return __call_java_next_extio (
			runio_data->env, runio_data->obj, "flush_console",
			data, size);
	}
#endif

	return -1;
}

static xp_ssize_t __process_extio_file (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	if (cmd == XP_AWK_IO_OPEN)
	{
		return __call_java_open_extio (
			runio_data->env, runio_data->obj, 
			"open_file", epa->name);
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		return __call_java_close_extio (
			runio_data->env, runio_data->obj, 
			"close_file", epa->name);
	}

	return -1;
}
