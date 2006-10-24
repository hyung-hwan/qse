/*
 * $Id: jni.c,v 1.16 2006-10-24 04:57:29 bacon Exp $
 */

#include <ase/awk/jni.h>
#include <ase/awk/awk.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <stdio.h>

#define EXCEPTION_AWK "ase/awk/AwkException"
#define FIELD_AWK     "__awk"

enum
{
	SOURCE_READ = 1,
	SOURCE_WRITE = 2
};

/* TODO: what if ase_char_t is ase_mchar_t??? */

static ase_ssize_t __read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t __write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t __process_extio_console (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t __process_extio_file (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

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

static void* __awk_malloc (ase_size_t n, void* custom_data)
{
	return malloc (n);
}

static void* __awk_realloc (void* ptr, ase_size_t n, void* custom_data)
{
	return realloc (ptr, n);
}

static void __awk_free (void* ptr, void* custom_data)
{
	free (ptr);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_open (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid;
	jthrowable except;
	ase_awk_t* awk;
	ase_awk_syscas_t syscas;
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

	awk = ase_awk_open (&syscas);
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

	opt = ASE_AWK_EXPLICIT | ASE_AWK_UNIQUE | ASE_AWK_DBLSLASHES |
		ASE_AWK_SHADING | ASE_AWK_IMPLICIT | ASE_AWK_SHIFT | 
		ASE_AWK_EXTIO | ASE_AWK_BLOCKLESS;
	ase_awk_setopt (awk, opt);

printf ("__awk(native) done => %u, 0x%X\n", awk, awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid;
	
	class = (*env)->GetObjectClass(env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	ase_awk_close ((ase_awk_t*) (*env)->GetLongField (env, obj, fid));
	(*env)->SetLongField (env, obj, fid, (jlong)0);

printf ("close (native) done\n");
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid;
	jthrowable except;

	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	srcio_data_t srcio_data;

	class = (*env)->GetObjectClass (env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, fid);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = &srcio_data;

	if (ase_awk_parse (awk, &srcios) == -1)
	{
printf ("parse error.......\n");
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "Parse Error ...");
printf ("parse error -> line [%d] %S\n", ase_awk_getsrcline(awk), ase_awk_geterrstr(ase_awk_geterrnum(awk)));
		return;
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid;
	jthrowable except;

	ase_awk_t* awk;
	ase_awk_runios_t runios;
	runio_data_t runio_data;

	class = (*env)->GetObjectClass (env, obj);

	fid = (*env)->GetFieldID (env, class, FIELD_AWK, "J");
	if (fid == 0) return;

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, fid);

	runio_data.env = env;
	runio_data.obj = obj;

	runios.pipe = ASE_NULL;
	runios.coproc = ASE_NULL;
	runios.file = __process_extio_file;
	runios.console = __process_extio_console;
	runios.custom_data = &runio_data;

	if (ase_awk_run (awk, &runios, ASE_NULL, ASE_NULL) == -1)
	{
		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;
		(*env)->ThrowNew (env, except, "Run Error ...");
		return;
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_set_1extio (
	JNIEnv* env, jobject obj, jlong extio, jobject handle)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)extio;
	epa->handle = (void*)handle;
}

JNIEXPORT jobject JNICALL Java_ase_awk_Awk_get_1extio (
	JNIEnv* env, jobject obj, jlong extio)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)extio;
	return (jobject)epa->handle;
}

static ase_ssize_t __call_java_open_source (JNIEnv* env, jobject obj, int mode)
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

static ase_ssize_t __call_java_close_source (JNIEnv* env, jobject obj, int mode)
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

static ase_ssize_t __call_java_read_source (
	JNIEnv* env, jobject obj, ase_char_t* buf, ase_size_t size)
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
	for (i = 0; i < ret; i++) buf[i] = (ase_char_t)tmp[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	(*env)->DeleteLocalRef (env, array);
	return i;
}

static ase_ssize_t __call_java_write_source (
	JNIEnv* env, jobject obj, ase_char_t* buf, ase_size_t size)
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

	(*env)->DeleteLocalRef (env, array);
	return ret;
}

static ase_ssize_t __call_java_open_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	if (extio == ASE_NULL)
	{
		mid = (*env)->GetMethodID (env, class, meth, "()I");
		if (mid == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid);
	}
	else
	{
		/*
		jstring name_str;

		mid = (*env)->GetMethodID (
			env, class, meth, "(Ljava/lang/String;)I");
		if (mid == 0) return -1;

		name_str = (*env)->NewString (
			env, extio->name, ase_awk_strlen(extio->name));
		if (name_str == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid, name_str);

		(*env)->DeleteLocalRef (env, name_str);
		*/
		jclass extio_class;
		jmethodID extio_cons;
		jobject extio_object;

		extio_class = (*env)->FindClass (env, "ase/awk/Extio");
		if (extio_class == NULL) return -1;

		extio_cons = (*env)->GetMethodID (
			env, extio_class, "<init>", "(J)V");
		if (extio_cons == NULL) return -1;

		mid = (*env)->GetMethodID (
			env, class, meth, "(Lase/awk/Extio;)I");
		if (mid == NULL) return -1;

		extio_object = (*env)->NewObject (
			env, extio_class, extio_cons, (jlong)extio);
		if (extio_object == NULL) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid, extio_object);
	}

	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __call_java_close_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	if (extio == ASE_NULL)
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

		name_str = (*env)->NewString (
			env, extio->name, ase_awk_strlen(extio->name));
		if (name_str == 0) return -1;

		ret = (*env)->CallIntMethod (env, obj, mid, name_str);
		(*env)->DeleteLocalRef (env, name_str);
	}

	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __call_java_read_extio (
	JNIEnv* env, jobject obj, char* meth, ase_char_t* buf, ase_size_t size)
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
	for (i = 0; i < ret; i++) buf[i] = (ase_char_t)tmp[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	(*env)->DeleteLocalRef (env, array);
	return ret;
}

static ase_ssize_t __call_java_write_extio (
	JNIEnv* env, jobject obj, char* meth, ase_char_t* data, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	ase_ssize_t i;
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

	(*env)->DeleteLocalRef (env, array);
	return ret;
}

static ase_ssize_t __read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return __call_java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __call_java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		return __call_java_read_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static ase_ssize_t __write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return __call_java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __call_java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return __call_java_write_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static ase_ssize_t __process_extio_console (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		return __call_java_open_extio (
			runio_data->env, runio_data->obj, 
			"open_console", ASE_NULL);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __call_java_close_extio (
			runio_data->env, runio_data->obj, 
			"close_console", ASE_NULL);
	}

	else if (cmd == ASE_AWK_IO_READ)
	{
		return __call_java_read_extio (
			runio_data->env, runio_data->obj, "read_console",
			data, size);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return __call_java_write_extio (
			runio_data->env, runio_data->obj, "write_console",
			data, size);
	}
#if 0
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		return __call_java_flush_extio (
			runio_data->env, runio_data->obj, "flush_console",
			data, size);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		return __call_java_next_extio (
			runio_data->env, runio_data->obj, "flush_console",
			data, size);
	}
#endif

	return -1;
}

static ase_ssize_t __process_extio_file (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		return __call_java_open_extio (
			runio_data->env, runio_data->obj, 
			"open_file", epa);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __call_java_close_extio (
			runio_data->env, runio_data->obj, 
			"close_file", epa);
	}

	return -1;
}
