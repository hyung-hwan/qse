#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/arr.h>
#include <qse/si/sio.h>
#include <stdlib.h>


#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

void keeper1 (qse_arr_t* arr, void* dptr, qse_size_t dlen)
{
	qse_printf (QSE_T("[%.*s] has been kept\n"), (int)dlen, dptr);
}

qse_arr_walk_t walker1 (qse_arr_t* arr, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%.*s]\n"), 
		(int)index, (int)QSE_ARR_DLEN(arr,index), QSE_ARR_DPTR(arr,index));
	return QSE_ARR_WALK_FORWARD;
}
qse_arr_walk_t rwalker1 (qse_arr_t* arr, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%.*s]\n"), 
		(int)index, (int)QSE_ARR_DLEN(arr,index), QSE_ARR_DPTR(arr,index));
	return QSE_ARR_WALK_BACKWARD;
}

qse_arr_walk_t walker3 (qse_arr_t* arr, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%d]\n"), 
		(int)index, *(int*)QSE_ARR_DPTR(arr,index));
	return QSE_ARR_WALK_FORWARD;
}

static int test1 ()
{
	qse_arr_t* s1;
	qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!")
	};
	int i;

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setkeeper (s1, keeper1);
	qse_arr_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_arr_insert (s1, 0, x[i], qse_strlen(x[i])) == QSE_ARR_NIL)
		{
			qse_printf (QSE_T("failed to add at 0 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
		else
		{
			qse_printf (QSE_T("add at 0 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
	}

	if (qse_arr_update (s1, 0, QSE_ARR_DPTR(s1,0), QSE_ARR_DLEN(s1,0)) == QSE_ARR_NIL)
	{
		qse_printf (QSE_T("failed to update index 0 with [%.*s]\n"), (int)QSE_ARR_DLEN(s1,0), QSE_ARR_DPTR(s1,0));
	}
	else
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_ARR_DLEN(s1,0), QSE_ARR_DPTR(s1,0));
	}

	if (qse_arr_update (s1, 0, QSE_ARR_DPTR(s1,1), QSE_ARR_DLEN(s1,1)) == QSE_ARR_NIL)
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_ARR_DLEN(s1,1), QSE_ARR_DPTR(s1,1));
	}
	else
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_ARR_DLEN(s1,0), QSE_ARR_DPTR(s1,0));
	}
	
	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_arr_insert (s1, 10, x[i], qse_strlen(x[i])) == QSE_ARR_NIL)
		{
			qse_printf (QSE_T("failed to add at 10 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
		else
		{
			qse_printf (QSE_T("add at 10 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, walker1, QSE_NULL);
	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_rwalk (s1, rwalker1, QSE_NULL);

qse_arr_setcapa (s1, 3);

	qse_arr_close (s1);
	return 0;
}

static int test2 ()
{
	qse_arr_t* s1;
	qse_arr_slot_t* p;
	const qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!"),
		QSE_T("hello hello!"),
		QSE_T("oopsy!")
	};
	const qse_char_t* y[] = 
	{
		QSE_T("tipsy!"),
		QSE_T("oopsy!")
	};
	int i, j;

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (j = 0; j < 20; j++)
	{
		qse_size_t index;
		for (i = 0; i <  QSE_COUNTOF(x); i++)
		{
			if (qse_arr_insert (s1, (i + 1) * j - 1, x[i], qse_strlen(x[i])) == QSE_ARR_NIL)
			{
				
				qse_printf (QSE_T("failed to add at %u => [%.*s]\n"), 
					(unsigned int)((i + 1) * j - 1),
					(int)qse_strlen(x[i]), x[i]);
			}
			else
			{
				qse_printf (QSE_T("add at %u => [%.*s]\n"), 
					(unsigned int)((i + 1) * j - 1),
					(int)qse_strlen(x[i]), x[i]);
			}
		}

		for (i = 0; i < QSE_ARR_SIZE(s1); i++)
		{
			if (QSE_ARR_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_ARR_DLEN(s1,i), QSE_ARR_DPTR(s1,i));
			}
		}


		for (i = 0; i < QSE_COUNTOF(y); i++)
		{
			index = qse_arr_search (s1, 0, y[i], qse_strlen(y[i]));
			if (index == QSE_ARR_NIL)
			{
				qse_printf (QSE_T("failed to find [%s]\n"), y[i]);
			}
			else
			{
				qse_printf (QSE_T("found [%.*s] at %lu\n"), 
					(int)QSE_ARR_DLEN(s1,index), QSE_ARR_DPTR(s1,index), (unsigned long)index);
			}

			index = qse_arr_rsearch (s1, QSE_ARR_SIZE(s1), y[i], qse_strlen(y[i]));
			if (index == QSE_ARR_NIL)
			{
				qse_printf (QSE_T("failed to find [%s]\n"), y[i]);
			}
			else
			{
				qse_printf (QSE_T("found [%.*s] at %lu\n"), 
					(int)QSE_ARR_DLEN(s1,index), QSE_ARR_DPTR(s1,index), (unsigned long)index);
			}
		}


		qse_arr_clear (s1);
		qse_printf (QSE_T("~~~~~~~~\n"));
	}

	qse_arr_close (s1);
	return 0;
}

static int test3 ()
{
	qse_arr_t* s1;
	const qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!")
	};
	const qse_char_t* y = 
		QSE_T("We Accept MasterCard, VISA, JCB, Dinner & eCheck");
	int i, j;

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (j = 0; j < 20; j++)
	{
		for (i = 0; i <  QSE_COUNTOF(x); i++)
		{
			if (qse_arr_insert (s1, (i + 1) * j - 1, x[i], qse_strlen(x[i])) == QSE_ARR_NIL)
			{
				
				qse_printf (QSE_T("failed to add at %u => [%.*s]\n"), 
					(unsigned int)((i + 1) * j - 1),
					(int)qse_strlen(x[i]), x[i]);
			}
			else
			{
				qse_printf (QSE_T("add at %u => [%.*s]\n"), 
					(unsigned int)((i + 1) * j - 1),
					(int)qse_strlen(x[i]), x[i]);
			}
		}

	
		for (i = 2; i < 3; i++)
		{
			if (i < QSE_ARR_SIZE(s1))
			{
				if (QSE_ARR_SLOT(s1,i))
				{
					qse_printf (QSE_T("deleted at %d => [%.*s]\n"), 
						i, (int)QSE_ARR_DLEN(s1,i), QSE_ARR_DPTR(s1,i));
				}

				qse_arr_delete (s1, i, 1);
			}

			if (i < QSE_ARR_SIZE(s1))
			{
				if (qse_arr_update (s1, i, y, qse_strlen(y)) == QSE_ARR_NIL)
				{
					qse_printf (QSE_T("failed to update at %d => [%.*s]\n"), 
						i, (int)qse_strlen(y), y);
				}
				else
				{
					qse_printf (QSE_T("updated at %d => [%.*s]\n"), 
						i, (int)QSE_ARR_DLEN(s1,i), QSE_ARR_DPTR(s1,i));
				}
			}

		}

		qse_printf (QSE_T("array size => %lu\n"), (unsigned long)QSE_ARR_SIZE(s1));

		for (i = 0; i < QSE_ARR_SIZE(s1); i++)
		{
			if (QSE_ARR_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_ARR_DLEN(s1,i), QSE_ARR_DPTR(s1,i));
			}
		}

		{
			qse_size_t count = qse_arr_uplete (s1, 3, 20);
			qse_printf (QSE_T("upleted %lu items from index 3\n"), (unsigned long)count);
		}

		qse_printf (QSE_T("array size => %lu\n"), (unsigned long)QSE_ARR_SIZE(s1));

		for (i = 0; i < QSE_ARR_SIZE(s1); i++)
		{
			if (QSE_ARR_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_ARR_DLEN(s1,i), QSE_ARR_DPTR(s1,i));
			}
		}

		qse_arr_clear (s1);
		qse_printf (QSE_T("~~~~~~~~\n"));
	}

	qse_arr_close (s1);
	return 0;
}

