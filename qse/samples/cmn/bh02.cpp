#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <qse/cmn/BinaryHeap.hpp>
#include <qse/cmn/String.hpp>
#include <qse/cmn/alg.h>
#include <qse/cmn/time.h>


//#define MAX_HEAP

class Julia
{
public:
	Julia (int q = 0): x(QSE_NULL)
	{ 
		this->x = new int (q); 
	}

	Julia (const Julia& q): x(QSE_NULL)
	{
		this->x = new int (*q.x);
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE) 
	Julia (Julia&& q)
	{
		this->x = q.x;
		q.x = QSE_NULL;
	}
#endif

	~Julia()
	{
		if (this->x) 
		{
			delete this->x;
		}
	}

	Julia& operator= (const Julia& q)
	{
		if (this != &q)
		{
			if (this->x) { delete this->x; this->x = QSE_NULL; }
			this->x = new int (*q.x);
		}

		return *this;
	}

#if defined(QSE_CPP_ENABLE_CPP11_MOVE) 
	Julia& operator= (Julia&& q)
	{
		if (this != &q)
		{
			if (this->x) { delete this->x; this->x = QSE_NULL; }
			this->x = q.x;
			q.x = QSE_NULL;
		}

		return *this;
	}
#endif

#if defined(MAX_HEAP)
	bool operator> (const Julia& q) const { return *this->x > *q.x; }
#else
	bool operator> (const Julia& q) const { return *this->x < *q.x; }
#endif
	int* x;
};

typedef QSE::BinaryHeap<Julia> JuliaHeap;

int main ()
{
	JuliaHeap jh;
	qse_uint32_t x;
	qse_ntime_t nt;
	int oldval, newval;

	qse_gettime (&nt);

	x = nt.sec + nt.nsec;

	for (int i = 0; i < 2500; i++)
	{
		//x = qse_rand31(x);
		x = rand();
		jh.insert (Julia((int)x % 1000));
	}

#if defined(MAX_HEAP)
	oldval = 9999999;
#else
	oldval = 0;
#endif
	while (!jh.isEmpty())
	{
		newval = *jh.getValueAt(0).x;
		printf ("%d    oldval => %d\n", newval, oldval);
#if defined(MAX_HEAP)
		QSE_ASSERT (newval <= oldval);	
#else
		QSE_ASSERT (newval >= oldval);	
#endif
		jh.remove (0);
		oldval = newval;
	}

	return 0;
}
