#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include <qse/cmn/stdio.h>
#include <stdlib.h>


#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_rbt_walk_t walk (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_printf (QSE_T("key = %d, value = %d\n"), 
		*(int*)QSE_RBT_KPTR(pair), *(int*)QSE_RBT_VPTR(pair));
	return QSE_RBT_WALK_FORWARD;
}

static int test1 ()
{
	qse_rbt_t* s1;
	int i;

	s1 = qse_rbt_open (QSE_MMGR_GETDFL(), 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_rbt_setcopier (s1, QSE_RBT_KEY, QSE_RBT_COPIER_INLINE);
	qse_rbt_setcopier (s1, QSE_RBT_VAL, QSE_RBT_COPIER_INLINE);
	/*
	qse_rbt_setscale (s1, QSE_RBT_KEY, QSE_SIZEOF(int));
	qse_rbt_setscale (s1, QSE_RBT_VAL, QSE_SIZEOF(int));
	*/
	/*
	qse_rbt_setkeeper (s1, keeper1);
	*/

	for (i = 0; i < 20; i++)
	{
		int x = i * 20;
qse_printf (QSE_T("inserting at %d\n"), i);
		qse_rbt_insert (s1, &i, QSE_SIZEOF(i), &x, QSE_SIZEOF(x));
	}

	qse_rbt_rwalk (s1, walk, QSE_NULL);

	for (i = 0; i < 20; i += 2)
	{
qse_printf (QSE_T("deleting %d\n"), i);
		qse_rbt_delete (s1, &i, QSE_SIZEOF(i));
	}

	qse_rbt_rwalk (s1, walk, QSE_NULL);

	for (i = 0; i < 20; i++)
	{
		int x = i * 20;
qse_printf (QSE_T("inserting at %d\n"), i);
		qse_rbt_insert (s1, &i, QSE_SIZEOF(i), &x, QSE_SIZEOF(x));
	}

	qse_rbt_rwalk (s1, walk, QSE_NULL);

	qse_rbt_clear (s1);
	for (i = 20; i > 0; i--)
	{
		int x = i * 20;
qse_printf (QSE_T("inserting at %d\n"), i);
		qse_rbt_insert (s1, &i, QSE_SIZEOF(i), &x, QSE_SIZEOF(x));
	}

	qse_rbt_rwalk (s1, walk, QSE_NULL);

	for (i = 0; i < 20; i += 3)
	{
qse_printf (QSE_T("deleting %d\n"), i);
		qse_rbt_delete (s1, &i, QSE_SIZEOF(i));
	}

	qse_rbt_rwalk (s1, walk, QSE_NULL);

	qse_rbt_close (s1);
	return 0;
}

int main ()
{
	R (test1);
	return 0;
}
