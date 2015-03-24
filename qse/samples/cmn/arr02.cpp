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


#if defined(QSE_CPP_ENABLE_CPP1_MOVE) 
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

#if defined(QSE_CPP_ENABLE_CPP1_MOVE) 
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

typedef QSE::Array<Julia> JuliaArray;

int main ()
{
	JuliaArray a0;

	for (int i = 0; i < 256; i++)
	{
		a0.insert (0, Julia(i));
	}

	a0.setCapacity (1024);
	a0.insert (512, Julia (512));

	a0.remove (2, 5);

	a0.update (5, Julia(9999));
#if defined(QSE_CPP_ENABLE_CPP1_MOVE) 
	JuliaArray a1 ((JuliaArray&&)a0);
#else
	JuliaArray a1 (a0);
#endif
	printf ("OK: %d %d\n", (int)a0.getSize(), (int)a1.getSize());

	
	for (qse_size_t i = 0; i < a1.getSize(); i++)
	{
		printf ("ITEM: %d => %d\n", (int)i, *a1[i].x);
	}
	printf ("----------------\n");
	a1.rotate (JuliaArray::ROTATE_LEFT, 2);
	for (qse_size_t i = 0; i < a1.getSize(); i++)
	{
		printf ("ITEM: %d => %d\n", (int)i, *a1[i].x);
	}
	return 0;
}
