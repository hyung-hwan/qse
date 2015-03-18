#include <qse/cmn/sio.h>
#include <qse/cmn/String.hpp>
#include <qse/cmn/HeapMmgr.hpp>


void t1 ()
{
	QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);
	QSE::String* z = new QSE::String();

	{
		QSE::String x (&heap_mmgr, QSE_T("this is a sample string"));
		QSE::String y (x);

		*z = y;

		//z->setCharAt (0, QSE_T('Q'));
		//z->prepend (QSE_T("ok."));
		z->append (*z);
		for (int i = 0; i < 80; i++)
		{
			z->prepend (QSE_T("ok."));
			z->insert (10, QSE_T("XXX"));
		}
		z->update (10, 2, QSE_T("ZZZZ"));
		//z->update (QSE_T("QQQ"));

		z->replace (QSE_T("XX"), QSE_T("^"));
		//z->invert();


		qse_printf (QSE_T("x: [%s] [%c] capa=%d len=%d\n"), x.getBuffer(), x[0u], (int)x.getCapacity(), (int)x.getLength());
		qse_printf (QSE_T("z: [%s] [%c] capa=%d len=%d\n"), z->getBuffer(), (*z)[0u], (int)z->getCapacity(), (int)z->getLength());

		qse_printf (QSE_T("%d %d\n"), (int)z->findIndex (0, QSE_T("K")), (int)z->findLastIndex (0, QSE_T("K")));
		qse_printf (QSE_T("%d %d %d\n"), z->beginsWith (QSE_T("ok.ok")), z->beginsWith (QSE_T("ok.okX")), z->endsWith (QSE_T("string")));

		////////////////////////////////////////////////////
		QSE::String t(QSE_T("   hello   world   good   "));
		t.trim ();
		QSE_ASSERT (t.getLength() == 20);
		qse_printf (QSE_T("t: [%s] %d\n"), t.getBuffer(), t.getLength());

		t = QSE_T("  come on and join me   ");
		QSE_ASSERT (t.getLength() == 24);
		t.trimLeft ();
		QSE_ASSERT (t.getLength() == 22);
		qse_printf (QSE_T("t: [%s] %d\n"), t.getBuffer(), t.getLength());
		t = QSE_T("  come on and join me   ");
		t.trimRight ();
		QSE_ASSERT (t.getLength() == 21);
		qse_printf (QSE_T("t: [%s] %d\n"), t.getBuffer(), t.getLength());

		////////////////////////////////////////////////////
		QSE::String q (z->getSubstring (4, 10));
		QSE_ASSERT (q.getLength() == 10);
		QSE_ASSERT (q.getCharAt(0) == z->getCharAt(4));
		qse_printf (QSE_T("q: [%s] %d\n"), q.getBuffer(), q.getLength());
		q = z->getSubstring (z->getLength() - 5);
		QSE_ASSERT (q.getLength() == 5);
		qse_printf (QSE_T("q: [%s] %d\n"), q.getBuffer(), q.getLength());


		QSE::PercentageGrowthPolicy gp(1);
		QSE::String g1(128), g2(128);
		QSE_ASSERT (g1.getCapacity() == 128);
		QSE_ASSERT (g2.getCapacity() == 128);
		QSE_ASSERT (g1.getLength() == 0);
		QSE_ASSERT (g2.getLength() == 0);
		g2.setGrowthPolicy (&gp);
		for (int i = 0; i < 1500; i++)
		{
			g1.append (i);
			g2.append (i);
		}
		qse_printf (QSE_T("g1: %d %d g2: %d %d\n"), (int)g1.getCapacity(), (int)g1.getLength(), (int)g2.getCapacity(), (int)g2.getLength());
		g1.compact();
		g2.compact();
		qse_printf (QSE_T("g1: %d %d g2: %d %d\n"), (int)g1.getCapacity(), (int)g1.getLength(), (int)g2.getCapacity(), (int)g2.getLength());
	}

	qse_printf (QSE_T("-----------------\n"));
	delete z;

}

void t2()
{
	QSE::MbString x(QSE_MT("this is a string"));
	qse_printf (QSE_T("x: [%hs] %d %d\n"), x.getBuffer(), (int)x.getCapacity(), (int)x.getLength());
}

int main ()
{
	qse_openstdsios ();

	t1 ();
	qse_printf (QSE_T("=================\n"));
	t2 ();
	qse_printf (QSE_T("=================\n"));
	qse_closestdsios ();

	return 0;
}
