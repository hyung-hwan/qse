#include <stdio.h>
#include <qse/cmn/RedBlackTree.hpp>

class IntPair
{
public:
	IntPair (int x = 0, int y = 0): x (x), y (y) {}

	int getX () const { return this->x; }
	int getY () const { return this->y; }

	bool operator== (const IntPair& ip) const { return this->x == ip.x; }
	bool operator< (const IntPair& ip) const { return this->x < ip.x; }
	bool operator> (const IntPair& ip) const { return this->x > ip.x; }

protected:
	int x, y;
};

typedef QSE::RedBlackTree<IntPair> IntTree;

int main ()
{

	IntTree t (0, 10000);
	//IntTree t (0);

	for (int i = 0; i < 20 ; i++)
	{
		t.insert (IntPair (i , i * 2));
	}
	t.clear (true);
	for (int i = 0; i < 20 ; i++)
	{
		t.insert (IntPair (i , i * 2));
	}

	IntTree::ConstIterator it;
	for (it = t.getConstIterator(); it.isLegit(); ++it)
	{
		printf ("%d %d\n", it.getValue().getX(), (*it).getY());
	}


	printf ("------------------\n");
	IntTree x(t);
	for (it = t.getConstIterator(); it.isLegit(); ++it)
	{
		printf ("%d %d\n", it.getValue().getX(), (*it).getY());
	}



	printf ("------------------\n");
	t.insert (IntPair(99, 999));
	t.insert (IntPair(88, 888));
	//IntTree y (QSE_NULL, 5);
	x = t;
	for (it = x.getConstIterator(); it.isLegit(); ++it)
	{
		printf ("%d %d\n", it.getValue().getX(), (*it).getY());
	}
	return 0;
}
