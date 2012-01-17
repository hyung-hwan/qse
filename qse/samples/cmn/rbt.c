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

static qse_rbt_walk_t walk2 (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_printf (QSE_T("key = %.*s, value = %.*s\n"), 
		(int)QSE_RBT_KLEN(pair), 
		QSE_RBT_KPTR(pair), 
		(int)QSE_RBT_VLEN(pair),
		QSE_RBT_VPTR(pair));

	return QSE_RBT_WALK_FORWARD;
}

static int test1 ()
{
	qse_rbt_t* s1;
	int i;

	s1 = qse_rbt_open (QSE_MMGR_GETDFL(), 0, 1, 1);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a table\n"));
		return -1;
	}
	qse_rbt_setmancbs (s1, qse_getrbtmancbs(QSE_RBT_MANCBS_INLINE_COPIERS));

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

static int test2 ()
{
	qse_rbt_t* s1;

	s1 = qse_rbt_open (QSE_MMGR_GETDFL(), 0, QSE_SIZEOF(qse_char_t), QSE_SIZEOF(qse_char_t));
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a table\n"));
		return -1;
	}
	qse_rbt_setmancbs (s1, qse_getrbtmancbs(QSE_RBT_MANCBS_INLINE_COPIERS));

	qse_rbt_insert (s1, QSE_T("hello"), 5, QSE_T("mr. monkey"), 10);
	qse_rbt_insert (s1, QSE_T("world"), 5, QSE_T("ms. panda"), 9);
	qse_rbt_insert (s1, QSE_T("thinkpad"), 8, QSE_T("x61"), 3);
	qse_rbt_rwalk (s1, walk2, QSE_NULL);

	qse_printf (QSE_T("-------------------\n"));
	qse_rbt_upsert (s1, QSE_T("hello"), 5, QSE_T("dr. gorilla"), 11);
	qse_rbt_upsert (s1, QSE_T("thinkpad"), 8, QSE_T("x61 rocks on"), 13);
	qse_rbt_ensert (s1, QSE_T("vista"), 5, QSE_T("microsoft"), 9);
	qse_rbt_update (s1, QSE_T("world"), 5, QSE_T("baseball classic"), 16);
	qse_rbt_rwalk (s1, walk2, QSE_NULL);

	qse_rbt_close (s1);
	return 0;
}

qse_rbt_walk_t print_map_pair (qse_rbt_t* map, qse_rbt_pair_t* pair, void* arg)
{
	qse_printf (QSE_T("%.*s[%d] => %.*s[%d]\n"), 
		(int)QSE_RBT_KLEN(pair), QSE_RBT_KPTR(pair), (int)QSE_RBT_KLEN(pair),
		(int)QSE_RBT_VLEN(pair), QSE_RBT_VPTR(pair), (int)QSE_RBT_VLEN(pair));

	return QSE_RBT_WALK_FORWARD;
}

static qse_rbt_pair_t* test5_cbserter (
	qse_rbt_t* rbt, qse_rbt_pair_t* pair, 
	void* kptr, qse_size_t klen, void* ctx)
{
	qse_xstr_t* v = (qse_xstr_t*)ctx;
	if (pair == QSE_NULL)
	{
		/* no existing key for the key */
		return qse_rbt_allocpair (rbt, kptr, klen, v->ptr, v->len);
	}
	else
	{
		/* a pair with the key exists. 
		 * in this sample, i will append the new value to the old value 
		 * separated by a comma */

		qse_rbt_pair_t* new_pair;
		qse_char_t comma = QSE_T(',');
		qse_byte_t* vptr;

		/* allocate a new pair, but without filling the actual value. 
		 * note vptr is given QSE_NULL for that purpose */
		new_pair = qse_rbt_allocpair (
			rbt, kptr, klen, QSE_NULL, 
			QSE_RBT_VLEN(pair) + 1 + v->len);
		if (new_pair == QSE_NULL) return QSE_NULL;

		/* fill in the value space */
		vptr = QSE_RBT_VPTR(new_pair);
		qse_memcpy (vptr, QSE_RBT_VPTR(pair), 
			QSE_RBT_VLEN(pair) * QSE_SIZEOF(qse_char_t));
		vptr += QSE_RBT_VLEN(pair) * QSE_SIZEOF(qse_char_t);
		qse_memcpy (vptr, &comma, QSE_SIZEOF(qse_char_t));
		vptr += QSE_SIZEOF(qse_char_t);
		qse_memcpy (vptr, v->ptr, v->len*QSE_SIZEOF(qse_char_t));

		/* this callback requires the old pair to be destroyed */
		qse_rbt_freepair (rbt, pair);

		/* return the new pair */
		return new_pair;
	}
}

static int test5 ()
{
	qse_rbt_t* s1;
	int i;

	qse_char_t* keys[] = 
	{
		QSE_T("one"), QSE_T("two"), QSE_T("three")
	};
	qse_char_t* vals[] = 
	{
		QSE_T("1"), QSE_T("2"), QSE_T("3"), QSE_T("4"), QSE_T("5"),
	};

	s1 = qse_rbt_open (QSE_MMGR_GETDFL(), 0, 
		QSE_SIZEOF(qse_char_t), QSE_SIZEOF(qse_char_t));
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a hash table\n"));
		return -1;
	}
	qse_rbt_setmancbs (s1, qse_getrbtmancbs(QSE_RBT_MANCBS_INLINE_COPIERS));

	for (i = 0; i < QSE_COUNTOF(vals); i++)
	{
		qse_xstr_t ctx;

		qse_printf (QSE_T("setting a key [%s] and a value [%s]: "), keys[i%QSE_COUNTOF(keys)], vals[i]);

		ctx.ptr = vals[i];
		ctx.len = qse_strlen(vals[i]);
		if (qse_rbt_cbsert (s1, 
			keys[i%QSE_COUNTOF(keys)], 
			qse_strlen(keys[i%QSE_COUNTOF(keys)]), 
			test5_cbserter, &ctx) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
		}
		else
		{	
			qse_printf (QSE_T("[OK]\n"));
		}
	}
	qse_rbt_walk (s1, print_map_pair, QSE_NULL);
		

	qse_rbt_close (s1);
	return 0;
}

int main ()
{
	R (test1);
	R (test2);
	R (test5);
	return 0;
}
