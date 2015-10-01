#include <qse/sys/rwl.h>
#include <qse/sys/thr.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/mem.h>

#include <locale.h>
#include <stdio.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

struct thr_xtn_t
{
	int id;
};
typedef struct thr_xtn_t thr_xtn_t;

qse_rwl_t* rwl;
qse_mtx_t* mtx;

#define OUTMSG(msg,id)  do { \
	qse_ntime_t now; \
	qse_gettime(&now); \
	qse_mtx_lock (mtx, QSE_NULL); \
	qse_printf (QSE_T("%10ld.%7ld thread %d: %s\n"), (long int)now.sec, (long int)now.nsec, id, msg); \
	qse_mtx_unlock (mtx); \
} while(0)

int thr_exec (qse_thr_t* thr)
{
	thr_xtn_t* xtn = qse_thr_getxtn(thr);

	OUTMSG (QSE_T("starting"), xtn->id);
	while (1)
	{
		if (xtn->id % 2)
		/*if (xtn->id > 0)*/
		{
			OUTMSG (QSE_T("read waiting"), xtn->id);

			qse_rwl_lockr (rwl, QSE_NULL);
			OUTMSG (QSE_T("read start"), xtn->id);
			/*sleep (1);*/
			OUTMSG (QSE_T("read done"), xtn->id);
			qse_rwl_unlockr (rwl);
		}
		else
		{
			qse_ntime_t x;
			OUTMSG (QSE_T("write waiting"), xtn->id);

			qse_inittime (&x, 3, 0);
			/*if (qse_rwl_lockw (rwl, QSE_NULL) >= 0)*/
			if (qse_rwl_lockw (rwl, &x) >= 0)
			{
				OUTMSG (QSE_T("write start"), xtn->id);
				/*sleep (1);*/
				OUTMSG (QSE_T("write done"), xtn->id);
				qse_rwl_unlockw (rwl);
				/*sleep (2);*/
			}
			else
			{
				OUTMSG (QSE_T("write fail"), xtn->id);
			}

		}
	}

	OUTMSG (QSE_T("exiting"), xtn->id);
	return 0;
}


void test_001 (void)
{
	qse_mmgr_t* mmgr;
	qse_thr_t* t[6];
	qse_size_t i;
	thr_xtn_t* xtn;

	mmgr = QSE_MMGR_GETDFL();

	mtx = qse_mtx_open (mmgr, 0);
	rwl = qse_rwl_open (mmgr, 0, 0/*QSE_RWL_PREFER_WRITER*/);

	for (i = 0; i < QSE_COUNTOF(t); i++)
	{
		t[i] = qse_thr_open (mmgr, QSE_SIZEOF(thr_xtn_t), thr_exec);
		xtn = qse_thr_getxtn(t[i]);
		xtn->id = i;
	}

	for (i = 0; i < QSE_COUNTOF(t); i++) qse_thr_start (t[i], 0);
	for (i = 0; i < QSE_COUNTOF(t); i++) qse_thr_join (t[i]);
	for (i = 0; i < QSE_COUNTOF(t); i++) qse_thr_close (t[i]);

	qse_rwl_close (rwl);
	qse_mtx_close (mtx);
}

int main ()
{
#if defined(_WIN32)
	char codepage[100];
	UINT old_cp = GetConsoleOutputCP();
	SetConsoleOutputCP (CP_UTF8);

#else
	setlocale (LC_ALL, "");
#endif

	qse_openstdsios ();

setbuf (stdout, NULL);
	test_001 ();

	qse_closestdsios ();

#if defined(_WIN32)
	SetConsoleOutputCP (old_cp);
#endif
	return 0;
}
