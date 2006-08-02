/*
 * $Id: jni.c,v 1.2 2006-08-02 15:19:59 bacon Exp $
 */

#include "jni.h"
#include "awk.h"
#include "sa.h"

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_AWK_open (JNIEnv* env, jobject obj)
{
	xp_printf (XP_T("Java_AWK_open\n"));
}

JNIEXPORT void JNICALL Java_xpkit_xpj_awk_AWK_close (JNIEnv* env, jobject obj)
{
	xp_printf (XP_T("Java_AWK_close\n"));
}
