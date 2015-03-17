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


		qse_printf (QSE_T("[%s] [%c] capa=%d len=%d\n"), x.getBuffer(), x[0], (int)x.getCapacity(), (int)x.getLength());
		qse_printf (QSE_T("[%s] [%c] capa=%d len=%d\n"), z->getBuffer(), (*z)[0], (int)z->getCapacity(), (int)z->getLength());

		qse_printf (QSE_T("%d %d\n"), (int)z->findIndex (0, QSE_T("K")), (int)z->findLastIndex (0, QSE_T("K")));
	}

	qse_printf (QSE_T("-----------------\n"));
	delete z;

}

int main ()
{


	qse_openstdsios ();

	t1 ();
	qse_printf (QSE_T("=================\n"));
	qse_closestdsios ();

	return 0;
}
