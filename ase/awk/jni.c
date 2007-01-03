/*
 * $Id: jni.c,v 1.45 2007-01-03 09:51:51 bacon Exp $
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <stdarg.h>
#include <math.h>
#include <ase/awk/jni.h>
#include <ase/awk/awk_i.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifndef ASE_CHAR_IS_WCHAR
	#error this module supports ASE_CHAR_IS_WCHAR only
#endif

#define CLASS_EXCEPTION "ase/awk/Exception"
#define CLASS_EXTIO     "ase/awk/Extio"
#define FIELD_HANDLE    "handle"

#define MSG_SIZE 256

enum
{
	SOURCE_READ = 1,
	SOURCE_WRITE = 2
};

static ase_ssize_t __read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t __write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t __process_extio (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

typedef struct srcio_data_t srcio_data_t;
typedef struct runio_data_t runio_data_t;
typedef struct run_data_t   run_data_t;

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

struct run_data_t
{
	JNIEnv* env;
	jobject obj;

	jclass string_class;
	jclass integer_class;
	jclass long_class;
	jclass short_class;
	jclass float_class;
	jclass double_class;
	jclass object_class;

	jmethodID integer_init;
	jmethodID long_init;
	jmethodID short_init;
	jmethodID float_init;
	jmethodID double_init;

	jmethodID integer_value;
	jmethodID long_value;
	jmethodID short_value;
	jmethodID float_value;
	jmethodID double_value;
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
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

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
	jfieldID fid_handle;
	jthrowable except;
	ase_awk_t* awk;
	ase_awk_sysfns_t sysfns;
	int opt, errnum;
	
	memset (&sysfns, 0, sizeof(sysfns));
	sysfns.malloc = __awk_malloc;
	sysfns.realloc = __awk_realloc;
	sysfns.free = __awk_free;

	sysfns.is_upper  = iswupper;
	sysfns.is_lower  = iswlower;
	sysfns.is_alpha  = iswalpha;
	sysfns.is_digit  = iswdigit;
	sysfns.is_xdigit = iswxdigit;
	sysfns.is_alnum  = iswalnum;
	sysfns.is_space  = iswspace;
	sysfns.is_print  = iswprint;
	sysfns.is_graph  = iswgraph;
	sysfns.is_cntrl  = iswcntrl;
	sysfns.is_punct  = iswpunct;
	sysfns.to_upper  = towupper;
	sysfns.to_lower  = towlower;

	sysfns.memcpy = memcpy;
	sysfns.memset = memset;
	sysfns.pow = __awk_pow;
	sysfns.sprintf = __awk_sprintf;
	sysfns.aprintf = __awk_aprintf;
	sysfns.dprintf = __awk_dprintf;
	sysfns.abort = abort;

	awk = ase_awk_open (&sysfns, &errnum);
	if (awk == NULL)
	{
		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;
		(*env)->ThrowNew (env, except, "cannot create awk"); 
		(*env)->DeleteLocalRef (env, except);
		return;
	}

	class = (*env)->GetObjectClass(env, obj);
	fid_handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (fid_handle == NULL) 
	{
		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;
		(*env)->ThrowNew (env, except, "cannot find the handle field"); 
		(*env)->DeleteLocalRef (env, except);
		return;
	}

	(*env)->SetLongField (env, obj, fid_handle, (jlong)awk);

	opt = ASE_AWK_EXPLICIT | ASE_AWK_UNIQUEFN | ASE_AWK_SHADING |
		ASE_AWK_IMPLICIT | ASE_AWK_SHIFT | ASE_AWK_IDIV |
		ASE_AWK_EXTIO | ASE_AWK_BLOCKLESS | ASE_AWK_HASHSIGN | 
		ASE_AWK_NEXTOFILE;
	ase_awk_setopt (awk, opt);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID fid_handle;
	
	class = (*env)->GetObjectClass(env, obj);
	fid_handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (fid_handle == NULL) 
	{
		/* something wrong. should not happen */
		/* TODO: should it throw an exception??? */
		return;
	}

	ase_awk_close ((ase_awk_t*)(*env)->GetLongField (env, obj, fid_handle));
	(*env)->SetLongField (env, obj, fid_handle, (jlong)0);
}

