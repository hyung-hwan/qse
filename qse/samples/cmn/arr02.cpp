#include <stdio.h>

#include <string.h>
#include <qse/cmn/Array.hpp>
#include <qse/cmn/String.hpp>

/*
typedef  QSE::Array<int,int> IntArray;

struct IntClass
{
	IntClass (int x = 0): x (x) {}
	int x;
};

struct IntClassComparator
{
	int operator() (const IntClass& v, int y) const
	{
		IntTable::DefaultComparator comp;
		return comp (v.x, y);
	}
};

*/

#if 1
class PosStr: public QSE::MbString
{
public:
	PosStr (const char* str = ""): QSE::MbString(str), pos((qse_size_t)-1) {};
	
	PosStr (const PosStr& ps): QSE::MbString (ps), pos(ps.pos)  
	{
	}

	~PosStr()
	{
	}


	qse_size_t pos;
};
#else

class PosStr
{
public:
	PosStr (const char* str = "")
	{
		strcpy (buf, str);
	}

	const char* getBuffer() const { return this->buf; }
	const char* data() const { return this->buf; }

	char buf[512];
};
#endif


struct cstr_comparator
{
	int operator() (const char* v1, const QSE::MbString& v2) const
	{
		return strcmp (v1, v2.getBuffer());
	}
};


typedef  QSE::Array<PosStr> StrArray;


int main ()
{
	//StrArray h (QSE_NULL, 100);
	StrArray h (QSE_NULL, 15);
	char buf[20];

	for (int i = 0; i < 20; i++)
	{
		sprintf (buf, "hello %d", i);
		PosStr x (buf);
		h.insert (0, x);
	}

#if 0
	printf ("%s\n", h.get(3).getBuffer());
	h.remove (3, 5);
	printf ("%s\n", h.get(3).getBuffer());
	h.remove (3, 5);
	printf ("%s\n", h.get(3).getBuffer());
#endif
	printf ("--------------------\n");
	for (qse_size_t i = 0; i < h.getSize(); i++)
	{
		printf ("[%s] at [%lu]\n", h[i].getBuffer(), (unsigned long int)h.getIndex(h[i]));
	}
	printf ("--------------------\n");

	StrArray h2 (h);
	StrArray h3;

	h3 = h2;
	h.clear (true);
	printf ("--------------------\n");
	printf ("%d\n", (int)h.getSize());
	printf ("--------------------\n");
	for (qse_size_t i = 0; i < h2.getSize(); i++)
	{
		printf ("[%s] at [%lu]\n", h2[i].getBuffer(), (unsigned long int)h2.getIndex(h2[i]));
	}
	printf ("--------------------\n");

	h3.insert (21, "this is a large string");
	printf ("%d %d\n", (int)h2.getSize(), (int)h3.getSize());
	printf ("--------------------\n");
	h3.insert (21, "mystery!");
	for (qse_size_t i = 0; i < h3.getSize(); i++)
	{
		printf ("[%s] at [%lu]\n", h3[i].getBuffer(), (unsigned long int)h3.getIndex(h3[i]));
	}

	printf ("--------------------\n");
	h3.setCapacity (6);
	h3.insert (6, "good?");
	h3.rotate (StrArray::ROTATE_RIGHT, h3.getSize() / 2);
	printf ("[%s] [%s]\n", h3.getValueAt(5).getBuffer(), h3.getValueAt(6).getBuffer());
	printf ("%d\n", (int)h3.getSize());

	printf ("--------------------\n");
	h3.insert (1, "bad?");
	for (qse_size_t i = 0; i < h3.getSize(); i++)
	{
		printf ("[%s] at [%lu]\n", h3[i].getBuffer(), (unsigned long int)h3.getIndex(h3[i]));
	}

	printf ("--------------------\n");
	QSE::Array<int> a;
	a.insert (0, 10);
	a.insert (0, 20);
	a.insert (0, 30);
	const int& t = a[2u];
	printf ("%lu\n", (unsigned long int)a.getIndex(t));


	return 0;
}
