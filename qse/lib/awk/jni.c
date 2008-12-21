/*
 * $Id: jni.c 363 2008-09-04 10:58:08Z baconevi $
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

#include "../cmn/mem.h"
#include "../cmn/chr.h"
#include <ase/utl/stdio.h>

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
#define CLASS_RETURN           "ase/awk/Return"
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

static ase_char_t* dup_str (
	ase_awk_t* awk, const jchar* str, ase_size_t len);
static int get_str (
	JNIEnv* env, ase_awk_t* awk, jstring str, 
	const jchar** jptr, ase_char_t** aptr, jsize* outlen);
static void free_str (
	JNIEnv* env, ase_awk_t* awk, jstring str, 
	const jchar* jptr, ase_char_t* aptr);

static jstring new_str (
	JNIEnv* env, ase_awk_t* awk, const ase_char_t* ptr, ase_size_t len);

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

	ase_size_t errlin;
	int errnum;
	ase_char_t errmsg[256];

	jclass exception_class;
	jclass context_class;
	jclass argument_class;
	jclass return_class;

	jmethodID context_init;
	jmethodID argument_init;
	jmethodID return_init;

	jmethodID exception_get_code;
	jmethodID exception_get_line;
	jmethodID exception_get_message;

	jmethodID context_clear;

	jfieldID argument_valid;
	jfieldID return_valid;

	jobject context_object;
};

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
		MessageBox (ASE_NULL, buf, _T("ASSERTION FAILURE"), MB_OK | MB_ICONERROR);
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
	jsize chunk;

	except_class = (*env)->FindClass (env, CLASS_EXCEPTION);
	if (except_class == ASE_NULL) 
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
	if (except_cons == ASE_NULL)
	{
		/* the potential exception to be thrown by the GetMethodID
		 * method is not cleared here for the same reason as the
		 * FindClass method above */
		(*env)->DeleteLocalRef (env, except_class);
		return;
	}

	len = ase_strlen(msg);
	chunk = (len > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)len;

	if (chunk > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize i;

	#if defined(_WIN32) && defined(__DMC__)
		jchar* tmp = (jchar*) GlobalAlloc (GMEM_FIXED, ASE_SIZEOF(jchar)*chunk);
	#else
		jchar* tmp = (jchar*) malloc (ASE_SIZEOF(jchar)*chunk);
	#endif
		if (tmp == ASE_NULL)
		{
			(*env)->DeleteLocalRef (env, except_class);

			except_class = (*env)->FindClass (
				env, CLASS_OUTOFMEMORYERROR);
			if (except_class == ASE_NULL) return;

			(*env)->ThrowNew (env, except_class, "out of memory");
			(*env)->DeleteLocalRef (env, except_class);
			return;
		}

		for (i = 0; i < chunk; i++) tmp[i] = (jchar)msg[i];
		except_msg = (*env)->NewString (env, tmp, chunk);
	#if defined(_WIN32) && defined(__DMC__)
		GlobalFree ((HGLOBAL)tmp);
	#else
		free (tmp);
	#endif
	}
	else
	{
		except_msg = (*env)->NewString (env, (jchar*)msg, chunk);
	}

	if (except_msg == ASE_NULL)
	{
		(*env)->DeleteLocalRef (env, except_class);
		return;
	}

	except_obj = (*env)->NewObject (
		env, except_class, except_cons,
		except_msg, code, line);

	(*env)->DeleteLocalRef (env, except_msg);
	(*env)->DeleteLocalRef (env, except_class);

	if (except_obj == ASE_NULL) return;

	(*env)->Throw (env, except_obj);
	(*env)->DeleteLocalRef (env, except_obj);
}

#define THROW_STATIC_EXCEPTION(env,errnum) \
	throw_exception (env, ase_awk_geterrstr(ASE_NULL, errnum), errnum, 0)

#define EXCEPTION_ON_ASE_NULL_AWK(env,awk) \
	if (awk == ASE_NULL) \
	{ \
		THROW_STATIC_EXCEPTION(env,ASE_AWK_ENOPER); \
		return; \
	}

#define EXCEPTION_ON_ASE_NULL_AWK_RETURNING(env,awk,ret) \
	if (awk == ASE_NULL) \
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
		(jint)ase_awk_geterrnum(awk), \
		(jint)ase_awk_geterrlin(awk))

#define THROW_RUN_EXCEPTION(env,run) \
	throw_exception ( \
		env,  \
		ase_awk_getrunerrmsg(run), \
		(jint)ase_awk_getrunerrnum(run), \
		(jint)ase_awk_getrunerrlin(run))