static jint __java_get_max_depth (JNIEnv* env, jobject obj, const char* name)
{
	jclass class;
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, name, "()I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) return -1;

	ret = (*env)->CallIntMethod (env, obj, mid);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid_handle;
	jthrowable except;
	jint depth;

	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	srcio_data_t srcio_data;

	class = (*env)->GetObjectClass (env, obj);
	fid_handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (fid_handle == NULL) 
	{
		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;
		(*env)->ThrowNew (env, except, "cannot find the handle field"); 
		(*env)->DeleteLocalRef (env, except);
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, fid_handle);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = &srcio_data;

	depth = __java_get_max_depth (env, obj, "getMaxParseDepth");
	if (depth < 0) depth = 0;
	ase_awk_setmaxparsedepth (awk, 
		ASE_AWK_DEPTH_BLOCK | ASE_AWK_DEPTH_EXPR, depth);

	if (ase_awk_parse (awk, &srcios) == -1)
	{
		char msg[MSG_SIZE];
		int n;

		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;

		n = snprintf (msg, sizeof(msg), 
			"parse error at line %d: %S", 
			ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));
		if (n < 0 || n >= sizeof(msg)) msg[sizeof(msg)-1] = '\0';

		(*env)->ThrowNew (env, except, msg);
		(*env)->DeleteLocalRef (env, except);
		return;
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID fid_handle;
	jthrowable except;

	ase_awk_t* awk;
	ase_awk_runios_t runios;
	runio_data_t runio_data;
	run_data_t run_data;

	class = (*env)->GetObjectClass (env, obj);
	fid_handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (fid_handle == 0) 
	{
		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;
		(*env)->ThrowNew (env, except, "cannot find the handle field"); 
		(*env)->DeleteLocalRef (env, except);
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, fid_handle);

	run_data.env = env;
	run_data.obj = obj;

	/* would global reference be necessary? */
	run_data.string_class = (*env)->FindClass (env, "java/lang/String");
	run_data.integer_class = (*env)->FindClass (env, "java/lang/Integer");
	run_data.long_class = (*env)->FindClass (env, "java/lang/Long");
	run_data.short_class = (*env)->FindClass (env, "java/lang/Short");
	run_data.float_class = (*env)->FindClass (env, "java/lang/Float");
	run_data.double_class = (*env)->FindClass (env, "java/lang/Double");
	run_data.object_class = (*env)->FindClass (env, "java/lang/Object");

	ASE_AWK_ASSERT (awk, run_data.string_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.integer_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.short_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.long_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.float_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.double_class != NULL);
	ASE_AWK_ASSERT (awk, run_data.object_class != NULL);

	run_data.integer_init = (*env)->GetMethodID (
		env, run_data.integer_class, "<init>", "(I)V");
	run_data.long_init = (*env)->GetMethodID (
		env, run_data.long_class, "<init>", "(J)V");
	run_data.short_init = (*env)->GetMethodID (
		env, run_data.short_class, "<init>", "(S)V");
	run_data.float_init = (*env)->GetMethodID (
		env, run_data.float_class, "<init>", "(F)V");
	run_data.double_init = (*env)->GetMethodID (
		env, run_data.double_class, "<init>", "(D)V");

	ASE_AWK_ASSERT (awk, run_data.integer_init != NULL);
	ASE_AWK_ASSERT (awk, run_data.long_init != NULL);
	ASE_AWK_ASSERT (awk, run_data.short_init != NULL);
	ASE_AWK_ASSERT (awk, run_data.float_init != NULL);
	ASE_AWK_ASSERT (awk, run_data.double_init != NULL);

	run_data.integer_value = (*env)->GetMethodID (
		env, run_data.integer_class, "intValue", "()I");
	run_data.long_value = (*env)->GetMethodID (
		env, run_data.long_class, "longValue", "()J");
	run_data.short_value = (*env)->GetMethodID (
		env, run_data.short_class, "shortValue", "()S");
	run_data.float_value = (*env)->GetMethodID (
		env, run_data.float_class, "floatValue", "()F");
	run_data.double_value = (*env)->GetMethodID (
		env, run_data.double_class, "doubleValue", "()D");
	
	ASE_AWK_ASSERTX (awk, run_data.integer_value != NULL,
		"The Integer class must has the method - int intValue()");
	ASE_AWK_ASSERTX (awk, run_data.long_value != NULL,
		"The Long class must has the method - long longValue()");
	ASE_AWK_ASSERTX (awk, run_data.short_value != NULL,
		"The Short class must has the method - short shortValue()");
	ASE_AWK_ASSERTX (awk, run_data.float_value != NULL, 
		"The Float class must has the method - float floatValue()");
	ASE_AWK_ASSERTX (awk, run_data.double_value != NULL, 
		"The Double class must has the method - double doubleValue()");

	runio_data.env = env;
	runio_data.obj = obj;

	runios.pipe = __process_extio;
	runios.coproc = ASE_NULL;
	runios.file = __process_extio;
	runios.console = __process_extio;
	runios.custom_data = &runio_data;

	// TODO:
	//depth = __java_get_max_depth (env, obj, "getMaxRunDepth");
	// setMaxRunDepth...

	if (ase_awk_run (awk, 
		ASE_NULL, &runios, ASE_NULL, ASE_NULL, &run_data) == -1)
	{
		char msg[MSG_SIZE];
		int n;

		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return; /* exception thrown */

		n = snprintf (msg, sizeof(msg), "%S", 
			ase_awk_geterrstr(ase_awk_geterrnum(awk)));
		if (n < 0 || n >= sizeof(msg)) msg[sizeof(msg)-1] = '\0';

		(*env)->ThrowNew (env, except, msg);
		(*env)->DeleteLocalRef (env, except);

		(*env)->DeleteLocalRef (env, run_data.integer_class);
		(*env)->DeleteLocalRef (env, run_data.long_class);
		(*env)->DeleteLocalRef (env, run_data.float_class);
		(*env)->DeleteLocalRef (env, run_data.double_class);
		(*env)->DeleteLocalRef (env, run_data.string_class);
		(*env)->DeleteLocalRef (env, run_data.object_class);
		return;
	}

	(*env)->DeleteLocalRef (env, run_data.integer_class);
	(*env)->DeleteLocalRef (env, run_data.long_class);
	(*env)->DeleteLocalRef (env, run_data.short_class);
	(*env)->DeleteLocalRef (env, run_data.float_class);
	(*env)->DeleteLocalRef (env, run_data.double_class);
	(*env)->DeleteLocalRef (env, run_data.string_class);
	(*env)->DeleteLocalRef (env, run_data.object_class);
}

