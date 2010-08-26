#include <qse/cmn/fma.h>
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

	qse_fma_t* fma = qse_fma_open (QSE_NULL, 0, sizeof(int), 10, 5);
	if (fma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open fma\n"));
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_fma_alloc (fma);
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	for (i = 0; i < 30; i+=2)
	{
		if (ptr[i])
		{
			qse_fma_free (fma, ptr[i]);
			ptr[i] = QSE_NULL;
		}
	}

	for (i = 0; i < 100; i++)
	{
		if (ptr[i])
		{
			qse_fma_free (fma, ptr[i]);
			ptr[i] = QSE_NULL;
		}
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_fma_alloc (fma);
		if (ptr[i]) 
		{
			qse_printf (QSE_T("%d %p\n"), i, ptr[i]);
			*(ptr[i]) = i;
		}
		else qse_printf (QSE_T("%d FAIL\n"), i);
	}

	qse_fma_close (fma);
	return 0;
}

int main ()
{
	R (test1);
	return 0;
}