static jboolean is_debug (ase_awk_t* awk)
{
	awk_data_t* awk_data = (awk_data_t*)ase_awk_getextension (awk);
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

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<OPENING AWK>>>\n");
	#if defined(_MSC_VER)
		_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
	#endif
#endif

	awk = ase_awk_open (ASE_NULL, ASE_SIZEOF(awk_data_t), ASE_NULL);
	if (awk == ASE_NULL)
	{
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	awk_data = (awk_data_t*) ase_awk_getextension (awk);
	awk_data->debug = 0;
	awk_data->prmfns.pow     = awk_pow;
	awk_data->prmfns.sprintf = awk_sprintf;
	awk_data->prmfns.dprintf = awk_dprintf;
	awk_data->prmfns.data = ASE_NULL;

	ase_awk_setccls (awk, ASE_GETCCLS());
	ase_awk_setprmfns (awk, &awk_data->prmfns);

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == ASE_NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		ase_awk_close (awk);
#if defined(_WIN32) && defined(__DMC__)
		awk_free (heap, awk_data);
		HeapDestroy (heap);
#else
		awk_free (ASE_NULL, awk_data);
#endif
		return;
	}

	(*env)->SetLongField (env, obj, handle, (jlong)awk);

	opt = ASE_AWK_IMPLICIT |
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
	if (awk == ASE_NULL) return;

#if defined(_WIN32) && defined(_DEBUG) 
	OutputDebugStringW (L"<<<CLOSING AWK>>>\n");
#endif

	tmp = (awk_data_t*)ase_awk_getextension (awk);
#if defined(_WIN32) && defined(__DMC__)
	HANDLE heap = tmp->heap;
#endif
	ase_awk_close (awk);

#if defined(_WIN32) && defined(__DMC__)
	awk_free (heap, tmp);
	HeapDestroy (heap);
#else
	awk_free (ASE_NULL, tmp);
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
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);

	srcio_data.env = env;
	srcio_data.obj = obj;

	srcios.in = read_source;
	srcios.out = write_source;
	srcios.data = &srcio_data;

	if (ase_awk_parse (awk, &srcios) == -1)
	{
		THROW_AWK_EXCEPTION (env, awk);
		return;
	}
}

