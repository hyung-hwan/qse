#include <qse/cmn/fma.h>
#include <qse/cmn/rbt.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/stdio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_rbt_mancbs_t mancbs =
{
	{
		QSE_RBT_COPIER_INLINE,
		QSE_RBT_COPIER_INLINE
	},
	{
		QSE_RBT_FREEER_DEFAULT,
		QSE_RBT_FREEER_DEFAULT
	},
	QSE_RBT_COMPER_DEFAULT,
	QSE_RBT_KEEPER_DEFAULT
};

static int test1 ()
{
	int i;
	int* ptr[100];

	qse_fma_t* fma = qse_fma_open (QSE_MMGR_GETDFL(), 0, sizeof(int), 10, 5);
	if (fma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open fma\n"));
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_fma_alloc (fma, sizeof(int));
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
		ptr[i] = qse_fma_alloc (fma, sizeof(int));
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

static qse_rbt_walk_t walk (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_printf (QSE_T("key = %lld, value = %lld\n"),
		*(long*)QSE_RBT_KPTR(pair), *(long*)QSE_RBT_VPTR(pair));
	return QSE_RBT_WALK_FORWARD;
}

static int test2 ()
{ 
	qse_mmgr_t mmgr = 
	{
		(qse_mmgr_alloc_t) qse_fma_alloc,
		(qse_mmgr_realloc_t) qse_fma_realloc,
		(qse_mmgr_free_t) qse_fma_free,
		QSE_NULL
	};
	qse_fma_t* fma;
	qse_rbt_t rbt;
	qse_size_t blksize;
	long x;

	           /* key */      /* value */      /* internal node */
	blksize = sizeof(long) + sizeof(long) + sizeof(qse_rbt_pair_t);

	fma = qse_fma_open (QSE_MMGR_GETDFL(), 0, blksize, 10, 0);
	if (fma == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a memory allocator\n"));
		return -1;
	}

	mmgr.ctx = fma;
	if (qse_rbt_init (&rbt, &mmgr, QSE_SIZEOF(long), QSE_SIZEOF(long)) == QSE_NULL)
	{
		qse_printf (QSE_T("cannot initialize a tree\n"));
		qse_fma_close (fma);
		return -1;
	}
	qse_rbt_setmancbs (&rbt, &mancbs);

	for (x = 10; x < 100; x++)
	{
		long y = x * x;
		if (qse_rbt_insert (&rbt, &x, 1, &y, 1) == QSE_NULL) 
		{
			qse_printf (QSE_T("failed to insert. out of memory\n"));
			break;
		}
	}

	qse_rbt_walk (&rbt, walk, QSE_NULL);

	qse_rbt_fini (&rbt);
	qse_fma_close (fma);

	return 0;
}

int main ()
{
	R (test1);
	R (test2);
	return 0;
}
