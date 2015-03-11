#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/sio.h>
#include <stdlib.h>


#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

void keeper1 (qse_lda_t* lda, void* dptr, qse_size_t dlen)
{
	qse_printf (QSE_T("[%.*s] has been kept\n"), (int)dlen, dptr);
}

qse_lda_walk_t walker1 (qse_lda_t* lda, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%.*s]\n"), 
		index, (int)QSE_LDA_DLEN(lda,index), QSE_LDA_DPTR(lda,index));
	return QSE_LDA_WALK_FORWARD;
}
qse_lda_walk_t rwalker1 (qse_lda_t* lda, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%.*s]\n"), 
		index, (int)QSE_LDA_DLEN(lda,index), QSE_LDA_DPTR(lda,index));
	return QSE_LDA_WALK_BACKWARD;
}

qse_lda_walk_t walker3 (qse_lda_t* lda, qse_size_t index, void* arg)
{
	qse_printf (QSE_T("%d => [%d]\n"), 
		index, *(int*)QSE_LDA_DPTR(lda,index));
	return QSE_LDA_WALK_FORWARD;
}

static int test1 ()
{
	qse_lda_t* s1;
	qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!")
	};
	int i;

	s1 = qse_lda_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_lda_setcopier (s1, QSE_LDA_COPIER_INLINE);
	qse_lda_setkeeper (s1, keeper1);
	qse_lda_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_lda_insert (s1, 0, x[i], qse_strlen(x[i])) == QSE_LDA_NIL)
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

	if (qse_lda_update (s1, 0, QSE_LDA_DPTR(s1,0), QSE_LDA_DLEN(s1,0)) == QSE_LDA_NIL)
	{
		qse_printf (QSE_T("failed to update index 0 with [%.*s]\n"), (int)QSE_LDA_DLEN(s1,0), QSE_LDA_DPTR(s1,0));
	}
	else
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_LDA_DLEN(s1,0), QSE_LDA_DPTR(s1,0));
	}

	if (qse_lda_update (s1, 0, QSE_LDA_DPTR(s1,1), QSE_LDA_DLEN(s1,1)) == QSE_LDA_NIL)
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_LDA_DLEN(s1,1), QSE_LDA_DPTR(s1,1));
	}
	else
	{
		qse_printf (QSE_T("updated index 0 with [%.*s]\n"), (int)QSE_LDA_DLEN(s1,0), QSE_LDA_DPTR(s1,0));
	}
	
	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_lda_insert (s1, 10, x[i], qse_strlen(x[i])) == QSE_LDA_NIL)
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

	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_walk (s1, walker1, QSE_NULL);
	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_rwalk (s1, rwalker1, QSE_NULL);

qse_lda_setcapa (s1, 3);

	qse_lda_close (s1);
	return 0;
}

