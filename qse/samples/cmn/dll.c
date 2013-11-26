#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/dll.h>
#include <qse/cmn/sio.h>


#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_dll_walk_t walk_dll (qse_dll_t* dll, qse_dll_node_t* n, void* arg)
{
	qse_printf (QSE_T("[%.*s]\n"), (int)QSE_DLL_DLEN(n), QSE_DLL_DPTR(n));
	return QSE_DLL_WALK_FORWARD;
}

static qse_dll_walk_t rwalk_dll (qse_dll_t* dll, qse_dll_node_t* n, void* arg)
{
	qse_printf (QSE_T("[%.*s]\n"), (int)QSE_DLL_DLEN(n), QSE_DLL_DPTR(n));
	return QSE_DLL_WALK_BACKWARD;
}

static int test1 ()
{
	qse_dll_t* s1;
	qse_dll_node_t* p;
	qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the hack"),
		QSE_T("do you like it?")
	};
	int i;

	s1 = qse_dll_open (QSE_MMGR_GETDFL(), 0);	
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_dll_setcopier (s1, QSE_DLL_COPIER_INLINE);
	qse_dll_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_dll_pushtail (s1, x[i], qse_strlen(x[i]));
	}
	qse_printf (QSE_T("s1 holding [%zu] nodes\n"), QSE_DLL_SIZE(s1));
	qse_dll_walk (s1, walk_dll, QSE_NULL);
		

	p = qse_dll_search (s1, QSE_NULL, x[0], qse_strlen(x[0]));
	if (p != QSE_NULL)
	{
		qse_dll_delete (s1, p);
	}
	qse_printf (QSE_T("s1 holding [%zu] nodes\n"), QSE_DLL_SIZE(s1));
	qse_dll_walk (s1, walk_dll, QSE_NULL);

	qse_dll_close (s1);
	return 0;
}

static int test2 ()
{
	qse_dll_t* s1;
	qse_dll_node_t* p;
	qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the hack"),
		QSE_T("do you like it?")
	};
	int i;

	s1 = qse_dll_open (QSE_MMGR_GETDFL(), 0);	
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_dll_setcopier (s1, QSE_DLL_COPIER_INLINE);
	qse_dll_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_dll_pushtail (s1, x[i], qse_strlen(x[i]));
	}
	qse_printf (QSE_T("s1 holding [%zu] nodes\n"), QSE_DLL_SIZE(s1));
	qse_dll_rwalk (s1, rwalk_dll, QSE_NULL);
		

	p = qse_dll_search (s1, QSE_NULL, x[0], qse_strlen(x[0]));
	if (p != QSE_NULL)
	{
		qse_dll_delete (s1, p);
	}
	qse_printf (QSE_T("s1 holding [%zu] nodes\n"), QSE_DLL_SIZE(s1));
	qse_dll_rwalk (s1, rwalk_dll, QSE_NULL);

	qse_dll_close (s1);
	return 0;
}

int main ()
{
	qse_openstdsios ();

	R (test1);
	R (test2);

	qse_closestdsios ();
	return 0;
}