qse_size_t sizer1 (qse_arr_t* arr, qse_size_t hint)
{
	return 2;
}

static int test4 ()
{
	int i;
	qse_arr_t* s1;
	const qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!")
	};

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 3);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open an array\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setsizer (s1, sizer1);
	qse_arr_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_arr_insert (s1, 0, x[i], qse_strlen(x[i])) == QSE_ARR_NIL)
		{
			qse_printf (QSE_T("failed to add at 0 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
		else
		{
			qse_printf (QSE_T("add at 0 => [%.*s]\n"), 
				(int)qse_strlen(x[i]), x[i]);
		}
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, walker1, QSE_NULL);
	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_rwalk (s1, rwalker1, QSE_NULL);

	qse_arr_close (s1);
	return 0;
}


qse_arr_comper_t default_comparator;

static int integer_comparator (qse_arr_t* arr,
        const void* dptr1, size_t dlen1,
        const void* dptr2, size_t dlen2)
{
	return (*(int*)dptr1 > *(int*)dptr2)? 1:
	       (*(int*)dptr1 < *(int*)dptr2)? -1: 0;
}

static int inverse_comparator (qse_arr_t* arr,
        const void* dptr1, size_t dlen1,
        const void* dptr2, size_t dlen2)
{
	return -default_comparator (arr, dptr1, dlen1, dptr2, dlen2);
}

static int test5 ()
{
	qse_arr_t* s1;
	int i, j, oldv, newv;

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 3);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open an array\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setscale (s1, QSE_SIZEOF(i));
	qse_arr_setcomper (s1, integer_comparator);

	/* inverse the comparator to implement min-heap */
	default_comparator = qse_arr_getcomper (s1);
	qse_arr_setcomper (s1, inverse_comparator);

	for (i = 0; i < 2500; i++)
	{
		j = rand () % 1000;
		qse_arr_pushheap (s1, &j, 1);
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, walker3, QSE_NULL);

	oldv = 0;
	while (QSE_ARR_SIZE(s1) > 10 )
	{
		newv = *(int*)QSE_ARR_DPTR(s1,0);
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv >= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	for (i = 0; i < 2500; i++)
	{
		j = rand () % 1000;
		qse_arr_pushheap (s1, &j, 1);
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, walker3, QSE_NULL);

	oldv = 0;
	while (QSE_ARR_SIZE(s1) > 0)
	{
		newv = *(int*)QSE_ARR_DPTR(s1,0);
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv >= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	/* back to max-heap */
	qse_arr_setcomper (s1, default_comparator);
	for (i = 0; i < 2500; i++)
	{
		j = rand () % 1000;
		qse_arr_pushheap (s1, &j, 1);
	}
	j = 88888888;
	qse_arr_updateheap (s1, QSE_ARR_SIZE(s1) / 2, &j, 1);
	j = -123;
	qse_arr_updateheap (s1, QSE_ARR_SIZE(s1) / 2, &j, 1);

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, walker3, QSE_NULL);


	oldv = 99999999;
	while (QSE_ARR_SIZE(s1) > 0)
	{
		newv = *(int*)QSE_ARR_DPTR(s1,0);
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv <= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}


	qse_arr_close (s1);
	return 0;
}

struct test6_data_t
{
	int v;
	qse_size_t pos;	
};
typedef struct test6_data_t test6_data_t;

static int test6_comparator (qse_arr_t* arr,
        const void* dptr1, size_t dlen1,
        const void* dptr2, size_t dlen2)
{
	return ((test6_data_t*)dptr1)->v > ((test6_data_t*)dptr2)->v? 1:
	       ((test6_data_t*)dptr1)->v < ((test6_data_t*)dptr2)->v? -1: 0;
}

qse_arr_walk_t test6_walker (qse_arr_t* arr, qse_size_t index, void* arg)
{
	test6_data_t* x;

	x = QSE_ARR_DPTR(arr,index);
	qse_printf (QSE_T("%d => [%d] pos=%d\n"), (int)index, (int)x->v, (int)x->pos);
	QSE_ASSERT (index == x->pos);
	return QSE_ARR_WALK_FORWARD;
}

static int test6 ()
{
	qse_arr_t* s1;
	int i, oldv, newv;
	test6_data_t j;

	s1 = qse_arr_open (QSE_MMGR_GETDFL(), 0, 3);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open an array\n"));
		return -1;
	}

	qse_arr_setcopier (s1, QSE_ARR_COPIER_INLINE);
	qse_arr_setscale (s1, QSE_SIZEOF(j));
	qse_arr_setcomper (s1, test6_comparator);
	qse_arr_setheapposoffset (s1, QSE_OFFSETOF(test6_data_t, pos));

	for (i = 0; i < 100; i++)
	{
		j.v = rand () % 65535;
		qse_arr_pushheap (s1, &j, 1);
	}

	j.v = 88888888;
	qse_arr_updateheap (s1, QSE_ARR_SIZE(s1) / 2, &j, 1);
	j.v = -123;
	qse_arr_updateheap (s1, QSE_ARR_SIZE(s1) / 2, &j, 1);

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, test6_walker, QSE_NULL);

	oldv = 99999999;
	while (QSE_ARR_SIZE(s1) > 50)
	{
		newv = ((test6_data_t*)QSE_ARR_DPTR(s1,0))->v;
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv <= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, test6_walker, QSE_NULL);

	while (QSE_ARR_SIZE(s1) > 10)
	{
		newv = ((test6_data_t*)QSE_ARR_DPTR(s1,0))->v;
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv <= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, test6_walker, QSE_NULL);

	while (QSE_ARR_SIZE(s1) > 1)
	{
		newv = ((test6_data_t*)QSE_ARR_DPTR(s1,0))->v;
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv <= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	qse_printf (QSE_T("arr size => %lu\n"), QSE_ARR_SIZE(s1));
	qse_arr_walk (s1, test6_walker, QSE_NULL);

	while (QSE_ARR_SIZE(s1) > 0)
	{
		newv = ((test6_data_t*)QSE_ARR_DPTR(s1,0))->v;
		qse_printf (QSE_T("top => %d prevtop => %d\n"), newv, oldv);
		QSE_ASSERT (newv <= oldv);
		qse_arr_popheap (s1);
		oldv = newv;
	}

	qse_arr_close (s1);
	return 0;
}

int main ()
{
	qse_openstdsios ();
	R (test1);
	R (test2);
	R (test3);
	R (test4);
	R (test5);
	R (test6);
	qse_closestdsios ();
	return 0;
}
