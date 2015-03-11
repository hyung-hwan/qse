#include <qse/cmn/fma.h>
#include <qse/cmn/rbt.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/sio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_rbt_style_t style =
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

	qse_fma_t* fma = qse_fma_open (QSE_MMGR_GETDFL(), 0, sizeof(int), 10 /* max block size */, 5 /* max chunks */); 
	if (fma == QSE_NULL) 
	{
		qse_printf (QSE_T("cannot open fma\n"));
		return -1;
	}
	/* max 50 (10 * 5) allocations should be possible */

	for (i = 0; i < 100; i++)
	{
		ptr[i] = qse_fma_alloc (fma, sizeof(int));

		if (i < 50) QSE_ASSERT (ptr[i] != QSE_NULL);
		else QSE_ASSERT (ptr[i] == QSE_NULL);

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
		if (i < 50) QSE_ASSERT (ptr[i] != QSE_NULL);
		else QSE_ASSERT (ptr[i] == QSE_NULL);

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

static void* fma_alloc (qse_mmgr_t* mmgr, qse_size_t size)
{
	return qse_fma_alloc (mmgr->ctx, size);
}

static void* fma_realloc (qse_mmgr_t* mmgr, void* ptr, qse_size_t size)
{
	return qse_fma_realloc (mmgr->ctx, ptr, size);
}

static void fma_free (qse_mmgr_t* mmgr, void* ptr)
{
	qse_fma_free (mmgr->ctx, ptr);
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
		fma_alloc,
		fma_realloc,
		fma_free,
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
	if (qse_rbt_init (&rbt, &mmgr, QSE_SIZEOF(long int), QSE_SIZEOF(long int)) <= -1)
	{
		qse_printf (QSE_T("cannot initialize a tree\n"));
		qse_fma_close (fma);
		return -1;
	}

	qse_rbt_setstyle (&rbt, &style);


	for (x = 10; x < 100; x++)
	{
		long int y = x * x;

		if (qse_rbt_insert (&rbt, &x, 1, &y, 1) == QSE_NULL) 
		{
			qse_printf (QSE_T("failed to insert. out of memory\n"));
			break;
		}

		
	}

	for (x = 10; x < 105; x++)
	{
		long int y = x * x, yy;
		qse_rbt_pair_t* pair;

		pair = qse_rbt_search (&rbt, &x, 1);
		if (x < 100)
		{
			QSE_ASSERT (pair != QSE_NULL);
			yy = *(long int*)QSE_RBT_VPTR(pair);
			QSE_ASSERT (yy = y);
			qse_printf (QSE_T("%ld => %ld\n"), (long int)x, (long int)yy);
		}
		else
		{
			QSE_ASSERT (pair == QSE_NULL);
		}
	}

	qse_rbt_walk (&rbt, walk, QSE_NULL);

	qse_rbt_fini (&rbt);
	qse_fma_close (fma);

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