#define DELETE_CLASS_REFS(env, run_data) \
	do { \
		(*env)->DeleteLocalRef (env, run_data.exception_class); \
		(*env)->DeleteLocalRef (env, run_data.context_class); \
		(*env)->DeleteLocalRef (env, run_data.argument_class); \
		(*env)->DeleteLocalRef (env, run_data.return_class); \
		if (run_data.context_object != ASE_NULL) \
			(*env)->DeleteLocalRef (env, run_data.context_object); \
	} while (0)

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
	ase_awk_runarg_t* runarg = ASE_NULL;

	jsize len, i, j;
	const jchar* ptr;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);

	memset (&run_data, 0, sizeof(run_data));

	run_data.env = env;
	run_data.obj = obj;

	run_data.errnum = ASE_AWK_ENOERR;
	run_data.errlin = 0;

	/* would global reference be necessary? */
	run_data.exception_class = (*env)->FindClass (env, CLASS_EXCEPTION);
	run_data.context_class = (*env)->FindClass (env, CLASS_CONTEXT);
	run_data.argument_class = (*env)->FindClass (env, CLASS_ARGUMENT);
	run_data.return_class = (*env)->FindClass (env, CLASS_RETURN);

	ASE_ASSERT (run_data.exception_class != ASE_NULL);
	ASE_ASSERT (run_data.context_class != ASE_NULL);
	ASE_ASSERT (run_data.argument_class != ASE_NULL);
	ASE_ASSERT (run_data.return_class != ASE_NULL);

	run_data.context_init = (*env)->GetMethodID (
		env, run_data.context_class, "<init>", "(Lase/awk/Awk;)V");
	run_data.argument_init = (*env)->GetMethodID (
		env, run_data.argument_class, "<init>", "(Lase/awk/Context;JJ)V");
	run_data.return_init = (*env)->GetMethodID (
		env, run_data.return_class, "<init>", "(JJ)V");

	ASE_ASSERT (run_data.context_init != ASE_NULL);
	ASE_ASSERT (run_data.argument_init != ASE_NULL);
	ASE_ASSERT (run_data.return_init != ASE_NULL);

	run_data.exception_get_code = (*env)->GetMethodID (
		env, run_data.exception_class, "getCode", "()I");
	run_data.exception_get_line = (*env)->GetMethodID (
		env, run_data.exception_class, "getLine", "()I");
	run_data.exception_get_message = (*env)->GetMethodID (
		env, run_data.exception_class, "getMessage", "()Ljava/lang/String;");

	ASE_ASSERT (run_data.exception_get_code != ASE_NULL);
	ASE_ASSERT (run_data.exception_get_line != ASE_NULL);
	ASE_ASSERT (run_data.exception_get_message != ASE_NULL);

	run_data.context_clear = (*env)->GetMethodID (
		env, run_data.context_class, "clear", "()V");

	ASE_ASSERT (run_data.context_clear != ASE_NULL);

	run_data.argument_valid = (*env)->GetFieldID (
		env, run_data.argument_class, FIELD_VALID, "J");
	run_data.return_valid = (*env)->GetFieldID (
		env, run_data.return_class, FIELD_VALID, "J");
	if (run_data.argument_valid == ASE_NULL ||
	    run_data.return_valid == ASE_NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		DELETE_CLASS_REFS (env, run_data);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	/* No NewGlobalRef are needed on obj and run_data->context_object
	 * because they are valid while this run method runs */
	run_data.context_object = (*env)->NewObject (
		env, run_data.context_class, run_data.context_init, obj);
	if (run_data.context_object == ASE_NULL)
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
	runios.data = &runio_data;

	ASE_MEMSET (&runcbs, 0, ASE_SIZEOF(runcbs));
	runcbs.on_start = on_run_start;
	runcbs.on_end = on_run_end;
	runcbs.data = ASE_NULL;

	if (mfn == ASE_NULL) 
	{
		mmm = ASE_NULL;
		ptr = ASE_NULL;
	}
	else
	{
		/* process the main entry point */
		len = (*env)->GetStringLength (env, mfn);

		if (len > 0)
		{
			ptr = (*env)->GetStringChars (env, mfn, JNI_FALSE);
			if (ptr == ASE_NULL)
			{
				(*env)->ExceptionClear (env);
				DELETE_CLASS_REFS (env, run_data);
				THROW_NOMEM_EXCEPTION (env);
				return;
			}

			mmm = (ase_char_t*) ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
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
			mmm = ASE_NULL;
			ptr = ASE_NULL;
		}
	}

	if (args != ASE_NULL)
	{
		/* compose arguments */

		len = (*env)->GetArrayLength (env, args);

		runarg = ase_awk_alloc (awk, sizeof(ase_awk_runarg_t) * (len+1));
		if (runarg == ASE_NULL)
		{
			if (mmm != ASE_NULL) ase_awk_free (awk, mmm);
			if (ptr != ASE_NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
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
			if (tmp == ASE_NULL)
			{
				for (j = 0; j < i; j++) ase_awk_free (awk, runarg[j].ptr);
				ase_awk_free (awk, runarg);

				(*env)->DeleteLocalRef (env, obj);

				if (mmm != ASE_NULL && mmm) ase_awk_free (awk, mmm);
				if (ptr != ASE_NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
				DELETE_CLASS_REFS (env, run_data);

				THROW_NOMEM_EXCEPTION (env);
				return;
			}

			runarg[i].ptr = dup_str (awk, tmp, runarg[i].len);
			if (runarg[i].ptr == ASE_NULL)
			{
				for (j = 0; j < i; j++) ase_awk_free (awk, runarg[j].ptr);
				ase_awk_free (awk, runarg);

				(*env)->ReleaseStringChars (env, obj, tmp);
				(*env)->DeleteLocalRef (env, obj);

				if (mmm != ASE_NULL) ase_awk_free (awk, mmm);
				if (ptr != ASE_NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
				DELETE_CLASS_REFS (env, run_data);
				THROW_NOMEM_EXCEPTION (env);
				return;
			}
			
			(*env)->ReleaseStringChars (env, obj, tmp);
			(*env)->DeleteLocalRef (env, obj);
		}

		runarg[i].ptr = ASE_NULL;
		runarg[i].len = 0;
	}

	if (ase_awk_run (awk, mmm, &runios, &runcbs, runarg, &run_data) == -1 ||
	    run_data.errnum != ASE_AWK_ENOERR)
	{
		if (runarg != ASE_NULL)
		{
			for (i = 0; i < len; i++) ase_awk_free (awk, runarg[i].ptr);
			ase_awk_free (awk, runarg);
		}

		if (mmm != ASE_NULL) ase_awk_free (awk, mmm);
		if (ptr != ASE_NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
		DELETE_CLASS_REFS (env, run_data);

		if (run_data.errnum != ASE_AWK_ENOERR)
		{
			throw_exception (
				env, 
				run_data.errmsg,
				(jint)run_data.errnum,
				(jint)run_data.errlin);
		}
		else THROW_AWK_EXCEPTION (env, awk);
		return;
	}

	if (runarg != ASE_NULL)
	{
		for (i = 0; i < len; i++) ase_awk_free (awk, runarg[i].ptr);
		ase_awk_free (awk, runarg);
	}

	if (mmm != ASE_NULL) ase_awk_free (awk, mmm);
	if (ptr != ASE_NULL) (*env)->ReleaseStringChars (env, mfn, ptr);
	DELETE_CLASS_REFS (env, run_data);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_stop (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);
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
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL) 
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
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL) 
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
	jsize chunk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	mid = (*env)->GetMethodID (env, class, "readSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	chunk = (size > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)size;

	array = (*env)->NewCharArray (env, chunk);
	if (array == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, array, chunk);
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
	ase_awk_t* awk;
	jsize chunk, i;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	mid = (*env)->GetMethodID (env, class, "writeSource", "([CI)I");
	(*env)->DeleteLocalRef (env, class);
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* handle upto 'chunk' characters, let the rest be handled by
	 * the underlying engine */
	chunk = (size > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)size;

	array = (*env)->NewCharArray (env, chunk);
	if (array == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < chunk; i++) tmp[i] = (jchar)buf[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, array, chunk);
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
	
	/* get the method - meth */
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* look for extio class */
	extio_class = (*env)->FindClass (env, CLASS_EXTIO);
	if (extio_class == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* get the constructor */
	extio_cons = (*env)->GetMethodID (
		env, extio_class, "<init>", "(Ljava/lang/String;IIJ)V");
	if (extio_cons == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* construct the name */
	extio_name = new_str (env, awk, extio->name, ase_strlen(extio->name));
	if (extio_name == ASE_NULL) 
	{
		(*env)->DeleteLocalRef (env, extio_class);
		return -1;
	}

	/* construct the extio object */
	extio_object = (*env)->NewObject (
		env, extio_class, extio_cons, 
		extio_name, extio->type & 0xFF, extio->mode, (jlong)extio->run);
	(*env)->DeleteLocalRef (env, extio_class);
	if (extio_object == ASE_NULL) 
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
		if (extio->handle == ASE_NULL) 
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

	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == ASE_NULL) 
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
		extio->handle = ASE_NULL;
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
	jsize chunk;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);

	if (mid == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	/* shrink the buffer if it exceeds java's capability */
	chunk = (size > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)size;

	array = (*env)->NewCharArray (env, chunk);
	if (array == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, chunk);
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
	ase_awk_t* awk;
	jsize chunk, i;
	
	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	mid = (*env)->GetMethodID (env, class, meth, "(Lase/awk/Extio;[CI)I");
	(*env)->DeleteLocalRef (env, class);

	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	chunk = (size > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)size;

	array = (*env)->NewCharArray (env, chunk);
	if (array == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return -1;
	}

	tmp = (*env)->GetCharArrayElements (env, array, 0);
	for (i = 0; i < chunk; i++) tmp[i] = (jchar)data[i]; 
	(*env)->ReleaseCharArrayElements (env, array, tmp, 0);

	ret = (*env)->CallIntMethod (env, obj, mid, extio->handle, array, chunk);
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
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == ASE_NULL) 
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
	if (handle == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		return -1;
	}
	awk = (ase_awk_t*)(*env)->GetLongField (env, obj, handle);
	if (mid == ASE_NULL) 
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
	runio_data_t* runio_data = (runio_data_t*)epa->data;

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
	run_data_t* run_data;
	ase_awk_t* awk;

	JNIEnv* env; 
	jobject obj;

	jclass class; 
	jmethodID method;
	jthrowable throwable;
	jstring name;
	jsize i, nargs;
	jobjectArray args;
	jobject arg, ret;
	ase_awk_val_t* v;
	jlong vi;

	ase_size_t nargs_ase;

	awk = ase_awk_getrunawk (run);
	run_data = ase_awk_getruncustomdata (run);

	nargs_ase = ase_awk_getnargs (run);
	nargs = (nargs_ase > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)nargs_ase;

	env = run_data->env;
	obj = run_data->obj;

	name = new_str (env, awk, fnm, fnl);
	if (name == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	class = (*env)->GetObjectClass(env, obj);
	method = (*env)->GetMethodID (
		env, class, "handleFunction", 
		"(Lase/awk/Context;Ljava/lang/String;Lase/awk/Return;[Lase/awk/Argument;)V");
	(*env)->DeleteLocalRef (env, class);
	if (method == ASE_NULL) 
	{
		/* if the method is not found, the exception is thrown.
		 * clear it to prevent it from being thrown */
		(*env)->DeleteLocalRef (env, name);
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_EINTERN);
		return -1;
	}

	ret = (*env)->NewObject (env,
		run_data->return_class, run_data->return_init, 
		(jlong)run, (jlong)0);
	if (ret == ASE_NULL)
	{
		(*env)->DeleteLocalRef (env, name);
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	args = (*env)->NewObjectArray (
		env, nargs, run_data->argument_class, ASE_NULL);
	if (args == ASE_NULL)
	{
		/* no ref-down on ret.runid as it is still ase_awk_val_nil */
		(*env)->DeleteLocalRef (env, ret);

		(*env)->DeleteLocalRef (env, name);
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		v = ase_awk_getarg (run, i);

		/* these arguments are not registered for clearance into 
		 * the context. so ASE_NULL is passed as the first argument
		 * to the constructor of the Argument class. It is because
		 * the reference count of these arguments are still positive
		 * while this function runs. However, if you ever use an 
		 * argument outside the current context, it may cause 
		 * a program failure such as program crash */
		arg = (*env)->NewObject (env, 
			run_data->argument_class, run_data->argument_init, 
			ASE_NULL, (jlong)run, (jlong)v);
		if (arg == ASE_NULL)
		{
			if ((*env)->ExceptionCheck(env))
			{
				if (is_debug(awk)) 
					(*env)->ExceptionDescribe (env);
				(*env)->ExceptionClear (env);
			}
			(*env)->DeleteLocalRef (env, args);

			/* no ref-down on ret.runid as it is 
			 * still ase_awk_val_nil */
			(*env)->DeleteLocalRef (env, ret);
			(*env)->DeleteLocalRef (env, name);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		(*env)->SetObjectArrayElement (env, args, i, arg);
		if (arg != ASE_NULL) (*env)->DeleteLocalRef (env, arg);
	}

	(*env)->CallVoidMethod (
		env, obj, method, run_data->context_object, name, ret, args);
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

		/* invoke context.clear */
		(*env)->CallVoidMethod (env, run_data->context_object, run_data->context_clear);
		if ((*env)->ExceptionCheck(env))
		{
			if (is_debug(awk)) (*env)->ExceptionDescribe (env);
			(*env)->ExceptionClear (env);
		}

		/* refdown on ret.valid is needed here */
		vi = (*env)->GetLongField (env, ret, run_data->return_valid);
		if (vi != 0) 
		{
			ase_awk_refdownval (run, (ase_awk_val_t*)vi);
			(*env)->SetLongField (env, ret, run_data->return_valid ,(jlong)0);
		}

		(*env)->DeleteLocalRef (env, ret);
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

		if (mesg == ASE_NULL)
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
			rptr = (ase_char_t*) ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
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

	vi = (*env)->GetLongField (env, ret, run_data->return_valid);
	if (vi != 0)
	{
		ase_awk_setretval (run, (ase_awk_val_t*)vi);

		/* invalidate the value field in return object */
		ase_awk_refdownval (run, (ase_awk_val_t*)vi);
		(*env)->SetLongField (env, ret, run_data->return_valid, (jlong)0);
	}
	
	(*env)->DeleteLocalRef (env, args);
	(*env)->DeleteLocalRef (env, ret); 
	(*env)->DeleteLocalRef (env, name);

	(*env)->CallVoidMethod (env, 
		run_data->context_object, run_data->context_clear);
	if ((*env)->ExceptionCheck(env))
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		ase_awk_setrunerrnum (run, ASE_AWK_EBFNIMPL);
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
	if (handle == ASE_NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, name, ptr);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		for (x =  0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];
		n = (ase_awk_addfunc (awk, tmp, len, 0, 
			min_args, max_args, ASE_NULL, handle_bfn) == ASE_NULL)? -1: 0;
		ase_awk_free (awk, tmp);
	}
	else
	{
		n = (ase_awk_addfunc (awk, (ase_char_t*)ptr, len, 0, 
			min_args, max_args, ASE_NULL, handle_bfn) == ASE_NULL)? -1: 0;
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
	if (handle == ASE_NULL) 
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return;
	}

	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, name);
	ptr = (*env)->GetStringChars (env, name, JNI_FALSE);
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
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
	EXCEPTION_ON_ASE_NULL_AWK_RETURNING (env, awk, 0);
	return (jint)ase_awk_getmaxdepth (awk, id);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setmaxdepth (JNIEnv* env, jobject obj, jlong awkid, jint ids, jint depth)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);
	ase_awk_setmaxdepth (awk, ids, depth);
}

JNIEXPORT jint JNICALL Java_ase_awk_Awk_getoption (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK_RETURNING (env, awk, 0);
	return ase_awk_getoption (awk);
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setoption (JNIEnv* env, jobject obj, jlong awkid, jint options)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);
	ase_awk_setoption (awk, (int)options);
}

JNIEXPORT jboolean JNICALL Java_ase_awk_Awk_getdebug (JNIEnv* env, jobject obj, jlong awkid)
{
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK_RETURNING (env, awk, JNI_FALSE);
	return ((awk_data_t*)ase_awk_getextension(awk))->debug? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setdebug (
	JNIEnv* env, jobject obj, jlong awkid, jboolean debug)
{	
	ase_awk_t* awk = (ase_awk_t*)awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);
	((awk_data_t*)ase_awk_getextension(awk))->debug = debug;
}

JNIEXPORT jstring JNICALL Java_ase_awk_Awk_getword (
	JNIEnv* env, jobject obj, jlong awkid, jstring ow)
{
	ase_awk_t* awk;
	const jchar* ojp = ASE_NULL;
	jsize olen = 0;
	ase_size_t nlen;
	ase_char_t* oap, * nap;
	jstring ret;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_ASE_NULL_AWK_RETURNING (env, awk, ASE_NULL);

	if (ow == ASE_NULL) return ASE_NULL;

	if (get_str(env,awk,ow,&ojp,&oap,&olen) == -1)
	{
		THROW_NOMEM_EXCEPTION (env);
		return ASE_NULL;
	}

	if (ase_awk_getword (awk, oap, olen, &nap, &nlen) == -1)
	{
		free_str (env, awk, ow, ojp, oap);
		return ASE_NULL;
	}

	free_str (env, awk, ow, ojp, oap);

	ret = new_str (env, awk, nap, nlen);
	if (ret == ASE_NULL) THROW_NOMEM_EXCEPTION (env);
	return ret;
}

JNIEXPORT void JNICALL Java_ase_awk_Awk_setword (
	JNIEnv* env, jobject obj, jlong awkid, jstring ow, jstring nw)
{	
	ase_awk_t* awk;
	const jchar* ojp = ASE_NULL, * njp = ASE_NULL;
	jsize olen = 0, nlen = 0;
	ase_char_t* oap = ASE_NULL, * nap = ASE_NULL;
	jint r;

	awk = (ase_awk_t*) awkid;
	EXCEPTION_ON_ASE_NULL_AWK (env, awk);

	if (ow != ASE_NULL)
	{
		if (get_str(env,awk,ow,&ojp,&oap,&olen) == -1)
		{
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}

	if (nw != ASE_NULL)
	{
		if (get_str(env,awk,nw,&njp,&nap,&nlen) == -1)
		{
			if (ow != ASE_NULL) free_str (env, awk, ow, ojp, oap);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}

	r = ase_awk_setword (awk, oap, olen, nap, nlen);

	if (nw != ASE_NULL) free_str (env, awk, nw, njp, nap);
	if (ow != ASE_NULL) free_str (env, awk, ow, ojp, oap);

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
	if (ptr == ASE_NULL) 
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
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
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;
		ase_char_t* tmp = (ase_char_t*)
			ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
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
	jstring ret;

	jclass class; 
	jfieldID handle;
	ase_awk_t* awk;

	class = (*env)->GetObjectClass(env, obj);
	handle = (*env)->GetFieldID (env, class, FIELD_AWKID, "J");
	(*env)->DeleteLocalRef (env, class);
	if (handle == ASE_NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return ASE_NULL;
	}
	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, fmt);
	ptr = (*env)->GetStringChars (env, fmt, JNI_FALSE);
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return ASE_NULL;
	}

	tmp = (ase_char_t*) ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
	if (tmp == ASE_NULL)
	{
		(*env)->ReleaseStringChars (env, fmt, ptr);
		THROW_NOMEM_EXCEPTION (env);
		return ASE_NULL;
	}

	for (i = 0; i < len; i++) tmp[i] = (ase_char_t)ptr[i];
	tmp[i] = ASE_T('\0');

	len = wcsftime (buf, ASE_COUNTOF(buf), tmp, tm);

	ase_awk_free (awk, tmp);
	(*env)->ReleaseStringChars (env, fmt, ptr);

	ret = new_str (env, awk, buf, len);
	if (ret == ASE_NULL) THROW_NOMEM_EXCEPTION (env);
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
	if (handle == ASE_NULL)
	{
		/* internal error. no handle field 
		 * NoSuchFieldError, ExceptionInitializerError, 
		 * OutOfMemoryError might have occurred */
		return -1;
	}
	awk = (ase_awk_t*) (*env)->GetLongField (env, obj, handle);

	len = (*env)->GetStringLength (env, cmd);
	ptr = (*env)->GetStringChars (env, cmd, JNI_FALSE);
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return -1;
	}

	tmp = (ase_char_t*) ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*(len+1));
	if (tmp == ASE_NULL)
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
	char* mbs = (char*)ase_awk_alloc (awk, len*5+1);
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

