#include <stdio.h>
#include <string>
#include <qse/cmn/BinaryHeap.hpp>
#include <qse/cmn/LinkedList.hpp>
#include <string.h>

class PosStr: public std::string
{
public:
	PosStr (const char* str = ""): std::string(str), heap_pos((qse_size_t)-1) {};
	
	PosStr (const PosStr& ps): std::string (ps), heap_pos(ps.heap_pos)  
	{
	}

	~PosStr()
	{
	}


	qse_size_t heap_pos;
};

struct cstr_comparator
{
	bool operator() (const char* v1, const PosStr& v2) const
	{
		//return strcmp (v1, v2.c_str()) > 0;
		return strcmp (v1, v2.c_str()) < 0;
	}
};

struct str_comparator
{
	bool operator() (const PosStr& v1, const PosStr& v2) const
	{
		//return strcmp (v1.c_str(), v2.c_str()) > 0;
		return strcmp (v1.c_str(), v2.c_str()) < 0;
	}
};


typedef  QSE::BinaryHeap<PosStr,str_comparator> StrHeap;
typedef  QSE::LinkedList<PosStr> StrList;


class Container
{
public:
	struct str_ptr_greater_than
	{
		bool operator() (const StrList::Node* v1, const StrList::Node* v2) const
		{
			//return strcmp (v1.c_str(), v2.c_str()) > 0;
			return strcmp (v1->value.c_str(), v2->value.c_str()) > 0;
		}
	};

	struct str_ptr_positioner
	{
		void operator() (StrList::Node* v, qse_size_t index) const
		{
			v->value.heap_pos = index;	
		}
	};


	typedef  QSE::BinaryHeap<StrList::Node*,str_ptr_greater_than,str_ptr_positioner> StrPtrHeap;

	StrList::Node* insert (const char* str)
	{
		StrList::Node* node = this->str_list.append (PosStr(str));
		qse_size_t heap_pos = this->str_heap.insert (node);

		return node;
	}

	bool isEmpty () const
	{
		QSE_ASSERT (this->str_list.isEmpty() == this->str_heap.isEmpty());
		return this->str_list.isEmpty();
	}

	qse_size_t getSize() const
	{
		QSE_ASSERT (this->str_list.getSize() == this->str_heap.getSize());
		return this->str_list.getSize();
	}

	const PosStr& getLargest () const
	{
		StrList::Node* node = this->str_heap.getValueAt(0);
		return node->value;
	}

	void removeLargest () 
	{
		StrList::Node* node = this->str_heap.getValueAt(0);
		this->str_heap.remove (0);
		this->str_list.remove (node);
	}

	void remove (StrList::Node* node)
	{
		this->str_heap.remove (node->value.heap_pos);
		this->str_list.remove (node);	
	}

	StrList::Node* getHeapValueAt (qse_size_t index)
	{
		return this->str_heap.getValueAt(index);
	}


	StrList str_list;
	StrPtrHeap str_heap;
};



 struct IntComparator
 {
         bool operator() (int v1, int v2) const
         {
                 //return !(v1 > v2);
                 //return v1 > v2;
                 return v1 < v2;
         }
 };
 
 

static StrHeap getStrHeap ()
{
	char buf[100];
	StrHeap h;

	for (int i = 0; i < 25; i++)
	{
		sprintf (buf, "binary heap item %d", i);
		h.insert (buf);
	}

	return h;
}

int main ()
{
	char buf[20];
	StrHeap h;

#define TOTAL 22
	printf ("[TEST #1 - %d strings must be shown]\n", TOTAL * 2);
	for (int i = 0; i < TOTAL; i++)
	{
		sprintf (buf, "hello %d", i);
		h.insert (buf);
		h.insert (buf);
	}
	if (h.getSize() != TOTAL * 2)
	{
		printf ("[FAILURE] ------------------------ \n");
	}
	else
	{
		for (qse_size_t i = 0; i < h.getSize(); i++)
		{
			printf ("%05d %s\n", (int)h.getIndex(h[i]), h[i].c_str());
		}
	}
	printf ("\n");


	//////////////////////////////////////////////////////////////////////////////  

	printf ("[TEST #2 - strings must be shown in ascending order]\n");
	const char* x =  h[0u].c_str();
	std::string oldx;
	while (true)
	{
		oldx = x;
		printf ("%s\n", x);
		h.remove (0);
		if (h.isEmpty()) break;
		x =  h[0u].c_str();
		if (strcmp (x, oldx.c_str()) < 0) 
		{
			printf ("[FAILURE] ------------------------ \n");
			break;
		}
	}
	printf ("\n");
	//////////////////////////////////////////////////////////////////////////////  


	printf ("[TEST #3 - integers must be shown in ascending order]\n");
	{
		QSE::BinaryHeap<int,IntComparator> h2;

		h2.insert (70);
		h2.insert (90);
		h2.insert (10);
		h2.insert (5);
		h2.insert (88);
		h2.insert (87);
		h2.insert (300);
		h2.insert (91);
		h2.insert (100);
		h2.insert (200);

		int x = h2.getValueAt(0);
		int oldx;
		while (true)
		{
			oldx = x;
			printf ("%d\n", x);
			h2.remove (0);
			if (h2.getSize() <= 0) break;
			x = h2.getValueAt(0);
			if (x < oldx)
			{
				printf ("[FAILURE] ------------------------ \n");
				break;
			}
		}
	}
	printf ("\n");
	//////////////////////////////////////////////////////////////////////////////  


	printf ("[TEST #4 - strings must be shown in the ascending order]\n");
	{
		StrHeap h (getStrHeap ());

		const char* x = h.getValueAt(0).c_str();
		std::string oldx;
		while (true)
		{
			oldx = x;
			printf ("%s\n", x);
			h.remove (0);
			if (h.isEmpty()) break;
			x = h.getValueAt(0).c_str();
			if (strcmp (x, oldx.c_str()) < 0) 
			{
				printf ("[FAILURE] ------------------------ \n");
				break;
			}
		}
	}
	printf ("\n");
	//////////////////////////////////////////////////////////////////////////////  

	printf ("[TEST #5 - random test]\n");
	{
		Container c;
		StrList::Node* node2, * node14;
		for (qse_size_t i = 0; i < TOTAL; i++)
		{
			sprintf (buf, "hello %d", (int)i);
	
			//c.insert (buf);
	
			if (i == 2) 
			{
				node2 = c.insert (buf);
				printf ("2nd => %s\n", node2->value.c_str());
			}
			else if (i == 14)
			{
				node14 = c.insert (buf);
				printf ("14th => %s\n", node14->value.c_str());
			}
			else c.insert (buf);
		}
	
		/*
	         for (int i = 0; i < c.getSize(); i++)
		{
			StrList::Node* node = c.getHeapValueAt(i);

	                 printf ("%s %d\n", node->value.c_str(), (int)node->value.heap_pos);
		}
		*/
		
		for (int i = 0; c.getSize() > 0; i++)
		{
			if (i == 3) c.remove (node2);
			if (i == 5) c.remove (node14);

			const char* largest = c.getLargest().c_str();
			printf ("%s\n", largest);

			c.removeLargest ();
		}
	}

	return 0;
}
