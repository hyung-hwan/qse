#include <stdio.h>
#include <qse/cmn/SharedPtr.hpp>
#include <qse/cmn/HeapMmgr.hpp>

class X
{
public:
	X(int y = 0): y(y)
	{
		printf ("X(%d) constructured\n", this->y);
	}

	~X()
	{
		printf ("X(%d) destructed\n", this->y);
	}

	int y;
};

struct destroy_x_in_mmgr
{
	void operator() (X* x, void* arg)
	{
		x->~X();	
		::operator delete (x, (QSE::Mmgr*)arg);
	}
};

void f2 ()
{
        QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);
        QSE::HeapMmgr heap_mmgr_2 (QSE::Mmgr::getDFL(), 30000);
	QSE::SharedPtr<X> y (new X(1));
	QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > k (&heap_mmgr);

	{
		QSE::SharedPtr<X> x1 (y);
	}
	printf ("----------------------------\n");

	{
		QSE::SharedPtr<X,QSE::SharedPtrArrayDeleter<X> > x3 (new X[10]);
	}
	printf ("----------------------------\n");

	{
		//QSE::SharedPtr<X> x2 (new(&heap_mmgr) X, destroy_x);
		QSE::SharedPtr<X,destroy_x_in_mmgr> x2 (&heap_mmgr, new(&heap_mmgr) X(2), &heap_mmgr);
	}
	printf ("----------------------------\n");

	{
		QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > x4 (new(&heap_mmgr_2) X(3), &heap_mmgr_2);

		k = x4;
		QSE::SharedPtr<X,QSE::SharedPtrMmgrDeleter<X> > x5 (k);
	}
	printf ("----------------------------\n");

	QSE_ASSERT (k->y == 3);
}


int main ()
{
	f2 ();
	return 0;
}