static ase_ssize_t __java_open_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, "openSource", "(I)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __java_close_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, "closeSource", "(I)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __java_read_source (
	JNIEnv* env, jobject obj, ase_char_t* buf, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, "readSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
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
	return i;
}

static ase_ssize_t __java_write_source (
	JNIEnv* env, jobject obj, ase_char_t* buf, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret;
	jthrowable thrown;
	ase_size_t i;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, "writeSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)buf[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, array);
	return ret;
}

static ase_ssize_t __java_open_extio (
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
	
	extio_class = (*env)->FindClass (env, CLASS_EXTIO);
	if (extio_class == NULL) return -1;

	/* get the constructor */
	extio_cons = (*env)->GetMethodID (
		env, extio_class, "<init>", "(Ljava/lang/String;IIJ)V");
	if (extio_cons == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* get the method - meth */
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* construct the name */
	extio_name = (*env)->NewString (
		env, extio->name, ase_awk_strlen(extio->name));
	if (extio_name == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* construct the extio object */
	extio_object = (*env)->NewObject (
		env, extio_class, extio_cons, 
		extio_name, extio->type & 0xFF, extio->mode, extio->run);
	(*env)->DeleteLocalRef (env, extio_class);
	if (extio_object == NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_name);
		return -1;
	}

	(*env)->DeleteLocalRef (env, extio_name);

	/* execute the method */
	ret = (*env)->CallIntMethod (env, obj, mid, extio_object);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	if (ret >= 0) 
	{
		/* ret == -1 failed to open the stream
		 * ret ==  0 opened the stream and reached its end 
		 * ret ==  1 opened the stream. */
		extio->handle = (*env)->NewGlobalRef (env, extio_object);
		/* TODO: close it...
		if (extio->handle == NULL) 
		{
			close it again...
			ret = -1;
		}
		*/
	}

	(*env)->DeleteLocalRef (env, extio_object);
	return ret;
}

static ase_ssize_t __java_close_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
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

	if (ret >= 0) 
	{
		/* ret == -1  failed to close the stream
		 * ret ==  0  closed the stream */
		(*env)->DeleteGlobalRef (env, extio->handle);
		extio->handle = NULL;
	}

	return ret;
}

static ase_ssize_t __java_read_extio (
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
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
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
	return ret;
}

