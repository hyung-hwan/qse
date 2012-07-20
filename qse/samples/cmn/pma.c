#include <qse/cmn/pma.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/stdio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 ()
{
	int i;
	int* ptr[100];

	qse_pma_t* pma = qse_pma_open (QSE_MMGR_GETDFL(), 0);
	if (pma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open pma\n"));
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_pma_alloc (pma, sizeof(int));
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_pma_alloc (pma, sizeof(int));
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	qse_pma_close (pma);
	return 0;
}

static int test2 ()
{
	int i;
	int* ptr[100];

	qse_pma_t* pma = qse_pma_open (QSE_MMGR_GETDFL(), 0);
	if (pma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open pma\n"));
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_pma_alloc (pma, sizeof(int));
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	qse_pma_clear (pma);

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_pma_alloc (pma, sizeof(int));
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	qse_pma_close (pma);
	return 0;
}

int main ()
{
	R (test1);
	R (test2);
	return 0;
}
