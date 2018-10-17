#include <qse/si/Socket.hpp>
#include <qse/si/mtx.h>
#include <qse/si/sio.h>
#include <qse/cmn/mem.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <string.h>

static int test1 ()
{
	QSE::Socket s;
	QSE::SocketAddress addr;
	qse_ioptl_t k[3];

	if (s.open (QSE_AF_INET6, QSE_SOCK_STREAM, 0) <= -1)
	{
		qse_printf (QSE_T("cannot open socket\n"));
		return -1;
	}

	qse_printf (QSE_T("lo ifindex ===> %d\n"), s.getIfceIndex(QSE_WT("lo")));
	if (s.getIfceAddress(QSE_WT("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo ifaddr ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceNetmask(QSE_WT("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo netmask ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceBroadcast(QSE_WT("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo broadcast ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}


	qse_printf (QSE_T("eth0 ifindex ===> %d\n"), s.getIfceIndex(QSE_WT("eth0")));
	if (s.getIfceAddress(QSE_WT("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 ifaddr ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceNetmask(QSE_WT("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 netmask ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceBroadcast(QSE_WT("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 broadcast ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}


	addr.set ("[::1]:9999");
	s.connect (addr);

	k[0].ptr = (void*)"hello";
	k[0].len = 5;
	k[1].ptr = (void*)"world";
	k[1].len = 5;
	k[2].ptr = (void*)"forever";
	k[2].len = 7;
	s.sendx (k, 3);

	return 0;
}

int main ()
{
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	}
#else
	setlocale (LC_ALL, "");
	/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
#endif

	qse_open_stdsios ();
	test1();
	qse_close_stdsios ();

	return 0;
}