static ase_ssize_t __java_write_extio (
	JNIEnv* env, jobject obj, char* meth,
	ase_awk_extio_t* extio, ase_char_t* data, ase_size_t size)
{
	jclass class; 
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret;
	ase_size_t i;
	jthrowable thrown;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
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
	return ret;
}


static ase_ssize_t __java_flush_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
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

	return ret;
}

static ase_ssize_t __java_next_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jmethodID mid;
	jthrowable thrown;
	jint ret;
	
	class = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (mid == NULL) 
	{
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

	return ret;
}

static ase_ssize_t __read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return __java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		return __java_read_source (
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
		return __java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return __java_write_source (
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
		return __java_open_extio (
			runio_data->env, runio_data->obj, 
			"openExtio", epa);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return __java_close_extio (
			runio_data->env, runio_data->obj, 
			"closeExtio", epa);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		return __java_read_extio (
			runio_data->env, runio_data->obj, 
			"readExtio", epa, data, size);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return __java_write_extio (
			runio_data->env, runio_data->obj, 
			"writeExtio", epa, data, size);
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		return __java_flush_extio (
			runio_data->env, runio_data->obj,
			"flushExtio", epa);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		return __java_next_extio (
			runio_data->env, runio_data->obj, 
			"nextExtio", epa);
	}

	return -1;
}

