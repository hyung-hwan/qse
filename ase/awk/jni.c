/*
 * $Id: jni.c,v 1.57 2007-01-27 02:55:55 bacon Exp $
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
#include <ase/awk/awk.h>
#include <ase/awk/val.h>

#include "../etc/printf.c"

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

#define CLASS_OUTOFMEMORYERROR "java/lang/OutOfMemoryError"
#define CLASS_EXCEPTION        "ase/awk/Exception"
#define CLASS_EXTIO            "ase/awk/Extio"
#define FIELD_HANDLE           "handle"

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

typedef struct awk_data_t   awk_data_t;
typedef struct srcio_data_t srcio_data_t;
typedef struct runio_data_t runio_data_t;
typedef struct run_data_t   run_data_t;

struct awk_data_t
{
	int debug;
};

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

static void* awk_malloc (ase_size_t n, void* custom_data)
{
	return malloc (n);
}

static void* awk_realloc (void* ptr, ase_size_t n, void* custom_data)
{
	return realloc (ptr, n);
}

static void awk_free (void* ptr, void* custom_data)
{
	free (ptr);
}

static ase_real_t awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static void awk_abort (void* custom_data)
{
        abort ();
}

static int awk_sprintf (
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vsprintf (buf, len, fmt, ap);
	va_end (ap);
	return n;
}

static void awk_aprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vprintf (fmt, ap);
	va_end (ap);
}

static void awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

static void throw_exception (
	JNIEnv* env, const ase_char_t* msg, jint code, jint line)
{
	jclass except_class;
	jmethodID except_cons;
	jstring except_msg;
	jthrowable except_obj;

	except_class = (*env)->FindClass (env, CLASS_EXCEPTION);
	if (except_class == NULL) 
	{
		/* the exception to be thrown by FindClass is not cleared.
		 * 1. this should not happend as the ase.awk.Exception
		 *    class should always be there.
		 * 2. if it happens, this exception may abort the entire
		 *    program as the exception is not likely to be handled
		 *    explicitly by the java program. */
		return; 
	}

	except_cons = (*env)->GetMethodID (
		env, except_class, "<init>", "(Ljava/lang/String;II)V");
	if (except_cons == NULL)
	{
		/* the potential exception to be thrown by the GetMethodID
		 * method is not cleared here for the same reason as the
		 * FindClass method above */
		(*env)->DeleteLocalRef (env, except_class);
		return;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i, len = ase_awk_strlen(msg);
		jchar* tmp = (jchar*) malloc (ASE_SIZEOF(jchar)*len);
		if (tmp == NULL)
		{
			(*env)->DeleteLocalRef (env, except_class);

			except_class = (*env)->FindClass (
				env, CLASS_OUTOFMEMORYERROR);
			if (except_class == NULL) return;

			(*env)->ThrowNew (env, except_class, "out of memory");
			(*env)->DeleteLocalRef (env, except_class);
			return;
		}

		for (i = 0; i < len; i++) tmp[i] = (jchar)msg[i];
		except_msg = (*env)->NewString (env, tmp, len);
		free (tmp);
	}
	else
	{
		except_msg = (*env)->NewString (
			env, (jchar*)msg, ase_awk_strlen(msg));
	}

	if (except_msg == NULL)
	{
		(*env)->DeleteLocalRef (env, except_class);
		return;
	}

	except_obj = (*env)->NewObject (
		env, except_class, except_cons,
		except_msg, code, line);

	(*env)->DeleteLocalRef (env, except_msg);
	(*env)->DeleteLocalRef (env, except_class);

	if (except_obj == NULL) return;

	(*env)->Throw (env, except_obj);
	(*env)->DeleteLocalRef (env, except_obj);
}

