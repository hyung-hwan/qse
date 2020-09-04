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
	struct julia_t
	{
		julia_t(int v): v(v), heap_pos(~(qse_size_t)0) {}

		int v;
		qse_size_t heap_pos;
	};

	Julia (int q = 0): x(QSE_NULL)
	{ 
		this->x = new julia_t (q); 
	}

	Julia (const Julia& q): x(QSE_NULL)
	{
		this->x = new julia_t (*q.x);
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
		if (this->x) delete this->x;
	}

	Julia& operator= (const Julia& q)
	{
		if (this != &q)
		{
			if (this->x) { delete this->x; this->x = QSE_NULL; }
			this->x = new julia_t (*q.x);
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
	bool operator> (const Julia& q) const { return this->x->v > q.x->v; }
#else
	bool operator> (const Julia& q) const { return this->x->v < q.x->v; }
#endif
	julia_t* x;
};

struct JuliaGreaterThan
{
	bool operator() (const Julia& b1, const Julia& b2) const
	{
		return b1 > b2;
	}
};

struct JuliaOnAssign
{
	void operator() (Julia& b, qse_size_t index) const
	{
		b.x->heap_pos = index;
		//printf ("%p[%d] to position %zd\n", &b, b.x->v, index);
	}
};

typedef QSE::BinaryHeap<Julia,JuliaGreaterThan,JuliaOnAssign> JuliaHeap;

int main ()
{
	{
		JuliaHeap jh;
		jh.insert (Julia(10));
		jh.insert (Julia(20));
		jh.insert (Julia(30));
		jh.insert (Julia(40));
		jh.insert (Julia(50));
		jh.remove (1);

		// test if the positioner(JuliaOnAssign) is called properly.
		QSE_ASSERT (jh.getValueAt(0).x->heap_pos == 0);
		QSE_ASSERT (jh.getValueAt(1).x->heap_pos == 1);
		QSE_ASSERT (jh.getValueAt(2).x->heap_pos == 2);
		QSE_ASSERT (jh.getValueAt(3).x->heap_pos == 3);

		printf ("%zd\n", jh.getValueAt(0).x->heap_pos);
		printf ("%zd\n", jh.getValueAt(1).x->heap_pos);
		printf ("%zd\n", jh.getValueAt(2).x->heap_pos);
		printf ("%zd\n", jh.getValueAt(3).x->heap_pos);
	}

	return 0;
}
