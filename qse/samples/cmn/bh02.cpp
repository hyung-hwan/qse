#include <stdio.h>

#include <string.h>
#include <qse/cmn/BinaryHeap.hpp>
#include <qse/cmn/String.hpp>
#include <qse/cmn/alg.h>
#include <qse/cmn/time.h>

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

#if defined(QSE_ENABLE_CPP11_MOVE) 
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

#if defined(QSE_ENABLE_CPP11_MOVE) 
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

	bool operator> (const Julia& q) const { return *this->x > *q.x; }
	int* x;
};

typedef QSE::BinaryHeap<Julia> JuliaHeap;

int main ()
{
	JuliaHeap jh;
	qse_uint32_t x;
	qse_ntime_t nt;

	qse_gettime (&nt);

	x = nt.sec + nt.nsec;

	for (int i = 0; i < 100; i++)
	{
		x = qse_rand31(x);
		jh.insert (Julia((int)x % 100));
	}

	while (!jh.isEmpty())
	{
		printf ("%d\n", *jh.getValueAt(0).x);
		jh.remove (0);
	}
}
