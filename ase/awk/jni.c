/*
 * $Id: jni.c,v 1.35 2007/10/21 15:05:21 bacon Exp $
 *
 * {License}
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <ase/awk/jni.h>
#include <ase/awk/awk.h>
#include <ase/awk/val.h>

#include <ase/cmn/mem.h>
#include <ase/utl/stdio.h>
#include <ase/utl/ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifndef ASE_CHAR_IS_WCHAR
	#error this module supports ASE_CHAR_IS_WCHAR only
#endif

#define CLASS_OUTOFMEMORYERROR "java/lang/OutOfMemoryError"
#define CLASS_EXCEPTION        "ase/awk/Exception"
#define CLASS_EXTIO            "ase/awk/Extio"
#define CLASS_CONTEXT          "ase/awk/Context"
#define CLASS_ARGUMENT         "ase/awk/Argument"
#define FIELD_AWKID            "awkid"
#define FIELD_RUNID            "runid"
#define FIELD_VALID            "valid"

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER>=1400)
	#pragma warning(disable:4996)

	#define time_t __time64_t
	#define time _time64
	#define localtime _localtime64
	#define gmtime _gmtime64
#endif

enum
{
	SOURCE_READ = 1,
	SOURCE_WRITE = 2
};

static ase_ssize_t read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);
static ase_ssize_t process_extio (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

typedef struct awk_data_t   awk_data_t;
typedef struct srcio_data_t srcio_data_t;
typedef struct runio_data_t runio_data_t;
typedef struct run_data_t   run_data_t;

struct awk_data_t
{
	int debug;
#if defined(_WIN32) && defined(__DMC__)
	HANDLE heap;
#endif
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

	ase_size_t errlin;
	int errnum;
	ase_char_t errmsg[256];

	jclass string_class;
	jclass integer_class;
	jclass long_class;
	jclass short_class;
	jclass float_class;
	jclass double_class;
	jclass object_class;
	jclass exception_class;
	jclass context_class;
	jclass argument_class;

	jmethodID integer_init;
	jmethodID long_init;
	jmethodID short_init;
	jmethodID float_init;
	jmethodID double_init;
	jmethodID context_init;
	jmethodID argument_init;

	jmethodID integer_value;
	jmethodID long_value;
	jmethodID short_value;
	jmethodID float_value;
	jmethodID double_value;

	jmethodID exception_get_code;
	jmethodID exception_get_line;
	jmethodID exception_get_message;

	jobject context_object;
};

static void* awk_malloc (void* custom, ase_size_t n)
{
#if defined(_WIN32) && defined(__DMC__)
	return HeapAlloc ((HANDLE)custom, 0, n);
#else
	return malloc (n);
#endif
}

static void* awk_realloc (void* custom, void* ptr, ase_size_t n)
{
#if defined(_WIN32) && defined(__DMC__)
	if (ptr == NULL)
		return HeapAlloc ((HANDLE)custom, 0, n);
	else
		return HeapReAlloc ((HANDLE)custom, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void awk_free (void* custom, void* ptr)
{
#if defined(_WIN32) && defined(__DMC__)
	HeapFree ((HANDLE)custom, 0, ptr);
#else
	free (ptr);
#endif
}

/* custom character class functions */
static ase_bool_t awk_isupper (void* custom, ase_cint_t c)  
{ 
	return ase_isupper (c); 
}

static ase_bool_t awk_islower (void* custom, ase_cint_t c)  
{ 
	return ase_islower (c); 
}

static ase_bool_t awk_isalpha (void* custom, ase_cint_t c)  
{ 
	return ase_isalpha (c); 
}

static ase_bool_t awk_isdigit (void* custom, ase_cint_t c)  
{ 
	return ase_isdigit (c); 
}

static ase_bool_t awk_isxdigit (void* custom, ase_cint_t c) 
{ 
	return ase_isxdigit (c); 
}

static ase_bool_t awk_isalnum (void* custom, ase_cint_t c)
{ 
	return ase_isalnum (c); 
}

static ase_bool_t awk_isspace (void* custom, ase_cint_t c)
{ 
	return ase_isspace (c); 
}

static ase_bool_t awk_isprint (void* custom, ase_cint_t c)
{ 
	return ase_isprint (c); 
}

static ase_bool_t awk_isgraph (void* custom, ase_cint_t c)
{
	return ase_isgraph (c); 
}

static ase_bool_t awk_iscntrl (void* custom, ase_cint_t c)
{
	return ase_iscntrl (c);
}

static ase_bool_t awk_ispunct (void* custom, ase_cint_t c)
{
	return ase_ispunct (c);
}

static ase_cint_t awk_toupper (void* custom, ase_cint_t c)
{
	return ase_toupper (c);
}

static ase_cint_t awk_tolower (void* custom, ase_cint_t c)
{
	return ase_tolower (c);
}

static ase_real_t awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int awk_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static void awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

#ifndef NDEBUG
void ase_assert_abort (void)
{
        abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
#ifdef _WIN32
	{
		TCHAR buf[512];
		_vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
		MessageBox (NULL, buf, _T("ASSERTION FAILURE"), MB_OK | MB_ICONERROR);
	}
#else
	ase_vfprintf (stdout, fmt, ap);
#endif
	va_end (ap);
}
#endif

