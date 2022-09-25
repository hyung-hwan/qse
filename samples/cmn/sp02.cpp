#include <stdio.h>
#include <qse/cmn/SharedPtr.hpp>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/cmn/test.h>

static int test_marker[1000] = { 0, };
static int array_deleted = 0;

class X
{
public:
	X(int y = 0): y(y)
	{
		qse_printf (QSE_T("X(%d) constructured\n"), this->y);
		test_marker[y]++;
	}

	~X()
	{
		qse_printf (QSE_T("X(%d) destructed\n"), this->y);
		test_marker[y]--;
	}

	int y;
};

struct array_deleter: QSE::SharedPtrArrayDeleter<X>
{
	void operator() (X* ptr, void* arg)
	{
		array_deleted++;
		QSE::SharedPtrArrayDeleter<X>::operator() (ptr, arg);
	}
};

struct destroy_x_in_mmgr
{
	void operator() (X* x, void* arg)
	{
		array_deleted++;
		x->~X();
		::operator delete (x, (QSE::Mmgr*)arg);
	}
};

int f2 ()
{
	QSE::HeapMmgr heap_mmgr (30000, QSE::Mmgr::getDFL());
	QSE::HeapMmgr heap_mmgr_2 (30000, QSE::Mmgr::getDFL());
	QSE::SharedPtr<X> y (new X(1));
	QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > k (&heap_mmgr);

	QSE_TESASSERT1 (y->y == 1, QSE_T("unexpected value"));
	QSE_TESASSERT1 (test_marker[y->y] == 1, QSE_T("allocation tally wrong"));

	{
		QSE::SharedPtr<X> x1 (y);
		QSE_TESASSERT1 (x1->y == 1, QSE_T("unexpected value"));
	}
	QSE_TESASSERT1 (test_marker[y->y] == 1, QSE_T("allocation tally wrong"));
	qse_printf (QSE_T("----------------------------\n"));

	{
		//QSE::SharedPtr<X,QSE::SharedPtrArrayDeleter<X> > x3 (new X[10]);
		QSE::SharedPtr<X,array_deleter > x3 (new X[10]);
	}
	QSE_TESASSERT1 (array_deleted == 1, QSE_T("array not deleted"));
	qse_printf (QSE_T("----------------------------\n"));

	{
		//QSE::SharedPtr<X> x2 (new(&heap_mmgr) X, destroy_x);
		QSE_TESASSERT1 (test_marker[2] == 0, QSE_T("allocation tally wrong"));
		QSE::SharedPtr<X,destroy_x_in_mmgr> x2 (&heap_mmgr, new(&heap_mmgr) X(2), &heap_mmgr);
		QSE_TESASSERT1 (x2->y == 2, QSE_T("unexpected value"));
		QSE_TESASSERT1 (test_marker[2] == 1, QSE_T("allocation tally wrong"));
	}
	QSE_TESASSERT1 (test_marker[2] == 0, QSE_T("allocation tally wrong"));
	qse_printf (QSE_T("----------------------------\n"));

	{
		QSE_TESASSERT1 (test_marker[3] == 0, QSE_T("allocation tally wrong"));
		QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > x4 (new(&heap_mmgr_2) X(3), &heap_mmgr_2);
		QSE_TESASSERT1 (test_marker[3] == 1, QSE_T("allocation tally wrong"));

		k = x4;
		QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > x5 (k);
	}
	QSE_TESASSERT1 (test_marker[3] == 1, QSE_T("allocation tally wrong"));
	qse_printf (QSE_T("----------------------------\n"));

	QSE_TESASSERT1 (k->y == 3, QSE_T("unexpected value"));
	return 0;

oops:
	return -1;
}


int main ()
{
	qse_open_stdsios();
	f2 ();
	qse_close_stdsios();
	return 0;
}