static jboolean is_debug (ase_awk_t* awk)
{
	awk_data_t* awk_data = (awk_data_t*)ase_awk_getcustomdata (awk);
	return awk_data->debug? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_open (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;
	ase_awk_sysfns_t sysfns;
	awk_data_t* awk_data;
	int opt, errnum;
	
	memset (&sysfns, 0, sizeof(sysfns));
	sysfns.malloc = awk_malloc;
	sysfns.realloc = awk_realloc;
	sysfns.free = awk_free;

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

	sysfns.memcpy  = memcpy;
	sysfns.memset  = memset;
	sysfns.pow     = awk_pow;
	sysfns.sprintf = awk_sprintf;
	sysfns.aprintf = awk_aprintf;
	sysfns.dprintf = awk_dprintf;
	sysfns.abort   = awk_abort;

	awk_data = (awk_data_t*)malloc (sizeof(awk_data_t));
	if (awk_data == NULL)
	{
		throw_exception (
			env,
			ase_awk_geterrstr(errnum), 
			errnum, 
			0);
		return;
	}

	memset (awk_data, 0, sizeof(awk_data_t));

	awk = ase_awk_open (&sysfns, awk_data, &errnum);
	if (awk == NULL)
	{
		free (sysfns.custom_data);
		throw_exception (
			env,
			ase_awk_geterrstr(errnum), 
			errnum, 
			0);
		return;
	}

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */

		ase_awk_close (awk);
		free (awk_data);
		return;
	}

	(*env)->SetLongField (env, obj, handle, (jlong)awk);

	opt = ASE_AWK_EXPLICIT | ASE_AWK_UNIQUEFN | ASE_AWK_SHADING |
		ASE_AWK_IMPLICIT | ASE_AWK_SHIFT | ASE_AWK_IDIV |
		ASE_AWK_EXTIO | ASE_AWK_BLOCKLESS | ASE_AWK_NEXTOFILE;
	ase_awk_setoption (awk, opt);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{	
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	if (awk != NULL) 
	{
		/* the handle is not NULL. close it */
		void* tmp = ase_awk_getcustomdata (awk);
		ase_awk_close (awk);
		(*env)->SetLongField (env, obj, handle, (jlong)0);
		free (tmp);
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID handle;

	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	srcio_data_t srcio_data;

	class = (*env)->GetObjectClass (env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = &srcio_data;

	if (ase_awk_parse (awk, &srcios) == -1)
	{
		throw_exception (
			env, 
			ase_awk_geterrmsg(awk), 
			ase_awk_geterrnum(awk), 
			ase_awk_geterrlin(awk));

		return;
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv* env, jobject obj)
{
	jclass class;
	jfieldID handle;

	ase_awk_t* awk;
	ase_awk_runios_t runios;
	runio_data_t runio_data;
	run_data_t run_data;

	static int depth_ids[] =
	{
		ASE_AWK_DEPTH_BLOCK_PARSE,
		ASE_AWK_DEPTH_EXPR_PARSE,
		ASE_AWK_DEPTH_REX_BUILD,
		ASE_AWK_DEPTH_REX_MATCH
	};

	class = (*env)->GetObjectClass (env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == 0) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

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

	if (ase_awk_run (awk, 
		ASE_NULL, &runios, ASE_NULL, ASE_NULL, &run_data) == -1)
	{
		throw_exception (
			env, 
			ase_awk_geterrmsg(awk), 
			ase_awk_geterrnum(awk), 
			ase_awk_geterrlin(awk));

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
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, "openSource", "(I)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __java_close_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, "closeSource", "(I)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, mode);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __java_read_source (
	JNIEnv* env, jobject obj, ase_char_t* buf, ase_size_t size)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, "readSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jfieldID handle;
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret;
	ase_size_t i;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, "writeSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)buf[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, array, size);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jfieldID handle;
	jmethodID mid;
	jclass extio_class;
	jmethodID extio_cons;
	jobject extio_object;
	jstring extio_name;
	jint ret;
	ase_awk_t* awk;
	
	/* get the method - meth */
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* look for extio class */
	extio_class = (*env)->FindClass (env, CLASS_EXTIO);
	if (extio_class == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* get the constructor */
	extio_cons = (*env)->GetMethodID (
		env, extio_class, "<init>", "(Ljava/lang/String;IIJ)V");
	if (extio_cons == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* construct the name */
	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i, len = ase_awk_strlen(extio->name);
		jchar* tmp = (jchar*) malloc (ASE_SIZEOF(jchar)*len);
		if (tmp == NULL)
		{
			(*env)->DeleteLocalRef (env, extio_class);
			return -1;
		}

		for (i = 0; i < len; i++) tmp[i] = (jchar)extio->name[i];
		extio_name = (*env)->NewString (env, tmp, len);
		free (tmp);
	}
	else
	{
		extio_name = (*env)->NewString (
			env, (jchar*)extio->name, 
			ase_awk_strlen(extio->name));
	}

	if (extio_name == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
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
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, extio_name);
		return -1;
	}

	(*env)->DeleteLocalRef (env, extio_name);

	/* execute the method */
	ret = (*env)->CallIntMethod (env, obj, mid, extio_object);
	if ((*env)->ExceptionOccurred(env))
	{
		/* clear the exception */
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	if (ret >= 0) 
	{
		/* ret == -1 failed to open the stream
		 * ret ==  0 opened the stream and reached its end 
		 * ret ==  1 opened the stream. */
		extio->handle = (*env)->NewGlobalRef (env, extio_object);
		/*
		if (extio->handle == NULL) 
		{
			// TODO: close the stream ...  
			if (is_debug(awk)) (*env)->ExceptionDescribe (env);
			(*env)->ExceptionClear (env);
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
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jfieldID handle;
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret, i;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, size);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jfieldID handle;
	jmethodID mid;
	jcharArray array;
	jchar* tmp;
	jint ret;
	ase_size_t i;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	array = (*env)->NewCharArray (env, size);
	if (array == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < size; i++) tmp[i] = (jchar)data[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, size);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t __java_next_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
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
	jstring name;
	const char* name_utf;
	run_data_t* run_data;
	JNIEnv* env; 
	jobject obj;
	jint i, nargs;
	jobjectArray args;
	jobject arg, ret;
	ase_awk_val_t* v;
	ase_char_t msg_nomem[MSG_SIZE];
	ase_awk_t* awk;

	run_data = ase_awk_getruncustomdata (run);
	nargs = ase_awk_getnargs (run);
	awk = ase_awk_getrunawk (run);

	env = run_data->env;
	obj = run_data->obj;

	awk_sprintf (
		msg_nomem, ASE_COUNTOF(msg_nomem),
		ASE_T("out of memory in handling %.*s"),
		fnl, fnm);

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		jchar* tmp = (jchar*) malloc (ASE_SIZEOF(jchar)*fnl);
		if (tmp == NULL)
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, msg_nomem);
			return -1;
		}

		for (i = 0; i < fnl; i++) tmp[i] = (jchar)fnm[i];
		name = (*env)->NewString (env, tmp, fnl);
		free (tmp);
	}
	else name = (*env)->NewString (env, (jchar*)fnm, fnl);

	if (name == NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
		return -1;
	}
	name_utf = (*env)->GetStringUTFChars (env, name, JNI_FALSE);
	if (name_utf == NULL)
	{
		(*env)->DeleteLocalRef (env, name);
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
		 * clear it to prevent it from being thrown */
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNUSER);
		return -1;
	}

	args = (*env)->NewObjectArray (
		env, nargs, run_data->object_class, NULL);
	if (args == NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
			{
				ase_size_t i;
			
				jchar* tmp = (jchar*) malloc (
					((ase_awk_val_str_t*)v)->len * ASE_SIZEOF(jchar));
				if (tmp == NULL)
				{
					(*env)->DeleteLocalRef (env, args);
					ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
					return -1;
				}

				for (i = 0; i < ((ase_awk_val_str_t*)v)->len; i++)
					tmp[i] = (jchar)((ase_awk_val_str_t*)v)->buf[i];

				arg = (*env)->NewString (
					env, tmp, ((ase_awk_val_str_t*)v)->len);
				
				free (tmp);
			}
			else
			{
				arg = (*env)->NewString (env, 
					(jchar*)((ase_awk_val_str_t*)v)->buf, 
					((ase_awk_val_str_t*)v)->len);
			}
		}

		if (v->type != ASE_AWK_VAL_NIL && arg == NULL)
		{
			if ((*env)->ExceptionOccurred (env))
			{
				if (is_debug(awk)) 
					(*env)->ExceptionDescribe (env);
				(*env)->ExceptionClear (env);
			}
			(*env)->DeleteLocalRef (env, args);
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
			return -1;
		}

		(*env)->SetObjectArrayElement (env, args, i, arg);
		if (arg != NULL) (*env)->DeleteLocalRef (env, arg);
	}

	ret = (*env)->CallObjectMethod (env, obj, method, (jlong)run, args);
	if ((*env)->ExceptionOccurred (env))
	{
		if (is_debug(ase_awk_getrunawk(run))) 
			(*env)->ExceptionDescribe (env);

		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, args);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
			return -1;
		}

		if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
		{
			ase_size_t i;
			ase_char_t* tmp = (ase_char_t*)
				malloc (ASE_SIZEOF(ase_char_t)*len);
			if (tmp == ASE_NULL)
			{
				(*env)->ReleaseStringChars (env, ret, ptr);
				(*env)->DeleteLocalRef (env, ret);
				ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
				return -1;
			}

			for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
			v = ase_awk_makestrval (run, tmp, len);
			free (tmp);
		}
		else
		{
			v = ase_awk_makestrval (run, (ase_char_t*)ptr, len);
		}

		if (v == NULL)
		{
			(*env)->ReleaseStringChars (env, ret, ptr);
			(*env)->DeleteLocalRef (env, ret);
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, msg_nomem);
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

JNIEXPORT void JNICALL Java_ase_awk_Awk_addbfn (
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
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);

		throw_exception (
			env, 
			ase_awk_geterrstr(ASE_AWK_ENOMEM),
			ASE_AWK_ENOMEM, 
			0);
		return;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		ase_char_t* tmp = (ase_char_t*)
			malloc (ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			throw_exception (
				env, 
				ase_awk_geterrstr(ASE_AWK_ENOMEM),
				ASE_AWK_ENOMEM, 
				0);
			return;
		}

		for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
		n = (ase_awk_addbfn (awk, tmp, len, 0, 
			min_args, max_args, ASE_NULL, __handle_bfn) == NULL)? -1: 0;
		free (tmp);
	}
	else
	{
		n = (ase_awk_addbfn (awk, (ase_char_t*)ptr, len, 0, 
			min_args, max_args, ASE_NULL, __handle_bfn) == NULL)? -1: 0;
	}


	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1)
	{
		throw_exception (
			env, 
			ase_awk_geterrmsg(awk),
			ase_awk_geterrnum(awk),
			ase_awk_geterrlin(awk));
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_delbfn (
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
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		throw_exception (
			env, 
			ase_awk_geterrstr(ASE_AWK_ENOMEM),
			ASE_AWK_ENOMEM,
			0);
		return;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		ase_char_t* tmp = (ase_char_t*)
			malloc (ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			throw_exception (
				env, 
				ase_awk_geterrstr(ASE_AWK_ENOMEM),
				ASE_AWK_ENOMEM,
				0);
			return;
		}

		for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
		n = ase_awk_delbfn (awk, tmp, len);
		free (tmp);
	}
	else
	{
		n = ase_awk_delbfn (awk, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1)
	{
		throw_exception (
			env, 
			ase_awk_geterrmsg(awk),
			ase_awk_geterrnum(awk),
			ase_awk_geterrlin(awk));
	}
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getmaxdepth (
	JNIEnv* env, jobject obj, jint id)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) return 0; /* should never happen */

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	return (jint)ase_awk_getmaxdepth (awk, id);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (
	JNIEnv* env, jobject obj, jint ids, jint depth)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	ase_awk_setmaxdepth (awk, ids, depth);
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getoption (
	JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return 0;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	return ase_awk_getoption (awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setoption (
	JNIEnv* env, jobject obj, jint options)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	ase_awk_setoption (awk, (int)options);
}

JNIEXPORT jboolean JNICALL Java_ase_awk_Awk_getdebug (
	JNIEnv* env, jobject obj)
{
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return JNI_FALSE;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	return ((awk_data_t*)ase_awk_getcustomdata(awk))->debug? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setdebug (
	JNIEnv* env, jobject obj, jboolean debug)
{	
	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_HANDLE, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might occur */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);
	((awk_data_t*)ase_awk_getcustomdata(awk))->debug = debug;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	const jchar* ptr;
	jsize len;
	jint n;

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL) 
	{
		(*env)->ExceptionClear (env);
		throw_exception (
			env, 
			ase_awk_geterrstr(ASE_AWK_ENOMEM),
			ASE_AWK_ENOMEM,
			0);
		return;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		ase_char_t* tmp = (ase_char_t*)
			malloc (ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);

			throw_exception (
				env, 
				ase_awk_geterrstr(ASE_AWK_ENOMEM),
				ASE_AWK_ENOMEM, 
				0);
			return;
		}

		for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
		n = ase_awk_setfilename (run, tmp, len);
		free (tmp);
	}
	else
	{
		n = ase_awk_setfilename (run, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1)
	{
		throw_exception (
			env, 
			ase_awk_getrunerrmsg(run),
			ase_awk_getrunerrnum(run),
			ase_awk_getrunerrlin(run));
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	const jchar* ptr;
	jsize len;
	jint n;

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		throw_exception (
			env, 
			ase_awk_geterrstr(ASE_AWK_ENOMEM),
			ASE_AWK_ENOMEM, 
			0);
		return;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		ase_char_t* tmp = (ase_char_t*)
			malloc (ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);

			throw_exception (
				env, 
				ase_awk_geterrstr(ASE_AWK_ENOMEM),
				ASE_AWK_ENOMEM, 
				0);
			return;
		}

		for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
		n = ase_awk_setofilename (run, tmp, len);
		free (tmp);
	}
	else
	{
		n = ase_awk_setofilename (run, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1)
	{
		throw_exception (
			env, 
			ase_awk_getrunerrmsg(run),
			ase_awk_getrunerrnum(run),
			ase_awk_getrunerrlin(run));
	}
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

	len = (*env)->GetStringLength (env, str);
	ptr = (*env)->GetStringChars (env, str, JNI_FALSE);
	if (ptr == NULL) 
	{
		(*env)->ExceptionClear (env);
		throw_exception (
			env, 
			ase_awk_geterrstr(ASE_AWK_ENOMEM),
			ASE_AWK_ENOMEM, 
			0);
		return NULL;
	}

	if (ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		ase_char_t* tmp = (ase_char_t*)
			malloc (ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, str, ptr);
			throw_exception (
				env, 
				ase_awk_geterrstr(ASE_AWK_ENOMEM),
				ASE_AWK_ENOMEM, 
				0);
			return NULL;
		}

		for (i =  0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
		n = ase_awk_strtonum (
			(ase_awk_run_t*)runid, tmp, len, &lv, &rv);
		free (tmp);
	}
	else
	{
		n = ase_awk_strtonum ( 
			(ase_awk_run_t*)runid, 
			(ase_char_t*)ptr, len, &lv, &rv);
	}
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

	return ret;
}


JNIEXPORT jstring JNICALL Java_ase_awk_Awk_valtostr (
	JNIEnv* env, jobject obj, jlong runid)
{
	// TODO: ... 
	return NULL;
}