static void throw_exception (
	JNIEnv* env, const ase_char_t* msg, jint code, jint line)
{
	jclass except_class;
	jmethodID except_cons;
	jstring except_msg;
	jthrowable except_obj;
	ase_size_t len;

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

	len = ase_strlen(msg);
	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;

	#if defined(_WIN32) && defined(__DMC__)
		jchar* tmp = (jchar*) GlobalAlloc (GMEM_FIXED, ASE_SIZEOF(jchar)*len);
	#else
		jchar* tmp = (jchar*) malloc (ASE_SIZEOF(jchar)*len);
	#endif
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
	#if defined(_WIN32) && defined(__DMC__)
		GlobalFree ((HGLOBAL)tmp);
	#else
		free (tmp);
	#endif
	}
	else
	{
		except_msg = (*env)->NewString (env, (jchar*)msg, len);
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

#define THROW_STATIC_EXCEPTION(env,errnum) \
	throw_exception (env, ase_awk_geterrstr(ASE_NULL, errnum), errnum, 0)

#define EXCEPTION_ON_NULL_AWK(env,awk) \
	if (awk == NULL) \
	{ \
		THROW_STATIC_EXCEPTION(env,ASE_AWK_ENOPER); \
		return; \
	}

#define EXCEPTION_ON_NULL_AWK_RETURNING(env,awk,ret) \
	if (awk == NULL) \
	{ \
		THROW_STATIC_EXCEPTION(env,ASE_AWK_ENOPER); \
		return ret; \
	}

#define THROW_NOMEM_EXCEPTION(env) \
	THROW_STATIC_EXCEPTION(env,ASE_AWK_ENOMEM)

#define THROW_AWK_EXCEPTION(env,awk) \
	throw_exception ( \
		env, \
		ase_awk_geterrmsg(awk), \
		ase_awk_geterrnum(awk), \
		ase_awk_geterrlin(awk))

#define THROW_RUN_EXCEPTION(env,run) \
	throw_exception ( \
		env,  \
		ase_awk_getrunerrmsg(run), \
		ase_awk_getrunerrnum(run), \
		ase_awk_getrunerrlin(run))


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
	ase_awk_prmfns_t prmfns;
	awk_data_t* awk_data;
	int opt;
#if defined(_WIN32) && defined(__DMC__)
	HANDLE heap;
#endif

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<OPENING AWK>>>\n");
	#if defined(_MSC_VER)
		_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
	#endif
#endif

#if defined(_WIN32) && defined(__DMC__)
	heap = HeapCreate (0, 0, 0);
	if (heap == NULL)
	{
		THROW_NOMEM_EXCEPTION (env);
		return;
	}
#endif
	
	memset (&prmfns, 0, sizeof(prmfns));

	prmfns.mmgr.malloc = awk_malloc;
	prmfns.mmgr.realloc = awk_realloc;
	prmfns.mmgr.free = awk_free;
#if defined(_WIN32) && defined(__DMC__)
	prmfns.mmgr.custom_data = (void*)heap;
#else
	prmfns.mmgr.custom_data = NULL;
#endif

	prmfns.ccls.is_upper  = awk_isupper;
	prmfns.ccls.is_lower  = awk_islower;
	prmfns.ccls.is_alpha  = awk_isalpha;
	prmfns.ccls.is_digit  = awk_isdigit;
	prmfns.ccls.is_xdigit = awk_isxdigit;
	prmfns.ccls.is_alnum  = awk_isalnum;
	prmfns.ccls.is_space  = awk_isspace;
	prmfns.ccls.is_print  = awk_isprint;
	prmfns.ccls.is_graph  = awk_isgraph;
	prmfns.ccls.is_cntrl  = awk_iscntrl;
	prmfns.ccls.is_punct  = awk_ispunct;
	prmfns.ccls.to_upper  = awk_toupper;
	prmfns.ccls.to_lower  = awk_tolower;
	prmfns.ccls.custom_data = NULL;

	prmfns.misc.pow     = awk_pow;
	prmfns.misc.sprintf = awk_sprintf;
	prmfns.misc.dprintf = awk_dprintf;
	prmfns.misc.custom_data = NULL;

#if defined(_WIN32) && defined(__DMC__)
	awk_data = (awk_data_t*) awk_malloc (heap, sizeof(awk_data_t));
#else
	awk_data = (awk_data_t*) awk_malloc (NULL, sizeof(awk_data_t));
#endif
	if (awk_data == NULL)
	{
#if defined(_WIN32) && defined(__DMC__)
		HeapDestroy (heap);
#endif
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	memset (awk_data, 0, sizeof(awk_data_t));
	awk_data->debug = 0;
#if defined(_WIN32) && defined(__DMC__)
	awk_data->heap = heap;
#endif

	awk = ase_awk_open (&prmfns, awk_data);
	if (awk == NULL)
	{
#if defined(_WIN32) && defined(__DMC__)
		awk_free (heap, awk_data);
		HeapDestroy (heap);
#else
		awk_free (NULL, awk_data);
#endif
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		ase_awk_close (awk);
#if defined(_WIN32) && defined(__DMC__)
		awk_free (heap, awk_data);
		HeapDestroy (heap);
#else
		awk_free (NULL, awk_data);
#endif
		return;
	}

	(*env)->SetLongField (env, obj, handle, (jlong)awk);

	opt = ASE_AWK_IMPLICIT |
	      ASE_AWK_UNIQUEFN | 
	      ASE_AWK_SHADING | 
	      ASE_AWK_EXTIO | 
	      ASE_AWK_BLOCKLESS | 
	      ASE_AWK_BASEONE | 
	      ASE_AWK_PABLOCK;

	ase_awk_setoption (awk, opt);

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<OPENED AWK DONE>>>\n");
#endif
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_close (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk;
	awk_data_t* tmp;
	
	awk = (ase_awk_t*)awkid;
 	/* don't like to throw an exception for close.
	 * i find doing so very irritating, especially when it 
	 * should be called in an exception handler */
	if (awk == NULL) return;

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<CLOSING AWK>>>\n");
#endif

	tmp = (awk_data_t*)ase_awk_getcustomdata (awk);
#if defined(_WIN32) && defined(__DMC__)
	HANDLE heap = tmp->heap;
#endif
	ase_awk_close (awk);

#if defined(_WIN32) && defined(__DMC__)
	awk_free (heap, tmp);
	HeapDestroy (heap);
#else
	awk_free (NULL, tmp);
#endif

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<CLOSED AWK>>>\n");
	#if defined(_MSC_VER)
		_CrtDumpMemoryLeaks ();
	#endif
#endif

}

JNIEXPORT void JNICALL Java_ase_awk_Awk_parse (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	srcio_data_t srcio_data;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = read_source;
	srcios.out = write_source;
	srcios.custom_data = &srcio_data;

	if (ase_awk_parse (awk, &srcios) == -1)
	{
		THROW_AWK_EXCEPTION (env, awk);
		return;
	}
}

#define DELETE_CLASS_REFS(env, run_data) \
	do { \
		(*env)->DeleteLocalRef (env, run_data.integer_class); \
		(*env)->DeleteLocalRef (env, run_data.long_class); \
		(*env)->DeleteLocalRef (env, run_data.float_class); \
		(*env)->DeleteLocalRef (env, run_data.double_class); \
		(*env)->DeleteLocalRef (env, run_data.string_class); \
		(*env)->DeleteLocalRef (env, run_data.object_class); \
		(*env)->DeleteLocalRef (env, run_data.exception_class); \
		(*env)->DeleteLocalRef (env, run_data.context_class); \
		(*env)->DeleteLocalRef (env, run_data.argument_class); \
		if (run_data.context_object != NULL) \
			(*env)->DeleteLocalRef (env, run_data.context_object); \
	} while (0)

static ase_char_t* java_strxdup (ase_awk_t* awk, const jchar* str, jint len)
{
	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_char_t* tmp;
		ase_size_t i;

		tmp = (ase_char_t*) ase_awk_malloc (awk, (len+1) * ASE_SIZEOF(ase_char_t));
		if (tmp == ASE_NULL) return ASE_NULL;

		for (i = 0; i < (ase_size_t)len; i++) tmp[i] = (ase_char_t)str[i];
		tmp[i] = ASE_T('\0');

		return tmp;
	}
	else
	{
		ase_char_t* tmp;

		tmp = (ase_char_t*) ase_awk_malloc (awk, (len+1) * ASE_SIZEOF(ase_char_t));
		if (tmp == ASE_NULL) return ASE_NULL;

		ase_strncpy (tmp, (ase_char_t*)str, (ase_size_t)len);
		return tmp;
	}
}

static void on_run_start (ase_awk_run_t* run, void* custom)
{
	run_data_t* run_data;
	JNIEnv* env;
	jobject obj;
	jfieldID runid;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	env = run_data->env;
	obj = run_data->obj;

	runid = (*env)->GetFieldID (env, run_data->context_class, FIELD_RUNID, "J");
	(*env)->SetLongField (env, run_data->context_object, runid, (jlong)run);
}

static void on_run_end (ase_awk_run_t* run, int errnum, void* custom)
{
	run_data_t* run_data;
	JNIEnv* env;
	jobject obj;
	jfieldID runid_field;
	jlong runid;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	env = run_data->env;
	obj = run_data->obj;

	runid_field = (*env)->GetFieldID (env, run_data->context_class, FIELD_RUNID, "J");
	runid = (*env)->GetLongField (env, run_data->context_object, runid_field);

	if (errnum != ASE_AWK_ENOERR)
	{
		ase_char_t* tmp;

		ase_awk_getrunerror (
			(ase_awk_run_t*)runid, &run_data->errnum,
			&run_data->errlin, &tmp);
		ase_strxcpy (run_data->errmsg, 
			ASE_COUNTOF(run_data->errmsg), tmp);
	}

	/* runid field is not valid any more */
	(*env)->SetLongField (env, run_data->context_object, runid_field, (jlong)0);


}

JNIEXPORT void JNICALL Java_ase_awk_Awk_run (JNIEnv* env, jobject obj, jlong awkid, jstring mfn, jobjectArray args)
{
	ase_awk_t* awk;
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;
	runio_data_t runio_data;
	run_data_t run_data;
	ase_char_t* mmm;
	ase_awk_runarg_t* runarg = NULL;

	ase_size_t len, i;
	const jchar* ptr;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);

	memset (&run_data, 0, sizeof(run_data));

	run_data.env = env;
	run_data.obj = obj;

	run_data.errnum = ASE_AWK_ENOERR;
	run_data.errlin = 0;

	/* would global reference be necessary? */
	run_data.string_class = (*env)->FindClass (env, "java/lang/String");
	run_data.integer_class = (*env)->FindClass (env, "java/lang/Integer");
	run_data.long_class = (*env)->FindClass (env, "java/lang/Long");
	run_data.short_class = (*env)->FindClass (env, "java/lang/Short");
	run_data.float_class = (*env)->FindClass (env, "java/lang/Float");
	run_data.double_class = (*env)->FindClass (env, "java/lang/Double");
	run_data.object_class = (*env)->FindClass (env, "java/lang/Object");
	run_data.exception_class = (*env)->FindClass (env, CLASS_EXCEPTION);
	run_data.context_class = (*env)->FindClass (env, CLASS_CONTEXT);
	run_data.argument_class = (*env)->FindClass (env, CLASS_ARGUMENT);

	ASE_ASSERT (run_data.string_class != NULL);
	ASE_ASSERT (run_data.integer_class != NULL);
	ASE_ASSERT (run_data.short_class != NULL);
	ASE_ASSERT (run_data.long_class != NULL);
	ASE_ASSERT (run_data.float_class != NULL);
	ASE_ASSERT (run_data.double_class != NULL);
	ASE_ASSERT (run_data.object_class != NULL);
	ASE_ASSERT (run_data.exception_class != NULL);
	ASE_ASSERT (run_data.context_class != NULL);
	ASE_ASSERT (run_data.argument_class != NULL);

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
	run_data.context_init = (*env)->GetMethodID (
		env, run_data.context_class, "<init>", "(Lase/awk/Awk;)V");
	run_data.argument_init = (*env)->GetMethodID (
		env, run_data.argument_class, "<init>", "(JJ)V");

	ASE_ASSERT (run_data.integer_init != NULL);
	ASE_ASSERT (run_data.long_init != NULL);
	ASE_ASSERT (run_data.short_init != NULL);
	ASE_ASSERT (run_data.float_init != NULL);
	ASE_ASSERT (run_data.double_init != NULL);
	ASE_ASSERT (run_data.context_init != NULL);
	ASE_ASSERT (run_data.argument_init != NULL);

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

	run_data.exception_get_code = (*env)->GetMethodID (
		env, run_data.exception_class, "getCode", "()I");
	run_data.exception_get_line = (*env)->GetMethodID (
		env, run_data.exception_class, "getLine", "()I");
	run_data.exception_get_message = (*env)->GetMethodID (
		env, run_data.exception_class, "getMessage", "()Ljava/lang/String;");
	
	ASE_ASSERTX (run_data.integer_value != NULL,
		"The Integer class must has the method - int intValue()");
	ASE_ASSERTX (run_data.long_value != NULL,
		"The Long class must has the method - long longValue()");
	ASE_ASSERTX (run_data.short_value != NULL,
		"The Short class must has the method - short shortValue()");
	ASE_ASSERTX (run_data.float_value != NULL, 
		"The Float class must has the method - float floatValue()");
	ASE_ASSERTX (run_data.double_value != NULL, 
		"The Double class must has the method - double doubleValue()");
	ASE_ASSERT (run_data.exception_get_code != NULL);
	ASE_ASSERT (run_data.exception_get_line != NULL);
	ASE_ASSERT (run_data.exception_get_message != NULL);

	/* No NewGlobalRef are needed on obj and run_data->context_object
	 * because they are valid while this run method runs */
	run_data.context_object = (*env)->NewObject (
		env, run_data.context_class, run_data.context_init, obj);
	if (run_data.context_object == NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		DELETE_CLASS_REFS (env, run_data);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	runio_data.env = env;
	runio_data.obj = obj;

	runios.pipe = process_extio;
	runios.coproc = ASE_NULL;
	runios.file = process_extio;
	runios.console = process_extio;
	runios.custom_data = &runio_data;

	ase_memset (&runcbs, 0, ASE_SIZEOF(runcbs));
	runcbs.on_start = on_run_start;
	runcbs.on_end = on_run_end;
	runcbs.custom_data = NULL;

	if (mfn == NULL) 
	{
		mmm = NULL;
		ptr = NULL;
	}
	else
	{
		/* process the main entry point */
		len = (*env)->GetStringLength (env, mfn);

		if (len > 0)
		{
			ase_size_t i;

			ptr = (*env)->GetStringChars (env, mfn, JNI_FALSE);
			if (ptr == NULL)
			{
				(*env)->ExceptionClear (env);
				DELETE_CLASS_REFS (env, run_data);
				THROW_NOMEM_EXCEPTION (env);
				return;
			}

			mmm = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
			if (mmm == ASE_NULL)
			{
				(*env)->ReleaseStringChars (env, mfn, ptr);
				DELETE_CLASS_REFS (env, run_data);
				THROW_NOMEM_EXCEPTION (env);
				return;
			}

			for (i = 0; i < len; i++) 
			{
				mmm[i] = (ase_char_t)ptr[i];
				if (mmm[i] == ASE_T('\0'))
				{
					ase_awk_free (awk, mmm);
					(*env)->ReleaseStringChars (env, mfn, ptr);
					DELETE_CLASS_REFS (env, run_data);
					throw_exception (
						env, 
						ASE_T("main function name not valid"),
						ASE_AWK_EINVAL,
						0);
					return;
				}
			}
			mmm[len] = ASE_T('\0');
		}
		else 
		{
			mmm = NULL;
			ptr = NULL;
		}
	}

	if (args != NULL)
	{
		/* compose arguments */

		len = (*env)->GetArrayLength (env, args);

		runarg = ase_awk_malloc (awk, sizeof(ase_awk_runarg_t) * (len+1));
		if (runarg == NULL)
		{
			if (mmm != NULL) ase_awk_free (awk, mmm);
			if (ptr != NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
			DELETE_CLASS_REFS (env, run_data);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (i = 0; i < len; i++)
		{
			const jchar* tmp;
			jstring obj = (jstring)(*env)->GetObjectArrayElement (env, args, i);

			runarg[i].len = (*env)->GetStringLength (env, obj);	
			tmp = (*env)->GetStringChars (env, obj, JNI_FALSE);
			if (tmp == NULL)
			{
				ase_size_t j;

				for (j = 0; j < i; j++) ase_awk_free (awk, runarg[j].ptr);
				ase_awk_free (awk, runarg);

				(*env)->DeleteLocalRef (env, obj);

				if (mmm != NULL && mmm) ase_awk_free (awk, mmm);
				if (ptr != NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
				DELETE_CLASS_REFS (env, run_data);

				THROW_NOMEM_EXCEPTION (env);
				return;
			}

			runarg[i].ptr = java_strxdup (awk, tmp, runarg[i].len);
			if (runarg[i].ptr == NULL)
			{
				ase_size_t j;

				for (j = 0; j < i; j++) ase_awk_free (awk, runarg[j].ptr);
				ase_awk_free (awk, runarg);

				(*env)->ReleaseStringChars (env, obj, tmp);
				(*env)->DeleteLocalRef (env, obj);

				if (mmm != NULL) ase_awk_free (awk, mmm);
				if (ptr != NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
				DELETE_CLASS_REFS (env, run_data);
				THROW_NOMEM_EXCEPTION (env);
				return;
			}
			
			(*env)->ReleaseStringChars (env, obj, tmp);
			(*env)->DeleteLocalRef (env, obj);
		}

		runarg[i].ptr = NULL;
		runarg[i].len = 0;
	}

	if (ase_awk_run (awk, mmm, &runios, &runcbs, runarg, &run_data) == -1 ||
	    run_data.errnum != ASE_AWK_ENOERR)
	{
		if (runarg != NULL)
		{
			for (i = 0; i < len; i++) ase_awk_free (awk, runarg[i].ptr);
			ase_awk_free (awk, runarg);
		}

		if (mmm != NULL) ase_awk_free (awk, mmm);
		if (ptr != NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
		DELETE_CLASS_REFS (env, run_data);

		if (run_data.errnum != ASE_AWK_ENOERR)
		{
			throw_exception (
				env, 
				run_data.errmsg,
				run_data.errnum,
				run_data.errlin);
		}
		else THROW_AWK_EXCEPTION (env, awk);
		return;
	}

	if (runarg != NULL)
	{
		for (i = 0; i < len; i++) ase_awk_free (awk, runarg[i].ptr);
		ase_awk_free (awk, runarg);
	}

	if (mmm != NULL) ase_awk_free (awk, mmm);
	if (ptr != NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
	DELETE_CLASS_REFS (env, run_data);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_stop (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);
	ase_awk_stopall (awk);
}

static ase_ssize_t java_open_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t java_close_source (JNIEnv* env, jobject obj, int mode)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t java_read_source (
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
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
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

static ase_ssize_t java_write_source (
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
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, array);
	return ret;
}

static ase_ssize_t java_open_extio (
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
	ase_size_t len;
	
	/* get the method - meth */
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	len = ase_strlen(extio->name);
	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		jchar* tmp = (jchar*) ase_awk_malloc (awk, ASE_SIZEOF(jchar)*len);
		if (tmp == NULL)
		{
			(*env)->DeleteLocalRef (env, extio_class);
			return -1;
		}

		for (i = 0; i < len; i++) tmp[i] = (jchar)extio->name[i];
		extio_name = (*env)->NewString (env, tmp, len);
		ase_awk_free (awk, tmp);
	}
	else
	{
		extio_name = (*env)->NewString (env, (jchar*)extio->name, len);
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
		extio_name, extio->type & 0xFF, extio->mode, (jlong)extio->run);
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
	if ((*env)->ExceptionCheck(env))
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

static ase_ssize_t java_close_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
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

static ase_ssize_t java_read_extio (
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
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
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

static ase_ssize_t java_write_extio (
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
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	(*env)->DeleteLocalRef (env, array);
	return ret;
}


static ase_ssize_t java_flush_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t java_next_extio (
	JNIEnv* env, jobject obj, char* meth, ase_awk_extio_t* extio)
{
	jclass class; 
	jfieldID handle;
	jmethodID mid;
	jint ret;
	ase_awk_t* awk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
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
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ret = -1;
	}

	return ret;
}

static ase_ssize_t read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_READ);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		return java_read_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static ase_ssize_t write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	srcio_data_t* srcio_data = (srcio_data_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return java_open_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return java_close_source (
			srcio_data->env, srcio_data->obj, SOURCE_WRITE);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return java_write_source (
			srcio_data->env, srcio_data->obj, data, count);
	}

	return -1;
}

static ase_ssize_t process_extio (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	runio_data_t* runio_data = (runio_data_t*)epa->custom_data;

	switch (cmd)
	{

		case ASE_AWK_IO_OPEN:
			return java_open_extio (
				runio_data->env, runio_data->obj, 
				"openExtio", epa);

		case ASE_AWK_IO_CLOSE:
			return java_close_extio (
				runio_data->env, runio_data->obj, 
				"closeExtio", epa);

		case ASE_AWK_IO_READ:
			return java_read_extio (
				runio_data->env, runio_data->obj, 
				"readExtio", epa, data, size);

		case ASE_AWK_IO_WRITE:

			return java_write_extio (
				runio_data->env, runio_data->obj, 
				"writeExtio", epa, data, size);

		case ASE_AWK_IO_FLUSH:
			return java_flush_extio (
				runio_data->env, runio_data->obj,
				"flushExtio", epa);

		case ASE_AWK_IO_NEXT:
			return java_next_extio (
				runio_data->env, runio_data->obj, 
				"nextExtio", epa);

	}

	return -1;
}

static int handle_bfn (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	jclass class; 
	jmethodID method;
	jthrowable throwable;
	jstring name;
	//const char* name_utf;
	run_data_t* run_data;
	JNIEnv* env; 
	jobject obj;
	jint i, nargs;
	jobjectArray args;
	jobject arg, ret;
	ase_awk_val_t* v;
	ase_awk_t* awk;

	run_data = ase_awk_getruncustomdata (run);
	nargs = ase_awk_getnargs (run);
	awk = ase_awk_getrunawk (run);

	env = run_data->env;
	obj = run_data->obj;

	if (fnl > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t i;
		jchar* tmp = (jchar*) ase_awk_malloc (
			awk, ASE_SIZEOF(jchar)*(fnl+4));
		if (tmp == NULL)
		{
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		/*
		tmp[0] = (jchar)'b';
		tmp[1] = (jchar)'f';
		tmp[2] = (jchar)'n';
		tmp[3] = (jchar)'_';
		for (i = 0; i < fnl; i++) tmp[i+4] = (jchar)fnm[i];
		*/
		for (i = 0; i < fnl; i++) tmp[i] = (jchar)fnm[i];
		name = (*env)->NewString (env, tmp, fnl+4);
		ase_awk_free (awk, tmp);
	}
	else 
	{
		name = (*env)->NewString (env, (jchar*)fnm, fnl);
	}

	if (name == NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	/*
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
	*/
	class = (*env)->GetObjectClass(env, obj);
	method = (*env)->GetMethodID (
		env, class, "handleFunction", 
		"(Lase/awk/Context;Ljava/lang/String;[Lase/awk/Argument;)Ljava/lang/Object;");

	(*env)->DeleteLocalRef (env, class);
	/*(*env)->ReleaseStringUTFChars (env, name, name_utf);*/
	//(*env)->DeleteLocalRef (env, name);
	if (method == NULL) 
	{
		/* if the method is not found, the exception is thrown.
		 * clear it to prevent it from being thrown */
		(*env)->DeleteLocalRef (env, name);
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_EINTERN);
		return -1;
	}

	args = (*env)->NewObjectArray (
		env, nargs, run_data->argument_class, NULL);
	if (args == NULL)
	{
		(*env)->DeleteLocalRef (env, name);
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		v = ase_awk_getarg (run, i);

		arg = (*env)->NewObject (env, 
			run_data->argument_class, 
			run_data->argument_init, (jlong)run, (jlong)v);
		if (arg == NULL)
		{
			if ((*env)->ExceptionCheck(env))
			{
				if (is_debug(awk)) 
					(*env)->ExceptionDescribe (env);
				(*env)->ExceptionClear (env);
			}
			(*env)->DeleteLocalRef (env, args);
			(*env)->DeleteLocalRef (env, name);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->SetObjectArrayElement (env, args, i, arg);
		if (arg != NULL) (*env)->DeleteLocalRef (env, arg);
	}

	ret = (*env)->CallObjectMethod (
		env, obj, method, run_data->context_object, name, args);
	throwable = (*env)->ExceptionOccurred(env);
	if (throwable)
	{
		jint code;
		jstring mesg;
		ase_char_t* rptr;
		const jchar* ptr;
		jsize len;
		jclass class;

		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, args);
		(*env)->DeleteLocalRef (env, name);

		class = (*env)->GetObjectClass (env, throwable);
		if (!(*env)->IsSameObject(env,class,run_data->exception_class))
		{
			(*env)->DeleteLocalRef (env, class);
			ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);
			return -1;
		}
		(*env)->DeleteLocalRef (env, class);

		code = (*env)->CallIntMethod (env, throwable, run_data->exception_get_code);
		if ((*env)->ExceptionCheck(env))
		{
			if (is_debug(awk)) (*env)->ExceptionDescribe (env);
			(*env)->ExceptionClear (env);
			ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);
			return -1;
		}
		if (code == ASE_AWK_ENOERR) code = ASE_AWK_EBFNIMPL;

		/* the line information is not important in this context.
		 * it will be replaced by the underlying engine anyhow. */
		/*line = (*env)->CallIntMethod (env, throwable, run_data->exception_get_line);*/
		mesg = (*env)->CallObjectMethod (env, throwable, run_data->exception_get_message);
		if ((*env)->ExceptionCheck(env))
		{
			if (is_debug(awk)) (*env)->ExceptionDescribe (env);
			(*env)->ExceptionClear (env);
			ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);
			return -1;
		}

		if (mesg == NULL)
		{
			/* there is no message given */
			ase_awk_setrunerrnum (run, code);
			return -1;
		}

		len = (*env)->GetStringLength (env, mesg);
		ptr = (*env)->GetStringChars (env, mesg, JNI_FALSE);

		if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
		{
			jsize x;
			rptr = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
			if (rptr == ASE_NULL)
			{
				/* ran out of memory in exception handling.
				 * it is freaking studid. */
				ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
				goto error_in_exception_handler;
			}
			for (x = 0; x < len; x++) rptr[x] = (ase_char_t)ptr[x];
		}
		else rptr = (ase_char_t*)ptr;

		ase_awk_setrunerrmsg (run, code, 0, rptr);
		if (rptr != ptr) ase_awk_free (awk, rptr);

	error_in_exception_handler:
		(*env)->ReleaseStringChars (env, mesg, ptr);
		(*env)->DeleteLocalRef (env, throwable);

		return -1;
	}

	(*env)->DeleteLocalRef (env, args);
	(*env)->DeleteLocalRef (env, name);

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

		if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
		{
			jsize x;
			ase_char_t* tmp = (ase_char_t*)
				ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
			if (tmp == ASE_NULL)
			{
				(*env)->ReleaseStringChars (env, ret, ptr);
				(*env)->DeleteLocalRef (env, ret);
				ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
				return -1;
			}

			for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
			v = ase_awk_makestrval (run, tmp, len);
			ase_awk_free (awk, tmp);
		}
		else
		{
			v = ase_awk_makestrval (run, (ase_char_t*)ptr, len);
		}

		if (v == NULL)
		{
			(*env)->ReleaseStringChars (env, ret, ptr);
			(*env)->DeleteLocalRef (env, ret);
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

JNIEXPORT void JNICALL Java_ase_awk_Awk_addfunc (
	JNIEnv* env, jobject obj, jstring name, jint min_args, jint max_args)
{
	jclass class; 
	jfieldID handle;
	jint n;

	ase_awk_t* awk;
	const jchar* ptr;
	jsize len;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
		n = (ase_awk_addfunc (awk, tmp, len, 0, 
			min_args, max_args, ASE_NULL, handle_bfn) == NULL)? -1: 0;
		ase_awk_free (awk, tmp);
	}
	else
	{
		n = (ase_awk_addfunc (awk, (ase_char_t*)ptr, len, 0, 
			min_args, max_args, ASE_NULL, handle_bfn) == NULL)? -1: 0;
	}


	(*env)->ReleaseStringChars (env, name, ptr);
	if (n == -1) THROW_AWK_EXCEPTION (env, awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_delfunc (
	JNIEnv* env, jobject obj, jstring name)
{
	jclass class; 
	jfieldID handle;
	jint n;

	ase_awk_t* awk;
	const jchar* ptr;
	jsize len;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
		n = ase_awk_delfunc (awk, tmp, len);
		ase_awk_free (awk, tmp);
	}
	else
	{
		n = ase_awk_delfunc (awk, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);
	if (n == -1) THROW_AWK_EXCEPTION (env, awk);
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getmaxdepth (JNIEnv* env, jobject obj, jlong awkid, jint id)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK_RETURNING (env, awk, 0);
	return (jint)ase_awk_getmaxdepth (awk, id);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (JNIEnv* env, jobject obj, jlong awkid, jint ids, jint depth)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);
	ase_awk_setmaxdepth (awk, ids, depth);
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getoption (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK_RETURNING (env, awk, 0);
	return ase_awk_getoption (awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setoption (JNIEnv* env, jobject obj, jlong awkid, jint options)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);
	ase_awk_setoption (awk, (int)options);
}

JNIEXPORT jboolean JNICALL Java_ase_awk_Awk_getdebug (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK_RETURNING (env, awk, JNI_FALSE);
	return ((awk_data_t*)ase_awk_getcustomdata(awk))->debug? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setdebug (
	JNIEnv* env, jobject obj, jlong awkid, jboolean debug)
{	
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);
	((awk_data_t*)ase_awk_getcustomdata(awk))->debug = debug;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setword (
	JNIEnv* env, jobject obj, jlong awkid, jstring ow, jstring nw)
{	
	ase_awk_t* awk;
	const jchar* op = NULL, * np = NULL;
	jsize ol = 0, nl = 0;
	ase_char_t* ox, * nx;
	jint r;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_NULL_AWK (env, awk);

	if (ow != NULL)
	{
		ol = (*env)->GetStringLength (env, ow);
		op = (*env)->GetStringChars (env, ow, JNI_FALSE);
		if (op == NULL) 
		{
			(*env)->ExceptionClear (env);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}

	if (nw != NULL)
	{
		nl = (*env)->GetStringLength (env, nw);
		np = (*env)->GetStringChars (env, nw, JNI_FALSE);
		if (np == NULL) 
		{
			if (ow != NULL) 
				(*env)->ReleaseStringChars (env, ow, op);
			(*env)->ExceptionClear (env);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}

	if (ol > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ox = (ase_char_t*)ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*ol);
		if (ox == ASE_NULL)
		{
			if (nw != NULL) (*env)->ReleaseStringChars (env, nw, np);
			if (ow != NULL) (*env)->ReleaseStringChars (env, ow, op);

			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < ol; x++) ox[x] = (ase_char_t)op[x];
	}
	else ox = (ase_char_t*)op;

	if (nl > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		nx = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*nl);
		if (nx == ASE_NULL)
		{
			if (ox != (ase_char_t*)op) ase_awk_free (awk, ox);

			if (nw != NULL) (*env)->ReleaseStringChars (env, nw, np);
			if (ow != NULL) (*env)->ReleaseStringChars (env, ow, op);

			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < nl; x++) nx[x] = (ase_char_t)np[x];
	}
	else nx = (ase_char_t*)np;

	r = ase_awk_setword (awk, ox, ol, nx, nl);

	if (nx != (ase_char_t*)np) ase_awk_free (awk, nx);
	if (ox != (ase_char_t*)op) ase_awk_free (awk, ox);

	if (nw != NULL) (*env)->ReleaseStringChars (env, nw, np);
	if (ow != NULL) (*env)->ReleaseStringChars (env, ow, op);

	if (r == -1) THROW_AWK_EXCEPTION (env, awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setfilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	const jchar* ptr;
	jsize len;
	jint n;

	ase_awk_t* awk = ase_awk_getrunawk (run);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL) 
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
		n = ase_awk_setfilename (run, tmp, len);
		ase_awk_free (awk, tmp);
	}
	else
	{
		n = ase_awk_setfilename (run, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1) THROW_RUN_EXCEPTION (env, run);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setofilename (
	JNIEnv* env, jobject obj, jlong runid, jstring name)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	const jchar* ptr;
	jsize len;
	jint n;

	ase_awk_t* awk = ase_awk_getrunawk (run);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
		n = ase_awk_setofilename (run, tmp, len);
		ase_awk_free (awk, tmp);
	}
	else
	{
		n = ase_awk_setofilename (run, (ase_char_t*)ptr, len);
	}

	(*env)->ReleaseStringChars (env, name, ptr);

	if (n == -1) THROW_RUN_EXCEPTION (env, run);
}

static jstring JNICALL call_strftime (
	JNIEnv* env, jobject obj, jstring fmt, struct tm* tm)
{
	ase_char_t buf[128]; 
	ase_size_t len, i;
	const jchar* ptr;
	ase_char_t* tmp;
	jchar* tmp2;
	jstring ret;

	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return NULL;
	}
	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, fmt);
	ptr = (*env)->GetStringChars (env, fmt, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return NULL;
	}

	tmp = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
	if (tmp == NULL)
	{
		(*env)->ReleaseStringChars (env, fmt, ptr);
		THROW_NOMEM_EXCEPTION (env);
		return NULL;
	}

	for (i = 0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
	tmp[i] = ASE_T('\0');

	len = wcsftime (buf, ASE_COUNTOF(buf), tmp, tm);

	ase_awk_free (awk, tmp);
	(*env)->ReleaseStringChars (env, fmt, ptr);

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		tmp2 = (jchar*) ase_awk_malloc (awk, ASE_SIZEOF(jchar)*len);
		if (tmp2 == NULL)
		{
			THROW_NOMEM_EXCEPTION (env);
			return NULL;
		}
		for (i = 0; i < len; i++) tmp2[i] = (jchar)buf[i];
		ret = (*env)->NewString (env, tmp2, len);
		ase_awk_free (awk, tmp2);
	}
	else
	{
		ret = (*env)->NewString (env, (jchar*)buf, len);
	}

	if (ret == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
	}

	return ret;
}


JNIEXPORT jstring JNICALL Java_ase_awk_Awk_strftime (
	JNIEnv* env, jobject obj, jstring fmt, jlong sec)
{
	struct tm* tm;
	time_t t = (time_t)sec;

#ifdef _WIN32
	tm = localtime (&t);
#else
	struct tm tmb;
	tm = localtime_r (&t, &tmb);
#endif

	return call_strftime (env, obj, fmt, tm);
}

JNIEXPORT jstring JNICALL Java_ase_awk_Awk_strfgmtime (
	JNIEnv* env, jobject obj, jstring fmt, jlong sec)
{
	struct tm* tm;
	time_t t = (time_t)sec;

#ifdef _WIN32
	tm = gmtime (&t);
#else
	struct tm tmb;
	tm = gmtime_r (&t, &tmb);
#endif

	return call_strftime (env, obj, fmt, tm);
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_system (
	JNIEnv* env, jobject obj, jstring cmd)
{
	ase_size_t len, i;
	const jchar* ptr;
	ase_char_t* tmp;
	jint ret;

	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return -1;
	}
	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, cmd);
	ptr = (*env)->GetStringChars (env, cmd, JNI_FALSE);
	if (ptr == NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return -1;
	}

	tmp = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
	if (tmp == NULL)
	{
		(*env)->ReleaseStringChars (env, cmd, ptr);
		THROW_NOMEM_EXCEPTION (env);
		return -1;
	}

	for (i = 0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
	tmp[i] = ASE_T('\0');

	(*env)->ReleaseStringChars (env, cmd, ptr);

#ifdef _WIN32
	ret = _tsystem(tmp);
#else
	char* mbs = (char*)ase_awk_malloc (awk, len*5+1);
	if (mbs == ASE_NULL) 
	{
		ase_awk_free (awk, tmp);
		return -1;
	}

	size_t mbl = wcstombs (mbs, tmp, len*5);
	if (mbl == (size_t)-1) 
	{
		ase_awk_free (awk, mbs);
		ase_awk_free (awk, tmp);
		return -1;
	}
	mbs[mbl] = '\0';
	ret = system(mbs);
	ase_awk_free (awk, mbs);
#endif

	ase_awk_free (awk, tmp);
	return ret;
}

JNIEXPORT void JNICALL Java_ase_awk_Context_stop (JNIEnv* env, jobject obj, jlong runid)
{
	ase_awk_stop ((ase_awk_run_t*)runid);
}


JNIEXPORT jlong JNICALL Java_ase_awk_Argument_getintval (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	int n;
	ase_long_t lv = 0;
	ase_real_t rv = 0.0;

	n = ase_awk_valtonum (run, (ase_awk_val_t*)valid, &lv, &rv);
	if (n == -1) THROW_RUN_EXCEPTION (env, run);
	else if (n == 1) lv = (ase_long_t)rv;

	return (jlong)lv; 
}

JNIEXPORT jdouble JNICALL Java_ase_awk_Argument_getrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	int n;
	ase_long_t lv = 0;
	ase_real_t rv = 0.0;

	n = ase_awk_valtonum (run, (ase_awk_val_t*)valid, &lv, &rv);
	if (n == -1) THROW_RUN_EXCEPTION (env, run);
	else if (n == 0) rv = (ase_real_t)lv;

	return (jdouble)rv; 
}

JNIEXPORT jstring JNICALL Java_ase_awk_Argument_getstrval (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_t* awk = ase_awk_getrunawk (run);

	ase_char_t* str;
	ase_size_t  len;
	jstring ret = NULL;

	str = ase_awk_valtostr (
		run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
	if (str == ASE_NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return ret;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jchar* tmp;
		ase_size_t i;
	       
		tmp = (jchar*) ase_awk_malloc (awk, ASE_SIZEOF(jchar)*len);
		if (tmp == NULL)
		{
			ase_awk_free (awk, str);
			goto nomem;
		}
		for (i = 0; i < len; i++) tmp[i] = (jchar)str[i];
		ret = (*env)->NewString (env, tmp, len);
		ase_awk_free (awk, tmp);
	}
	else
	{
		ret = (*env)->NewString (env, (jchar*)str, len);
	}

	ase_awk_free (awk, str);

	if (ret == NULL)
	{
nomem:
		if ((*env)->ExceptionCheck(env))
		{
			if (is_debug(awk)) (*env)->ExceptionDescribe (env);
			(*env)->ExceptionClear (env);
		}

		THROW_NOMEM_EXCEPTION (env);
	}

	return ret;
}

JNIEXPORT jboolean JNICALL Java_ase_awk_Argument_isindexed (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	return (ASE_AWK_VAL_TYPE(val) == ASE_AWK_VAL_MAP)? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT jobject JNICALL Java_ase_awk_Argument_getindexed (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring index)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_t* awk = ase_awk_getrunawk (run);
	run_data_t* run_data = (run_data_t*)ase_awk_getruncustomdata (run);
	ase_awk_pair_t* pair;
	jobject arg;

	const jchar* ptr;
	ase_char_t* rptr;
	ase_size_t len;

	if (ASE_AWK_VAL_TYPE(val) != ASE_AWK_VAL_MAP) return ASE_NULL;

	len = (*env)->GetStringLength (env, index);
	ptr = (*env)->GetStringChars (env, index, JNI_FALSE);
	if (ptr == NULL) goto nomem;

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t x;
		rptr = (ase_char_t*) ase_awk_malloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (rptr == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, index, ptr);
			goto nomem;
		}
		for (x = 0; x < len; x++) rptr[x] = (ase_char_t)ptr[x];
	}
	else rptr = (ase_char_t*)ptr;

	pair = ase_awk_map_get (((ase_awk_val_map_t*)val)->map, rptr, len);
	if (ptr != rptr) ase_awk_free (awk, rptr);
	(*env)->ReleaseStringChars (env, index, ptr);

	/* the key is not found. it is not an error. val is just nil */
	if (pair == ASE_NULL) return ASE_NULL;; 

	arg = (*env)->NewObject (env, 
		run_data->argument_class, 
		run_data->argument_init, (jlong)run, (jlong)pair->val);
	if (arg == NULL) goto nomem;

	return arg;

nomem:
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
	}

	THROW_NOMEM_EXCEPTION (env);
	return ASE_NULL;
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setintval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jlong newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	jclass class;
	jfieldID field;

	nv = ase_awk_makeintval (run, newval);
	if (nv == NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	ase_awk_refdownval (run, val);

	class = (*env)->GetObjectClass(env, obj);
	field = (*env)->GetFieldID (env, class, FIELD_VALID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (field == NULL)
	{
		/* out of memory, field not found, etc */
		return;
	}

	(*env)->SetLongField (env, obj, field, (jlong)nv);
	ase_awk_refupval (run, nv); // should be refdowned when Return is invalidated....);
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jdouble newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	jclass class;
	jfieldID field;

	nv = ase_awk_makerealval (run, newval);
	if (nv == NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	ase_awk_refdownval (run, val);

	class = (*env)->GetObjectClass(env, obj);
	field = (*env)->GetFieldID (env, class, FIELD_VALID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (field == NULL)
	{
		/* out of memory, field not found, etc */
		return;
	}

	(*env)->SetLongField (env, obj, field, (jlong)nv);
	ase_awk_refupval (run, nv); // should be refdowned when Return is invalidated....);
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setstrval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	jclass class;
	jfieldID field;


	// TODO:....
	nv = ase_awk_makerealval (run, newval);
	if (nv == NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	ase_awk_refdownval (run, val);

	class = (*env)->GetObjectClass(env, obj);
	field = (*env)->GetFieldID (env, class, FIELD_VALID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (field == NULL)
	{
		/* out of memory, field not found, etc */
		return;
	}

	(*env)->SetLongField (env, obj, field, (jlong)nv);
	ase_awk_refupval (run, nv); // should be refdowned when Return is invalidated....);
}
