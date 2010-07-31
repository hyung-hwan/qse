#include <qse/cmn/xma.h>
#include <qse/cmn/stdio.h>

int main ()
{
	int i;
	void* ptr[100];
	void* x;

	qse_xma_t* xma = qse_xma_open (QSE_NULL, 0, 100000L);
	if (xma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open xma\n"));
		return -1;
	}

	ptr[0] = qse_xma_alloc (xma, 5000);
	ptr[1] = qse_xma_alloc (xma, 1000);
	ptr[2] = qse_xma_alloc (xma, 3000);
	ptr[3] = qse_xma_alloc (xma, 1000);
	qse_xma_dump (xma);
	qse_xma_free (xma, ptr[0]);
	qse_xma_free (xma, ptr[2]);
	//qse_xma_free (xma, ptr[3]);

	qse_xma_dump (xma);
	qse_printf (QSE_T("===============================\n"));
	qse_xma_realloc (xma, ptr[1], 500);
	qse_xma_dump (xma);

#if 0
	for (i = 0; i < 100; i++)
	{
		int sz = (i + 1) * 10;
		/*int sz = 10240;*/
		ptr[i] = qse_xma_alloc (xma, sz);
		if (ptr[i] == QSE_NULL) 
		{
			qse_printf (QSE_T("failed to alloc %d\n"), sz);
			break;
		}
		qse_printf (QSE_T("%d %p\n"), sz, ptr[i]);
	}

	for (--i; i > 0; i-= 3)
	{
		if (i >= 0) qse_xma_free (xma, ptr[i]);
	}

/*
	qse_xma_free (xma, ptr[0]);
	qse_xma_free (xma, ptr[1]);
	qse_xma_free (xma, ptr[2]);
*/

	{
		void* x, * y;

		qse_printf (QSE_T("%p\n"), qse_xma_alloc (xma, 5000));
		qse_printf (QSE_T("%p\n"), qse_xma_alloc (xma, 1000));
		qse_printf (QSE_T("%p\n"), (x = qse_xma_alloc (xma, 10)));
		qse_printf (QSE_T("%p\n"), (y = qse_xma_alloc (xma, 40)));

		if (x) qse_xma_free (xma, x);
		if (y) qse_xma_free (xma, y);
		qse_printf (QSE_T("%p\n"), (x = qse_xma_alloc (xma, 10)));
		qse_printf (QSE_T("%p\n"), (y = qse_xma_alloc (xma, 40)));
	}
	qse_xma_dump (xma);
#endif

	qse_xma_close (xma);
	return 0;
}
