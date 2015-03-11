#include <stdio.h>
#include <qse/cmn/StdMmgr.hpp>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/cmn/HashTable.hpp>
#include <qse/cmn/sio.h>
#include <string>
#include <string.h>

struct IntHasher
{
	qse_size_t operator() (int v) const
	{
		return v;
	}
};

typedef  QSE::HashTable<int,int,IntHasher> IntTable;

struct IntClass
{
	IntClass (int x = 0): x (x) {}
	int x;
};

struct IntClassHasher
{
	qse_size_t operator() (const IntClass& v) const
	{
		return v.x;
	}
};

struct IntClassComparator
{
	bool operator() (const IntClass& v, int y) const
	{
		return v.x == y;
	}
};



struct StrHasher
{
	qse_size_t operator() (const std::string& v) const
	{
		return QSE::Hashable::getHashCode (v.c_str(), v.length());
	}
};

typedef  QSE::HashTable<std::string,int,StrHasher> Str1Table;

struct cstr_hasher
{
	qse_size_t operator() (const char* v) const
	{
		return QSE::Hashable::getHashCode (v, strlen(v));
	}
};

struct cstr_is_equal
{
	int operator() (const char* v1, const std::string& v2) const
	{
		return strcmp (v1, v2.c_str()) == 0;
	}
};


int main ()
{
//	qse_openstdsios ();

	QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 3000000);
	//QSE::HashTable<int,int,IntHasher> int_table (&heap_mmgr, 1000);
	IntTable int_table (NULL, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 1000);
	//IntTable int_table (NULL, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 0);
printf ("----------\n");
	for (int i = 0; i < 100; i++)
	{
		int_table.insert (i, i * 20);
	}
	for (int i = 50; i < 150; i++)
	{
		int_table.upsert (i, i * 20);
	}
printf ("----------\n");

/*
	qse_size_t bucket_size = int_table.getBucketSize();
	for (qse_size_t i = 0; i < bucket_size; i++)
	{
		IntTable::Bucket* b = int_table.getBucket (i);
	}
*/
printf ("----------\n");

/*
	IntTable::Pair* pair = int_table.findPairWithCustomKey<IntClass,IntClassHasher,IntClassComparator> (IntClass(50));
	if (pair)
	{
		printf ("pair found.... [%d]\n", pair->value);
	}
*/

	printf ("%p %p\n", int_table.search (60), int_table.search (90));
	printf ("%d %d\n", int_table.remove (60), int_table.remove (60));
	printf ("%d %d\n", int_table.remove (70), int_table.remove (70));

/*
	printf ("%d\n", int_table[90]);
	printf ("%d\n", int_table[9990]);
*/

	//Str1Table s1 (NULL, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 1000);
	Str1Table s1 (NULL, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 0);
	//Str1Table s1;
	s1.insert ("hello", 20);
	s1.insert ("hello", 20);
	s1.insert ("hello kara", 20);

	s1.insert ("this is good", 3896);
	printf ("remove = %d\n", s1.remove ("hello"));
	printf ("remove = %d\n", s1.remove ("hello"));

	for (int i = 0; i < 100; i++)
	{
		char buf[128];
		sprintf (buf, "surukaaaa %d", i);
		s1.insert (buf, i * 2);
	}

	printf ("%d\n", (int)s1.getSize());
	for (Str1Table::Iterator it = s1.getIterator(); it.isLegit(); it++)
	{
		Str1Table::Pair& pair = *it;
		printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
	}


	printf ("------------------\n");
	{
		Str1Table s11 (s1);
		for (Str1Table::Iterator it = s11.getIterator(); it.isLegit(); it++)
		{
			Str1Table::Pair& pair = *it;
			printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
		}
	}

	printf ("------------------\n");
	{
		Str1Table s11 (&heap_mmgr, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 0);
		//Str1Table s11 (NULL, IntTable::DEFAULT_CAPACITY, IntTable::DEFAULT_LOAD_FACTOR, 200);
		//Str1Table s11;

		for (int i = 0; i < 100; i++)
		{
			char buf[128];
			sprintf (buf, "abiyo %d", i);
			s11.insert (buf, i * 2);
		}
		printf ("%d\n", s11.heteroremove<const char*, cstr_hasher, cstr_is_equal> ("abiyo 12"));
		printf ("%d\n", s11.heteroremove<const char*, cstr_hasher, cstr_is_equal> ("abiyo 12"));
		printf ("SIZE => %d\n", (int)s11.getSize());
		for (Str1Table::Iterator it = s11.getIterator(); it.isLegit(); it++)
		{
			Str1Table::Pair& pair = *it;
			printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
		}
		printf ("------------------\n");

		//s11.clear (true);

		s11 = s1;
		printf ("SIZE => %d\n", (int)s11.getSize());
		for (Str1Table::Iterator it = s11.getIterator(); it.isLegit(); it++)
		{
			Str1Table::Pair& pair = *it;
			printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
		}


		printf ("-------------------\n");
		Str1Table::Pair* pair = s11.heterosearch<const char*, cstr_hasher, cstr_is_equal> ("surukaaaa 13");
		if (pair) printf ("%d\n", pair->value);
		else printf ("not found\n");

		s11.update ("surukaaaa 13", 999);
		s11.update ("surukaaaa 16", 99999);
		s11.inject ("surukaaaa 18", 999999, 1);

		pair = s11.heterosearch<const char*, cstr_hasher, cstr_is_equal> ("surukaaaa 13");
		if (pair) printf ("%d\n", pair->value);
		else printf ("not found\n");


		s1 = s11;
	}

	printf ("-------------------\n");
	printf ("%d %d\n", (int)s1.getSize(), (int)s1.getCapacity());
	for (Str1Table::ConstIterator it = s1.getConstIterator(); it.isLegit(); it++)
	{
		const Str1Table::Pair& pair = *it;
		printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
	}

	printf ("-------------------\n");

	for (Str1Table::PairNode* np = s1.getTailNode(); np; np = np->getPrevNode())
	{
		Str1Table::Pair& pair = np->value;
		printf ("[%s] [%d]\n", pair.key.c_str(), pair.value);
	}
	

//	qse_closestdsios ();
	return 0;	
}

