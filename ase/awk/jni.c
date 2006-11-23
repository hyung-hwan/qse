/*
 * $Id: jni.c,v 1.21 2006-11-23 03:31:36 bacon Exp $
 */

#include <ase/awk/jni.h>
#include <ase/awk/awk.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#ifndef ASE_CHAR_IS_WCHAR
	#error this module supports ASE_CHAR_IS_WCHAR only
#endif

#define EXCEPTION_AWK "ase/awk/Exception"
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
static ase_ssize_t __process_extio (
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

static ase_real_t __awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int __awk_sprintf (
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, len, fmt, ap);
	if (n < 0 || (ase_size_t)n >= len)
	{
		if (len > 0) buf[len-1] = ASE_T('\0');
		n = -1;
	}
#elif defined(__MSDOS__)
	/* TODO: check buffer overflow */
	n = vsprintf (buf, fmt, ap);
#else
	n = xp_vsprintf (buf, len, fmt, ap);
#endif
	va_end (ap);
	return n;
}

static void __awk_aprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, ase_countof(buf), fmt, ap);
	if (n < 0) buf[ase_countof(buf)-1] = ASE_T('\0');

	#if defined(_MSC_VER) && (_MSC_VER<1400)
	MessageBox (NULL, buf, 
		ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	#else
	MessageBox (NULL, buf, 
		ASE_T("\uB2DD\uAE30\uB9AC \uC870\uB610"), MB_OK|MB_ICONERROR);
	#endif
#elif defined(__MSDOS__)
	vprintf (fmt, ap);
#else
	xp_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void __awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

#if defined(_WIN32)
	_vftprintf (stderr, fmt, ap);
#elif defined(__MSDOS__)
	vfprintf (stderr, fmt, ap);
#else
	xp_vfprintf (stderr, fmt, ap);
#endif

	va_end (ap);
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
	syscas.pow = __awk_pow;
	syscas.sprintf = __awk_sprintf;
	syscas.aprintf = __awk_aprintf;
	syscas.dprintf = __awk_dprintf;
	syscas.abort = abort;

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
		ASE_AWK_EXTIO | ASE_AWK_BLOCKLESS | ASE_AWK_HASHSIGN | 
		ASE_AWK_NEXTOUTFILE;
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
	runios.file = __process_extio;
	runios.console = __process_extio;
	runios.custom_data = &runio_data;

	if (ase_awk_run (awk, ASE_NULL, &runios, ASE_NULL, ASE_NULL) == -1)
	{
		char msg[256];
		int n;

		except = (*env)->FindClass (env, EXCEPTION_AWK);
		if (except == 0) return;

		snprintf (msg, sizeof(msg), "%S", 
			ase_awk_geterrstr(ase_awk_geterrnum(awk)));
		if (n < 0 || n >= sizeof(msg)) msg[sizeof(msg)-1] = '\0';
		(*env)->ThrowNew (env, except, msg);
		return;
	}
}

static ase_ssize_t __call_java_open_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, "open_source", "(I)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, class);
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
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, class);
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
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

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
	(*env)->DeleteLocalRef (env, class);
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
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

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
	(*env)->DeleteLocalRef (env, class);
	return ret;
}

static ase_ssize_t __call_java_open_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jclass extio_class;
	jmethodID extio_cons;
	jobject extio_object;
	jstring extio_name;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	extio_class = (*env)->FindClass (env, "ase/awk/Extio");
	if (extio_class == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	/* get the constructor */
	extio_cons = (*env)->GetMethodID (
		env, extio_class, "<init>", "(Ljava/lang/String;IIJ)V");
	if (extio_cons == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	/* get the method - meth */
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	/* construct the name */
	extio_name = (*env)->NewString (
		env, extio->name, ase_awk_strlen(extio->name));
	if (extio_name == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	/* construct the extio object */
	extio_object = (*env)->NewObject (
		env, extio_class, extio_cons, 
		extio_name, extio->type & 0xFF, extio->mode, extio->run);
	if (extio_object == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_name);
		(*env)->DeleteLocalRef (env, extio_class);
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	/* execute the method */
	ret = (*env)->CallIntMethod (env, obj, mid, extio_object);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	if (ret != -1) 
	{
		/* ret == -1 failed to open the stream
		 * ret ==  0 opened the stream and reached its end 
		 * ret ==  1 opened the stream. */
		extio->handle = (*env)->NewGlobalRef (env, extio_object);
	}

	(*env)->DeleteLocalRef (env, extio_object);
	(*env)->DeleteLocalRef (env, extio_name);
	(*env)->DeleteLocalRef (env, extio_class);
	(*env)->DeleteLocalRef (env, class);
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

	mid = (*env)->GetMethodID (
		env, class, meth, "(Lase/awk/Extio;)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	if (ret != -1) 
	{
		/* ret == -1  failed to close the stream
		 * ret ==  0  closed the stream */
		(*env)->DeleteGlobalRef (env, extio->handle);
		extio->handle = NULL;
	}

	(*env)->DeleteLocalRef (env, class);
	return ret;
}

static ase_ssize_t __call_java_read_extio (
	JNIEnv* env, jobject obj, char* meth, 
	ase_awk_extio_t* extio, ase_char_t* buf, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	if (ret > 0)
	{
		tmp = (*env)->GetCharArrayElements (env, array, 0);
		for (i = 0; i < ret; i++) buf[i] = (ase_char_t)tmp[i]; 
		(*env)->ReleaseCharArrayElements (env, array, tmp, 0);
	}

	(*env)->DeleteLocalRef (env, array);
	(*env)->DeleteLocalRef (env, class);
	return ret;
}

static ase_ssize_t __call_java_write_extio (
	JNIEnv* env, jobject obj, char* meth,
	ase_awk_extio_t* extio, ase_char_t* data, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	ase_ssize_t i;
	jchar* tmp;
	jint ret;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)data[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, array);
	(*env)->DeleteLocalRef (env, class);
	return ret;
}

static ase_ssize_t __call_java_next_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);

	mid = (*env)->GetMethodID (
		env, class, meth, "(Lase/awk/Extio;)I");
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, class);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, class);
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

static ase_ssize_t __process_extio (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		return __call_java_open_extio (
			runio_data->env, runio_data->obj, 
			"open_extio", epa);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __call_java_close_extio (
			runio_data->env, runio_data->obj, 
			"close_extio", epa);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		return __call_java_read_extio (
			runio_data->env, runio_data->obj, 
			"read_extio", epa, data, size);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return __call_java_write_extio (
			runio_data->env, runio_data->obj, 
			"write_extio", epa, data, size);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		return __call_java_next_extio (
			runio_data->env, runio_data->obj, 
			"next_console", epa);
	}
#if 0
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		return __call_java_flush_extio (
			runio_data->env, runio_data->obj, "flush_console",
			data, size);
	}
#endif

	return -1;
}

JNIEXPORT int JNICALL Java_ase_awk_Awk_setconsolename (JNIEnv* env, jobject obj, jlong run_id, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)run_id;
	const jchar* str;
	int len, n;

	str = (*env)->GetStringChars (env, name, JNI_FALSE);
	len = (*env)->GetStringLength (env, name);
	n = ase_awk_setconsolename (run, str, len);
	(*env)->ReleaseStringChars (env, name, str);
	return n;
}


