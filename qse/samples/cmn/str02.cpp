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
		qse_printf (QSE_T("[%s]\n"), x.getBuffer());
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