JNIEXPORT void JNICALL Java_ase_awk_Context_setglobal (JNIEnv* env, jobject obj, jlong runid, jint id, jobject ret)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	run_data_t* run_data;
	ase_awk_val_t* v;
	jlong vi;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	vi = (*env)->GetLongField (env, ret, run_data->return_valid);
	v = (vi == 0)? ase_awk_val_nil: (ase_awk_val_t*)vi;
	/* invalidate the value field in the return object */
	(*env)->SetLongField (env, ret, run_data->return_valid, (jlong)0);

	if (ase_awk_setglobal(run,id,v) == -1)
	{
		if (vi != 0) ase_awk_refdownval (run, v);
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (vi != 0) ase_awk_refdownval (run, v);
}

JNIEXPORT jobject JNICALL Java_ase_awk_Context_getglobal (JNIEnv* env, jobject obj, jlong runid, jint id)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_t* awk;
	run_data_t* run_data;
	ase_awk_val_t* g;
	jobject arg;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);
	awk = ase_awk_getrunawk (run);

	g = ase_awk_getglobal(run, id);

	ASE_ASSERTX ((*env)->IsSameObject(env,obj,run_data->context_object),
		"this object(obj) should be the same object as the context object(run_data->context_object)");

	arg = (*env)->NewObject (env, 
		run_data->argument_class, run_data->argument_init, 
		obj, (jlong)run, (jlong)g);
	if (arg == ASE_NULL) 
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		THROW_NOMEM_EXCEPTION (env);
		return ASE_NULL;
	}

	/* the reference is incremented. this incremented reference is
	 * decremented in Argument.clear called from Context.clear.
	 * Note that the context object (obj) is passed to the contrustor of
	 * the argument class in the call to NewObject above */
	ase_awk_refupval (run, g);
	return arg;
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
	jstring ret = ASE_NULL;

	str = ase_awk_valtostr (
		run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
	if (str == ASE_NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return ret;
	}

	ret = new_str (env, awk, str, len);
	ase_awk_free (awk, str);

	if (ret == ASE_NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
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
	ase_pair_t* pair;
	jobject arg;

	const jchar* ptr;
	ase_char_t* rptr;
	ase_size_t len;

	if (ASE_AWK_VAL_TYPE(val) != ASE_AWK_VAL_MAP) return ASE_NULL;

	len = (*env)->GetStringLength (env, index);
	ptr = (*env)->GetStringChars (env, index, JNI_FALSE);
	if (ptr == ASE_NULL) goto nomem;

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_size_t x;
		rptr = (ase_char_t*) ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (rptr == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, index, ptr);
			goto nomem;
		}
		for (x = 0; x < len; x++) rptr[x] = (ase_char_t)ptr[x];
	}
	else rptr = (ase_char_t*)ptr;

	pair = ase_map_search (((ase_awk_val_map_t*)val)->map, rptr, len);
	if (ptr != rptr) ase_awk_free (awk, rptr);
	(*env)->ReleaseStringChars (env, index, ptr);

	/* the key is not found. it is not an error. val is just nil */
	if (pair == ASE_NULL) return ASE_NULL;; 

	arg = (*env)->NewObject (env, 
		run_data->argument_class, run_data->argument_init, 
		run_data->context_object, (jlong)run, (jlong)pair->val);
	if (arg == ASE_NULL) goto nomem;

	/* the reference is incremented. this incremented reference is
	 * decremented in Argument.clear called from Context.clear.
	 * Note that the context object (run_data->context_object) is 
	 * passed to the contrustor of the argument class in the call 
	 * to NewObject above */
	ase_awk_refupval (run, pair->val);
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

