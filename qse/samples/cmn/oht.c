#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/oht.h>
#include <qse/cmn/stdio.h>


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

static qse_oht_walk_t walk1 (qse_oht_t* oht, void* data, void* ctx)
{
	qse_printf (QSE_T("[%ld]\n"), *(long*)data);
 	return QSE_OHT_WALK_FORWARD;
}

static qse_oht_walk_t walk2 (qse_oht_t* oht, void* data, void* ctx)
{
	item_t* item = (item_t*)data;
	qse_printf (QSE_T("a [%ld] x [%ld] y [%ld]\n"), item->a, item->x, item->y);
 	return QSE_OHT_WALK_FORWARD;
}

static qse_size_t hash (qse_oht_t* oht, const void* data)
{
	item_t* item = (item_t*)data;
	return item->a	+ 123445;
}

static int comp (qse_oht_t* oht, const void* data1, const void* data2)
{
	return ((item_t*)data1)->a != ((item_t*)data2)->a;	
}

static int test1 ()
{
	long x;
	qse_oht_t* oht;

	oht = qse_oht_open (QSE_MMGR_GETDFL(), 0, QSE_SIZEOF(x), 10, 5);
	if (oht == QSE_NULL)
	{
		qse_printf (QSE_T("failed to open a table\n"));
		return -1;
	}

	for (x = 9; x < 20; x++)
	{
		qse_printf (QSE_T("inserting %ld => %lu\n"), 
				x, (unsigned long)qse_oht_insert (oht, &x));
	}

	x = 10;
	qse_printf (QSE_T("searching for %ld => %lu\n"),
			x, (unsigned long)qse_oht_search (oht, &x));

	x = 10;
	qse_printf (QSE_T("deleting %ld => %lu\n"),
			x, (unsigned long)qse_oht_delete (oht, &x));

	x = 10;
	qse_printf (QSE_T("searching for %ld => %lu\n"),
			x, (unsigned long)qse_oht_search (oht, &x));


	qse_printf (QSE_T("total %lu items\n"), (unsigned long)QSE_OHT_SIZE(oht));
	qse_oht_walk (oht, walk1, QSE_NULL);
	qse_oht_close (oht);
	return 0;
}

static int test2 ()
{
	item_t x;
	qse_oht_t* oht;

	oht = qse_oht_open (QSE_MMGR_GETDFL(), 0, QSE_SIZEOF(x), 10, 10);
	if (oht == QSE_NULL)
	{
		qse_printf (QSE_T("failed to open a table\n"));
		return -1;
	}

	qse_oht_sethasher (oht, hash);
	qse_oht_setcomper (oht, comp);

	for (x.a = 9; x.a < 20; x.a++)
	{
		x.x = x.a * 10;
		x.y = x.a * 100;
		qse_printf (QSE_T("inserting %ld => %lu\n"), 
				x.a, (unsigned long)qse_oht_insert (oht, &x));
	}

	x.a = 10;
	qse_printf (QSE_T("searching for %ld => %lu\n"),
			x.a, (unsigned long)qse_oht_search (oht, &x));

	x.a = 10;
	qse_printf (QSE_T("deleting %ld => %lu\n"),
			x.a, (unsigned long)qse_oht_delete (oht, &x));

	x.a = 10;
	qse_printf (QSE_T("searching for %ld => %lu\n"),
			x.a, (unsigned long)qse_oht_search (oht, &x));


	qse_printf (QSE_T("total %lu items\n"), (unsigned long)QSE_OHT_SIZE(oht));
	qse_oht_walk (oht, walk2, QSE_NULL);
	qse_oht_close (oht);
	return 0;
}

int main ()
{
	R (test1);
	R (test2);
	return 0;
}