static int test2 ()
{
	qse_lda_t* s1;
	qse_lda_slot_t* p;
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

	s1 = qse_lda_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_lda_setcopier (s1, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (j = 0; j < 20; j++)
	{
		qse_size_t index;
		for (i = 0; i <  QSE_COUNTOF(x); i++)
		{
			if (qse_lda_insert (s1, (i + 1) * j - 1, x[i], qse_strlen(x[i])) == QSE_LDA_NIL)
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

		for (i = 0; i < QSE_LDA_SIZE(s1); i++)
		{
			if (QSE_LDA_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_LDA_DLEN(s1,i), QSE_LDA_DPTR(s1,i));
			}
		}


		for (i = 0; i < QSE_COUNTOF(y); i++)
		{
			index = qse_lda_search (s1, 0, y[i], qse_strlen(y[i]));
			if (index == QSE_LDA_NIL)
			{
				qse_printf (QSE_T("failed to find [%s]\n"), y[i]);
			}
			else
			{
				qse_printf (QSE_T("found [%.*s] at %lu\n"), 
					(int)QSE_LDA_DLEN(s1,index), QSE_LDA_DPTR(s1,index), (unsigned long)index);
			}

			index = qse_lda_rsearch (s1, QSE_LDA_SIZE(s1), y[i], qse_strlen(y[i]));
			if (index == QSE_LDA_NIL)
			{
				qse_printf (QSE_T("failed to find [%s]\n"), y[i]);
			}
			else
			{
				qse_printf (QSE_T("found [%.*s] at %lu\n"), 
					(int)QSE_LDA_DLEN(s1,index), QSE_LDA_DPTR(s1,index), (unsigned long)index);
			}
		}


		qse_lda_clear (s1);
		qse_printf (QSE_T("~~~~~~~~\n"));
	}

	qse_lda_close (s1);
	return 0;
}

static int test3 ()
{
	qse_lda_t* s1;
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

	s1 = qse_lda_open (QSE_MMGR_GETDFL(), 0, 0);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_lda_setcopier (s1, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (j = 0; j < 20; j++)
	{
		for (i = 0; i <  QSE_COUNTOF(x); i++)
		{
			if (qse_lda_insert (s1, (i + 1) * j - 1, x[i], qse_strlen(x[i])) == QSE_LDA_NIL)
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
			if (i < QSE_LDA_SIZE(s1))
			{
				if (QSE_LDA_SLOT(s1,i))
				{
					qse_printf (QSE_T("deleted at %d => [%.*s]\n"), 
						i, (int)QSE_LDA_DLEN(s1,i), QSE_LDA_DPTR(s1,i));
				}

				qse_lda_delete (s1, i, 1);
			}

			if (i < QSE_LDA_SIZE(s1))
			{
				if (qse_lda_update (s1, i, y, qse_strlen(y)) == QSE_LDA_NIL)
				{
					qse_printf (QSE_T("failed to update at %d => [%.*s]\n"), 
						i, (int)qse_strlen(y), y);
				}
				else
				{
					qse_printf (QSE_T("updated at %d => [%.*s]\n"), 
						i, (int)QSE_LDA_DLEN(s1,i), QSE_LDA_DPTR(s1,i));
				}
			}

		}

		qse_printf (QSE_T("array size => %lu\n"), (unsigned long)QSE_LDA_SIZE(s1));

		for (i = 0; i < QSE_LDA_SIZE(s1); i++)
		{
			if (QSE_LDA_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_LDA_DLEN(s1,i), QSE_LDA_DPTR(s1,i));
			}
		}

		{
			qse_size_t count = qse_lda_uplete (s1, 3, 20);
			qse_printf (QSE_T("upleted %lu items from index 3\n"), (unsigned long)count);
		}

		qse_printf (QSE_T("array size => %lu\n"), (unsigned long)QSE_LDA_SIZE(s1));

		for (i = 0; i < QSE_LDA_SIZE(s1); i++)
		{
			if (QSE_LDA_SLOT(s1,i))
			{
				qse_printf (QSE_T("[%d] %d => [%.*s]\n"), 
					j, i, (int)QSE_LDA_DLEN(s1,i), QSE_LDA_DPTR(s1,i));
			}
		}

		qse_lda_clear (s1);
		qse_printf (QSE_T("~~~~~~~~\n"));
	}

	qse_lda_close (s1);
	return 0;
}

qse_size_t sizer1 (qse_lda_t* lda, qse_size_t hint)
{
	return 2;
}

static int test4 ()
{
	int i;
	qse_lda_t* s1;
	const qse_char_t* x[] =
	{
		QSE_T("this is so good"),
		QSE_T("what the fuck"),
		QSE_T("do you like it?"),
		QSE_T("oopsy!")
	};

	s1 = qse_lda_open (QSE_MMGR_GETDFL(), 0, 3);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open an array\n"));
		return -1;
	}

	qse_lda_setcopier (s1, QSE_LDA_COPIER_INLINE);
	qse_lda_setsizer (s1, sizer1);
	qse_lda_setscale (s1, QSE_SIZEOF(qse_char_t));

	for (i = 0; i <  QSE_COUNTOF(x); i++)
	{
		if (qse_lda_insert (s1, 0, x[i], qse_strlen(x[i])) == QSE_LDA_NIL)
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

	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_walk (s1, walker1, QSE_NULL);
	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_rwalk (s1, rwalker1, QSE_NULL);

	qse_lda_close (s1);
	return 0;
}


qse_lda_comper_t default_comparator;

static int inverse_comparator (qse_lda_t* lda,
        const void* dptr1, size_t dlen1,
        const void* dptr2, size_t dlen2)
{
	return -default_comparator (lda, dptr1, dlen1, dptr2, dlen2);
}

static int test5 ()
{
	qse_lda_t* s1;
	int i, j;

	s1 = qse_lda_open (QSE_MMGR_GETDFL(), 0, 3);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open an array\n"));
		return -1;
	}

	qse_lda_setcopier (s1, QSE_LDA_COPIER_INLINE);
	qse_lda_setscale (s1, QSE_SIZEOF(i));

	/* inverse the comparator to implement min-heap */
	default_comparator = qse_lda_getcomper (s1);
	qse_lda_setcomper (s1, inverse_comparator);

	for (i = 0; i < 25; i++)
	{
		j = rand () % 100;
		qse_lda_pushheap (s1, &j, 1);
	}

	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_walk (s1, walker3, QSE_NULL);

	while (QSE_LDA_SIZE(s1) > 10 )
	{
		qse_printf (QSE_T("top => %d\n"), *(int*)QSE_LDA_DPTR(s1,0));
		qse_lda_popheap (s1);
	}

	for (i = 0; i < 25; i++)
	{
		j = rand () % 100;
		qse_lda_pushheap (s1, &j, 1);
	}

	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_walk (s1, walker3, QSE_NULL);

	while (QSE_LDA_SIZE(s1))
	{
		qse_printf (QSE_T("top => %d\n"), *(int*)QSE_LDA_DPTR(s1,0));
		qse_lda_popheap (s1);
	}

	qse_lda_setcomper (s1, default_comparator);
	for (i = 0; i < 25; i++)
	{
		j = rand () % 100;
		qse_lda_pushheap (s1, &j, 1);
	}

	qse_printf (QSE_T("lda size => %lu\n"), QSE_LDA_SIZE(s1));
	qse_lda_walk (s1, walker3, QSE_NULL);

	while (QSE_LDA_SIZE(s1))
	{
		qse_printf (QSE_T("top => %d\n"), *(int*)QSE_LDA_DPTR(s1,0));
		qse_lda_popheap (s1);
	}


	qse_lda_close (s1);
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
	qse_closestdsios ();
	return 0;
}
