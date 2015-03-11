#include <stdio.h>
#include <qse/cmn/StdMmgr.hpp>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/cmn/LinkedList.hpp>
#include <qse/cmn/HashList.hpp>
#include <qse/cmn/sio.h>


class T
{
public:
	T(int x = 0): x(x) {
//		printf ("constructor\n");
		y = x + 99;
	}
	~T() {
//		printf ("destructor\n");
	}
	bool operator== (const T& x) const { return this->x == x.x; }
	qse_size_t getHashCode() const { return x; }

	int getValue() const { return this->x; }
	int getY() const { return this->y; }
protected:
	int x;
	int y;
};

void* operator new (size_t x, int q)
{
	return ::operator new (x);
}


struct IntHasher 
{
	qse_size_t operator() (int v) const { return v; }
};

struct IntIsEqual
{
	qse_size_t operator() (int v1, const T& v2) const { return v1 == v2.getValue(); }
};

typedef QSE::HashList<int,IntHasher> IntList;

int main ()
{
	qse_openstdsios ();

	T* x;
	//QSE::StdMmgr* mmgr = QSE::StdMmgr::getDFL();
	//QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 1000000);
	//QSE::Mmgr* mmgr = &heap_mmgr;
	QSE::Mmgr* mmgr = QSE_NULL;

/*
	x = new(mmgr) T; //[10];

printf ("x====> %p\n", x);
	x->~T();
	//for (int i = 0; i < 10; i++) x[i].~T();
	//::operator delete[] (x, mmgr);
	::operator delete (x, mmgr);
	//delete[] x;

printf ("----------------------\n");
	T* y = new(10) T;
	y->~T();
	::operator delete(y);
printf ("----------------------\n");
*/
try
{

	T t1,t2,t3;

#if 0
printf ("----------------------\n");
	{
	QSE::LinkedList<T> l (mmgr, 100);
printf ("----------------------\n");
	l.append (t1);
printf ("----------------------\n");
	l.append (t2);
	l.append (t3);
printf ("================\n");

	QSE::LinkedList<T> l2 (mmgr, 100);

	l2 = l;
	}
printf ("----------------------\n");
#endif

	//QSE::HashList<T> h (mmgr, 1000, 75, 1000);
	QSE::HashList<T> h (mmgr, 1000, 75, 500);
for (int i = 0; i < 1000; i++)
{
	T x(i);
	h.insert (x);
}
printf ("h.getSize() => %d\n", (int)h.getSize());

	QSE::HashList<T> h2 (mmgr, 1000, 75, 700);
	h2 = h;

	for (QSE::HashList<T>::Iterator it = h2.getIterator(); it.isLegit(); it++)
	{
		printf ("%d\n", (*it).getValue());
	}
printf ("----------------------\n");


	printf ("%p\n", h2.getHeadNode());
printf ("----------------------\n");

	IntList hl;
	IntList::Iterator it;
	hl.insert (10);
	hl.insert (150);
	hl.insert (200);
	for (it = hl.getIterator(); it.isLegit(); it++)
	{
		printf ("%d\n", *it);
	}


printf ("----------------------\n");

	{
		const QSE::HashList<T>& h3 = h2;
		const T* tt = h3.heterofindValue<int,IntHasher,IntIsEqual> (100);
		if (tt) printf ("%d:%d\n", tt->getValue(), tt->getY());
		else printf ("not found...\n");
	}
printf ("----------------------\n");
}
catch (QSE::Exception& e)
{
	qse_printf (QSE_T("Exception: %s\n"), QSE_EXCEPTION_NAME(e));
}


	qse_closestdsios ();
	return 0;	
}

