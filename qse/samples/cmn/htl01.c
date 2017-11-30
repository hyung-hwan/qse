#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htl.h>
#include <qse/si/sio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

typedef struct item_t item_t;
struct item_t
{
	long a;
	long x;
	long y;
};

static qse_htl_walk_t walk1 (qse_htl_t* htl, void* data, void* ctx)
{
	qse_printf (QSE_T("[%ld]\n"), *(long*)data);
 	return QSE_HTL_WALK_FORWARD;
}

static qse_htl_walk_t walk2 (qse_htl_t* htl, void* data, void* ctx)
{
	item_t* item = (item_t*)data;
	qse_printf (QSE_T("a [%ld] x [%ld] y [%ld]\n"), item->a, item->x, item->y);
	return QSE_HTL_WALK_FORWARD;
}

static qse_size_t hash_item (qse_htl_t* htl, const void* data)
{
	item_t* item = (item_t*)data;
	//return item->a + 123445;
	return qse_genhash (&item->a, QSE_SIZEOF(item->a));
}

static int comp_item (qse_htl_t* htl, const void* data1, const void* data2)
{
	return ((item_t*)data1)->a != ((item_t*)data2)->a;
}

static void* copy_item (qse_htl_t* htl, void* data)
{
	item_t* x = (item_t*)QSE_MMGR_ALLOC (htl->mmgr, QSE_SIZEOF(item_t));
	if (x) *x = *(item_t*)data;
	return x;
}

static void free_item (qse_htl_t* htl, void* data)
{
	QSE_MMGR_FREE (htl->mmgr, data);
}

static void* copy_long (qse_htl_t* htl, void* data)
{
	long* x = (long*)QSE_MMGR_ALLOC (htl->mmgr, QSE_SIZEOF(long));
	if (x) *x = *(long*)data;
	return x;
}

static void free_long (qse_htl_t* htl, void* data)
{
	QSE_MMGR_FREE (htl->mmgr, data);
}

static int test1 ()
{
	long x;
	qse_htl_t* htl;
	qse_htl_node_t* np;

	htl = qse_htl_open (QSE_MMGR_GETDFL(), 0, QSE_SIZEOF(x));
	if (htl == QSE_NULL)
	{
		qse_printf (QSE_T("failed to open a table\n"));
		return -1;
	}

	qse_htl_setcopier (htl, copy_long);
	qse_htl_setfreeer (htl, free_long);

	for (x = 9; x < 20; x++)
	{
		if (qse_htl_insert(htl, &x))
		{
			qse_printf (QSE_T("SUCCESS: inserted %ld\n"), x);
		}
		else
		{
			qse_printf (QSE_T("FAILURE: failed to insert %ld\n"), x);
		}
	}

	x = 10;
	if ((np = qse_htl_search(htl, &x)))
	{
		qse_printf (QSE_T("SUCCESS: found %ld\n"), *(long*)np->data);
		QSE_ASSERT (*(long*)np->data == x);
	}
	else
	{
		qse_printf (QSE_T("FAILURE: failed to found %ld\n"), x);
	}

	x = 10;
	if (qse_htl_delete(htl, &x) == 0)
	{
		qse_printf (QSE_T("SUCCESS: deleted %ld\n"), x);
	}
	else
	{
		qse_printf (QSE_T("FAILURE: failed to delete %ld\n"), x);
	}

	x = 10;
	qse_printf (QSE_T("searching for %ld\n"), x);
	np = qse_htl_search(htl, &x);
	QSE_ASSERT (np == QSE_NULL);
	if (np)
	{
		qse_printf (QSE_T("FAILURE: found something that must not be found - %ld\n"), x);
	}

	qse_printf (QSE_T("total %lu items\n"), (unsigned long)qse_htl_getsize(htl));
	qse_htl_walk (htl, walk1, QSE_NULL);
	qse_htl_close (htl);
	return 0;
}

static int test2 ()
{
	item_t x;
	qse_htl_t* htl;
	qse_htl_node_t* np;

	htl = qse_htl_open (QSE_MMGR_GETDFL(), 0, QSE_SIZEOF(x));
	if (htl == QSE_NULL)
	{
		qse_printf (QSE_T("failed to open a table\n"));
		return -1;
	}

	qse_htl_sethasher (htl, hash_item);
	qse_htl_setcomper (htl, comp_item);
	qse_htl_setcopier (htl, copy_item);
	qse_htl_setfreeer (htl, free_item);

	for (x.a = 9; x.a < 20; x.a++)
	{
		x.x = x.a * 10;
		x.y = x.a * 100;

		if (qse_htl_insert(htl, &x))
		{
			qse_printf (QSE_T("SUCCESS: inserted %ld\n"), x.a);
		}
		else
		{
			qse_printf (QSE_T("FAILURE: failed to insert %ld\n"), x.a);
		}
	}

	x.a = 10;
	if ((np = qse_htl_search(htl, &x)))
	{
		qse_printf (QSE_T("SUCCESS: found %ld\n"), *(long*)np->data);
		QSE_ASSERT (*(long*)np->data == x.a);
	}
	else
	{
		qse_printf (QSE_T("FAILURE: failed to found %ld\n"), x.a);
	}

	x.a = 10;
	if (qse_htl_delete(htl, &x) == 0)
	{
		qse_printf (QSE_T("SUCCESS: deleted %ld\n"), x.a);
	}
	else
	{
		qse_printf (QSE_T("FAILURE: failed to delete %ld\n"), x.a);
	}

	x.a = 10;
	qse_printf (QSE_T("searching for %ld\n"), x.a);
	np = qse_htl_search(htl, &x);
	QSE_ASSERT (np == QSE_NULL);
	if (np)
	{
		qse_printf (QSE_T("FAILURE: found something that must not be found - %ld\n"), x.a);
	}

	qse_printf (QSE_T("total %lu items\n"), (unsigned long)qse_htl_getsize(htl));
	qse_htl_walk (htl, walk2, QSE_NULL);
	qse_htl_close (htl);
	return 0;
}

int main ()
{
	qse_open_stdsios ();
	R (test1);
	R (test2);
	qse_close_stdsios ();
	return 0;
}