JNIEXPORT void JNICALL Java_ase_awk_Argument_clearval (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	run_data_t* run_data;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	if (val != ASE_NULL) ase_awk_refdownval (run, val);
	(*env)->SetLongField (env, obj, run_data->argument_valid, (jlong)0);
}


JNIEXPORT jboolean JNICALL Java_ase_awk_Return_isindexed (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;

	return (val != ASE_NULL && ASE_AWK_VAL_TYPE(val) == ASE_AWK_VAL_MAP)? JNI_TRUE: JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setintval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jlong newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	run_data_t* run_data;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	nv = ase_awk_makeintval (run, newval);
	if (nv == ASE_NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (val != ASE_NULL) ase_awk_refdownval (run, val);

	(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)nv);
	ase_awk_refupval (run, nv);
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jdouble newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	run_data_t* run_data;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	nv = ase_awk_makerealval (run, newval);
	if (nv == ASE_NULL) 
	{
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (val != ASE_NULL) ase_awk_refdownval (run, val);

	(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)nv);
	ase_awk_refupval (run, nv);
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setstrval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_val_t* nv;
	ase_awk_t* awk;
	run_data_t* run_data;

	const jchar* jptr;
	ase_char_t* aptr;
	jsize len;

	awk = ase_awk_getrunawk (run);
	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	if (get_str(env,awk,newval,&jptr,&aptr,&len) == -1)
	{
		THROW_NOMEM_EXCEPTION (env);
		return;
	}

	nv = (ase_awk_val_t*)ase_awk_makestrval (run, aptr, len);
	if (nv == ASE_NULL) 
	{
		free_str (env, awk, newval, jptr, aptr);
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	free_str (env, awk, newval, jptr, aptr);
	if (val != ASE_NULL) ase_awk_refdownval (run, val);

	(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)nv);
	ase_awk_refupval (run, nv);
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setindexedintval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring index, jlong newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_t* awk;
	run_data_t* run_data;

	int opt;
	const jchar* jptr;
	ase_char_t* aptr;
	jsize len;

	awk = ase_awk_getrunawk (run);
	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	opt = ase_awk_getoption (awk);
	if ((opt & ASE_AWK_MAPTOVAR) != ASE_AWK_MAPTOVAR)
	{
		/* refer to run_return in run.c */
		ase_awk_setrunerrnum (run, ASE_AWK_EMAPNOTALLOWED);
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (val == ASE_NULL || ASE_AWK_VAL_TYPE(val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x;
	       	ase_awk_val_t* x2;
		ase_pair_t* pair;
	
		x = ase_awk_makemapval (run);
		if (x == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x);

		x2 = ase_awk_makeintval (run, newval);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		if (val != ASE_NULL) ase_awk_refdownval (run, val);
		(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)x);
	}
	else
	{
		ase_awk_val_t* x2;
		ase_pair_t* pair;

		x2 = ase_awk_makeintval (run, newval);
		if (x2 == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		pair = ase_map_upsert (
			((ase_awk_val_map_t*)val)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setindexedrealval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring index, jdouble newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_t* awk;
	run_data_t* run_data;

	int opt;
	const jchar* jptr;
	ase_char_t* aptr;
	jsize len;

	awk = ase_awk_getrunawk (run);
	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	opt = ase_awk_getoption (awk);
	if ((opt & ASE_AWK_MAPTOVAR) != ASE_AWK_MAPTOVAR)
	{
		/* refer to run_return in run.c */
		ase_awk_setrunerrnum (run, ASE_AWK_EMAPNOTALLOWED);
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (val == ASE_NULL || ASE_AWK_VAL_TYPE(val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x;
	       	ase_awk_val_t* x2;
		ase_pair_t* pair;
	
		x = ase_awk_makemapval (run);
		if (x == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x);

		x2 = ase_awk_makerealval (run, newval);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}

		pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		if (val != ASE_NULL) ase_awk_refdownval (run, val);
		(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)x);
	}
	else
	{
		ase_awk_val_t* x2;
		ase_pair_t* pair;

		x2 = ase_awk_makerealval (run, newval);
		if (x2 == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		pair = ase_map_upsert (
			((ase_awk_val_map_t*)val)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Return_setindexedstrval (JNIEnv* env, jobject obj, jlong runid, jlong valid, jstring index, jstring newval)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	ase_awk_t* awk;
	run_data_t* run_data;

	int opt;
	const jchar* jptr;
	ase_char_t* aptr;
	jsize len;

	awk = ase_awk_getrunawk (run);
	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	opt = ase_awk_getoption (awk);
	if ((opt & ASE_AWK_MAPTOVAR) != ASE_AWK_MAPTOVAR)
	{
		/* refer to run_return in run.c */
		ase_awk_setrunerrnum (run, ASE_AWK_EMAPNOTALLOWED);
		THROW_RUN_EXCEPTION (env, run);
		return;
	}

	if (val == ASE_NULL || ASE_AWK_VAL_TYPE(val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x;
	       	ase_awk_val_t* x2;
		ase_pair_t* pair;
	
		x = ase_awk_makemapval (run);
		if (x == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x);

		if (get_str(env,awk,newval,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		x2 = ase_awk_makestrval (run, aptr, len);
		free_str (env, awk, index, jptr, aptr);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			ase_awk_refdownval (run, x);
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		if (val != ASE_NULL) ase_awk_refdownval (run, val);
		(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)x);
	}
	else
	{
		ase_awk_val_t* x2;
		ase_pair_t* pair;

		if (get_str(env,awk,newval,&jptr,&aptr,&len) == -1)
		{
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		x2 = ase_awk_makestrval (run, aptr, len);
		free_str (env, awk, index, jptr, aptr);
		if (x2 == ASE_NULL) 
		{
			THROW_RUN_EXCEPTION (env, run);
			return;
		}

		ase_awk_refupval (run, x2);

		if (get_str(env,awk,index,&jptr,&aptr,&len) == -1)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
		pair = ase_map_upsert (
			((ase_awk_val_map_t*)val)->map, aptr, len, x2);
		free_str (env, awk, index, jptr, aptr);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (run, x2);
			THROW_NOMEM_EXCEPTION (env);
			return;
		}
	}
}

JNIEXPORT void JNICALL Java_ase_awk_Return_clearval (JNIEnv* env, jobject obj, jlong runid, jlong valid)
{
	ase_awk_run_t* run = (ase_awk_run_t*)runid;
	ase_awk_val_t* val = (ase_awk_val_t*)valid;
	run_data_t* run_data;

	run_data = (run_data_t*)ase_awk_getruncustomdata (run);

	if (val != ASE_NULL) ase_awk_refdownval (run, val);
	(*env)->SetLongField (env, obj, run_data->return_valid, (jlong)0);
}

static ase_char_t* dup_str (ase_awk_t* awk, const jchar* str, ase_size_t len)
{
	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		ase_char_t* tmp;
		ase_size_t i;

		tmp = (ase_char_t*) ase_awk_alloc (awk, (len+1) * ASE_SIZEOF(ase_char_t));
		if (tmp == ASE_NULL) return ASE_NULL;

		for (i = 0; i < (ase_size_t)len; i++) tmp[i] = (ase_char_t)str[i];
		tmp[i] = ASE_T('\0');

		return tmp;
	}
	else
	{
		ase_char_t* tmp;

		tmp = (ase_char_t*) ase_awk_alloc (awk, (len+1) * ASE_SIZEOF(ase_char_t));
		if (tmp == ASE_NULL) return ASE_NULL;

		ase_strncpy (tmp, (ase_char_t*)str, len);
		return tmp;
	}
}

static int get_str (
	JNIEnv* env, ase_awk_t* awk, jstring str, 
	const jchar** jptr, ase_char_t** aptr, jsize* outlen)
{
	jsize len;
	const jchar* ptr;

	len = (*env)->GetStringLength (env, str);
	ptr = (*env)->GetStringChars (env, str, JNI_FALSE);
	if (ptr == ASE_NULL)
	{
		(*env)->ExceptionClear (env);
		return -1;
	}

	if (len > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jsize x;

		ase_char_t* tmp = (ase_char_t*)
			ase_awk_alloc (awk, ASE_SIZEOF(ase_char_t)*len);
		if (tmp == ASE_NULL)
		{
			(*env)->ReleaseStringChars (env, str, ptr);
			return -1;
		}

		for (x = 0; x < len; x++) tmp[x] = (ase_char_t)ptr[x];

		*outlen = len;
		*jptr = ptr;
		*aptr = tmp;
	}
	else
	{
		*outlen = len;
		*jptr = ptr;
		*aptr = (ase_char_t*)ptr;
	}

	return 0;
}

static void free_str (
	JNIEnv* env, ase_awk_t* awk, jstring str, 
	const jchar* jptr, ase_char_t* aptr)
{
	if (jptr != aptr) ase_awk_free (awk, aptr);
	(*env)->ReleaseStringChars (env, str, jptr);
}


static jstring new_str (
	JNIEnv* env, ase_awk_t* awk, const ase_char_t* ptr, ase_size_t len)
{
	jstring ret;
	jsize i, chunk;
       
	chunk = (len > ASE_TYPE_MAX(jsize))? ASE_TYPE_MAX(jsize): (jsize)len;
	if (chunk > 0 && ASE_SIZEOF(jchar) != ASE_SIZEOF(ase_char_t))
	{
		jchar* tmp = (jchar*) ase_awk_alloc (awk, ASE_SIZEOF(jchar)*chunk);
		if (tmp == ASE_NULL) return ASE_NULL;

		for (i = 0; i < chunk; i++) tmp[i] = (jchar)ptr[i];
		ret = (*env)->NewString (env, tmp, chunk);
		ase_awk_free (awk, tmp);
	}
	else
	{

		ret = (*env)->NewString (env, (jchar*)ptr, chunk);
	}

	if (ret == ASE_NULL)
	{
		if (is_debug(awk)) (*env)->ExceptionDescribe (env);
		(*env)->ExceptionClear (env);
		return ASE_NULL;
	}

	return ret;
}
