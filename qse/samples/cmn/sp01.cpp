#include <stdio.h>
#include <qse/cmn/ScopedPtr.hpp>
#include <qse/cmn/HeapMmgr.hpp>


class X
{
public:
	X()
	{
		printf ("X constructured\n");
	}

	~X()
	{
		printf ("X destructed\n");
	}
};

struct destroy_x_in_mmgr
{
	void operator() (X* x, void* arg)
	{
		x->~X();	
		::operator delete (x, (QSE::Mmgr*)arg);
	}
};

int main ()
{
        QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);

	{
		QSE::ScopedPtr<X> x1 (new X);
	}
	printf ("----------------------------\n");

	{
		QSE::ScopedPtr<X,QSE::ScopedPtrArrayDeleter<X> > x3 (new X[10]);
	}
	printf ("----------------------------\n");

	{
		//QSE::ScopedPtr<X> x2 (new(&heap_mmgr) X, destroy_x);
		QSE::ScopedPtr<X,destroy_x_in_mmgr> x2 (new(&heap_mmgr) X, &heap_mmgr);
	}
	printf ("----------------------------\n");
	{
		QSE::ScopedPtr<X,QSE::ScopedPtrMmgrDeleter<X> > x4 (new(&heap_mmgr) X, &heap_mmgr);
	}
	printf ("----------------------------\n");

	return 0;
}