static int __handle_bfn (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	jclass class; 
	jmethodID method;
	jthrowable thrown;
	jstring name;
	const char* name_utf;
	run_data_t* run_data;
	JNIEnv* env; 
	jobject obj;
	jint i, nargs;
	jobjectArray args;
	jobject arg, ret;
	ase_awk_val_t* v;

	run_data = ase_awk_getruncustomdata (run);
	nargs = ase_awk_getnargs (run);

	env = run_data->env;
	obj = run_data->obj;

	name = (*env)->NewString (env, fnm, fnl);
	if (name == NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}
	name_utf = (*env)->GetStringUTFChars (env, name, JNI_FALSE);
	if (name_utf == NULL)
	{
		(*env)->DeleteLocalRef (env, name);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	class = (*env)->GetObjectClass(env, obj);
	method = (*env)->GetMethodID (
		env, class, name_utf, 
		"(J[Ljava/lang/Object;)Ljava/lang/Object;");
	(*env)->DeleteLocalRef (env, class);
	(*env)->ReleaseStringUTFChars (env, name, name_utf);
	(*env)->DeleteLocalRef (env, name);
	if (method == NULL) 
	{
		/* if the method is not found, the exception is thrown.
		 * so clear it to prevent it from being thrown */
		if ((*env)->ExceptionOccurred (env))
			(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNUSER);
		return -1;
	}

	args = (*env)->NewObjectArray (
		env, nargs, run_data->object_class, NULL);
	if (args == NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		v = ase_awk_getarg (run, i);

		arg = NULL;

		if (v->type == ASE_AWK_VAL_INT)
		{
			jlong jv;

			jv = ((ase_awk_val_int_t*)v)->val;
			arg = (*env)->NewObject (env, 
				run_data->long_class, 
				run_data->long_init, jv);
		}
		else if (v->type == ASE_AWK_VAL_REAL)
		{
			jdouble jv;

			jv = ((ase_awk_val_real_t*)v)->val;
			arg = (*env)->NewObject (env, 
				run_data->double_class, 
				run_data->double_init, jv);
		}
		else if (v->type == ASE_AWK_VAL_STR)
		{
			arg = (*env)->NewString (env, 
				((ase_awk_val_str_t*)v)->buf, 
				((ase_awk_val_str_t*)v)->len);
		}

		if (v->type != ASE_AWK_VAL_NIL && arg == NULL)
		{
			(*env)->DeleteLocalRef (env, args);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->SetObjectArrayElement (env, args, i, arg);
		if (arg != NULL) (*env)->DeleteLocalRef (env, arg);
	}

	ret = (*env)->CallObjectMethod (env, obj, method, (jlong)run, args);
	thrown = (*env)->ExceptionOccurred (env);
	if (thrown)
	{
(*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, args);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);

		// TODO:
		//ase_awk_setrunerror (run, ASE_AWK_EBFNIMPL, "EXCEPTION:....");
		return -1;
	}

	(*env)->DeleteLocalRef (env, args);

	if (ret == NULL)
	{
		ase_awk_setretval (run, ase_awk_val_nil);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->integer_class))
	{
		jint jv = (*env)->CallIntMethod (
			env, ret, run_data->integer_value);

		v = ase_awk_makeintval (run, jv);
		if (v == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->long_class))
	{
		jlong jv = (*env)->CallLongMethod (
			env, ret, run_data->long_value);

		v = ase_awk_makeintval (run, jv);
		if (v == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->short_class))
	{
		jshort jv = (*env)->CallShortMethod (
			env, ret, run_data->short_value);

		v = ase_awk_makeintval (run, jv);
		if (v == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->float_class))
	{
		jfloat jv = (*env)->CallFloatMethod (
			env, ret, run_data->float_value);
		v = ase_awk_makerealval (run, jv);
		if (v == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->double_class))
	{
		jdouble jv = (*env)->CallDoubleMethod (
			env, ret, run_data->double_value);
		v = ase_awk_makerealval (run, jv);
		if (v == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else if ((*env)->IsInstanceOf (env, ret, run_data->string_class))
	{
		jsize len;
		const jchar* ptr;

		len = (*env)->GetStringLength (env, ret);
		ptr = (*env)->GetStringChars (env, ret, JNI_FALSE);
		if (ptr == NULL)
		{
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		v = ase_awk_makestrval (run, ptr, len);
		if (v == NULL)
		{
			(*env)->ReleaseStringChars (env, ret, ptr);
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->ReleaseStringChars (env, ret, ptr);
		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setretval (run, v);
	}
	else
	{
		(*env)->DeleteLocalRef (env, ret);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNUSER);
		return -1;
	}

	return 0;
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_addbfn (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args)
{
	jclass class; 
	jfieldID handle;
	jint n;

	ase_awk_t* awk;
	const jchar* ptr;
	jsize len;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) return -1;

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	len = (*env)->GetStringLength (env, name);

	n = (ase_awk_addbfn (awk, ptr, len, 0, 
		min_args, max_args, ASE_NULL, __handle_bfn) == NULL)? -1: 0;
	(*env)->ReleaseStringChars (env, name, ptr);

	return n;
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_delbfn (
	JNIEnv* env, jobject obj, jstring name)
{
	jclass class; 
	jfieldID handle;
	jint n;

	ase_awk_t* awk;
	const jchar* ptr;
	jsize len;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) return -1; /* should never happen */

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	n = ase_awk_delbfn (awk, ptr, len);
	(*env)->ReleaseStringChars (env, name, ptr);

	return n;
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong run_id, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)run_id;
	const jchar* ptr;
	jsize len;
	jint n;

	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	len = (*env)->GetStringLength (env, name);
	n = ase_awk_setfilename (run, ptr, len);
	(*env)->ReleaseStringChars (env, name, ptr);
	return n;
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong run_id, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)run_id;
	const jchar* ptr;
	jsize len;
	jint n;

	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	len = (*env)->GetStringLength (env, name);
	n = ase_awk_setofilename (run, ptr, len);
	(*env)->ReleaseStringChars (env, name, ptr);
	return n;
}

JNIEXPORT jobject JNICALL Java_ase_awk_Awk_strtonum (
	JNIEnv* env, jobject obj, jlong runid, jstring str)
{
	const jchar* ptr;
	jsize len;
	jint n;
	ase_long_t lv;
	ase_real_t rv;
	jobject ret;
	run_data_t* run_data;

	ptr = (*env)->GetStringChars (env, str, JNI_FALSE);
	len = (*env)->GetStringLength (env, str);
	n = ase_awk_strtonum ((ase_awk_run_t*)runid, ptr, len, &lv, &rv);
	(*env)->ReleaseStringChars (env, str, ptr);

	run_data = ase_awk_getruncustomdata ((ase_awk_run_t*)runid);
	if (n == 0)
	{
		ret = (*env)->NewObject (env,
			run_data->long_class, 
			run_data->long_init, (jlong)lv);
	}
	else
	{
		ret = (*env)->NewObject (env,
			run_data->double_class, 
			run_data->double_init, (jdouble)rv);
	}

	/*
	{
		jthrowable except;
		except = (*env)->FindClass (env, CLASS_EXCEPTION);
		if (except == NULL) return;
		(*env)->ThrowNew (env, except, "cannot create awk"); 
		(*env)->DeleteLocalRef (env, except);
	}
	*/

	return ret;
}

