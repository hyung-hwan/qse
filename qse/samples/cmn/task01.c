#include <qse/cmn/task.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#    include <windows.h>
#endif

static qse_task_slice_t* print (
	qse_task_t* task, qse_task_slice_t* slice, void* ctx)
{
	int i;
	int num = (int)ctx;

	qse_printf (QSE_T("task[%03d] => starting\n"), num);
	for (i = 0; i < 5; i++) 
	{ 
		qse_printf (QSE_T("task[%03d] => %03d\n"), num, i); 

		qse_task_schedule (task, slice, QSE_NULL);	

		if (i == 2 && num == 1)
		{
			qse_task_create (task, print, (void*)99, 40000);
		}
	}

	qse_printf (QSE_T("task[%03d] => exiting\n"), num);
	return QSE_NULL; 
}

static int start_task (qse_task_fnc_t fnc)
{
	qse_task_t* task;
	qse_task_slice_t* slice;

	task = qse_task_open (QSE_MMGR_GETDFL(), 0);
	if (task == NULL)
	{
		qse_printf (QSE_T("cannot initialize tasking system\n"));
		return -1;
	}

	qse_printf (QSE_T("== END ==\n"));

	if (qse_task_create (task, fnc, (void*)1, 40000) == QSE_NULL ||
	    (slice = qse_task_create (task, fnc, (void*)2, 40000)) == QSE_NULL ||
	    qse_task_create (task, fnc, (void*)3, 40000) == QSE_NULL ||
	    qse_task_create (task, fnc, (void*)4, 40000) == QSE_NULL)
	{
		qse_printf (QSE_T("cannot create task slice\n"));
		qse_task_close (task);
		return -1;
	}

	if (qse_task_boot (task, slice) <= -1)
	{
		qse_printf (QSE_T("cannot start task\n"));
		qse_task_close (task);
		return -1;
	}

	qse_printf (QSE_T("== END ==\n"));

	qse_task_close (task);
	return 0;
}

static int test_main (int argc, qse_char_t* argv[])
{
	int ret;

	ret  = start_task (print);
	qse_printf (QSE_T("== END ==\n"));

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
#if defined(_WIN32)
	char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif
     return qse_runmain (argc, argv, test_main);
}

