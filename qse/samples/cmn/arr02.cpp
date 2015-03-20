#include <stdio.h>

#include <string.h>
#include <qse/cmn/Array.hpp>
#include <qse/cmn/String.hpp>

class Julia
{
public:
	Julia (int q = 0): x(QSE_NULL)
	{ 
		this->x = new int (q); 
printf ("constructor %d\n", q);
	}

	Julia (const Julia& q): x(QSE_NULL)
	{
		this->x = new int (*q.x);
printf ("copy constructor %d\n", *q.x);
	}

#if (__cplusplus >= 201103L) // C++11 
	Julia (Julia&& q)
	{
printf ("move constructor %d\n", *q.x);
		this->x = q.x;
		q.x = QSE_NULL;

	}
#endif

	~Julia()
	{
		if (this->x) 
		{
printf ("deleting %p %d\n", this, *x);
			delete this->x;
		}
	}

	Julia& operator= (const Julia& q)
	{
		if (this != &q)
		{
			if (this->x) { delete this->x; this->x = QSE_NULL; }
			this->x = new int (*q.x);
printf ("operator= %d\n", *q.x);
		}

		return *this;
	}

#if (__cplusplus >= 201103L) // C++11 
	Julia& operator= (Julia&& q)
	{
		if (this != &q)
		{
			if (this->x) { delete this->x; this->x = QSE_NULL; }
printf ("move operator= %d\n", *q.x);
			this->x = q.x;
			q.x = QSE_NULL;
		}

		return *this;
	}
#endif

	int* x;
};

int main ()
{
	QSE::Array<Julia> a0;

	for (int i = 0; i < 256; i++)
	{
		a0.insert (0, Julia(i));
	}

	a0.setCapacity (1024);
	a0.insert (512, Julia (512));

	a0.remove (2, 5);

	a0.update (5, Julia(9999));
#if (__cplusplus >= 201103L) // C++11 
	QSE::Array<Julia> a1 ((QSE::Array<Julia>&&)a0);
#else
	QSE::Array<Julia> a1 (a0);
#endif
	printf ("OK: %d %d\n", (int)a0.getSize(), (int)a1.getSize());
	return 0;
}
