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

	if (s.open (QSE_AF_INET6, QSE_SOCK_STREAM, 0) <= -1)
	{
		qse_printf (QSE_T("cannot open socket\n"));
		return -1;
	}

	//if (addr.resolve(QSE_T("0"), QSE_T("tango6.miflux.com"), QSE_SOCK_STREAM) >= 0)
	if (addr.resolve(QSE_T("0"), QSE_T("code.miflux.com"), QSE_SOCK_STREAM) >= 0)
	{
		qse_printf (QSE_T("code.miflux.com ===> [%js]\n"), (const qse_char_t*)addr.toString());
	}

	// if 'addr' is set to IPv6 above, this resolve() will fail if the given host doesn't have an IPv6 address
	// specifying the family to QSE_AF_INET overrides this behavior.
	if (addr.resolve(QSE_T("https"), QSE_T("code.miflux.com"), QSE_AF_INET, QSE_SOCK_STREAM) >= 0)
	{
		qse_printf (QSE_T("code.miflux.com:https ===> [%js]\n"), (const qse_char_t*)addr.toString());
	}

	qse_printf (QSE_T("lo ifindex ===> %d\n"), s.getIfceIndex(QSE_MT("lo")));
	if (s.getIfceAddress(QSE_T("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo ifaddr ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceNetmask(QSE_T("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo netmask ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceBroadcast(QSE_T("lo"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("lo broadcast ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}


	qse_printf (QSE_T("eth0 ifindex ===> %d\n"), s.getIfceIndex(QSE_T("eth0")));
	if (s.getIfceAddress(QSE_T("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 ifaddr ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceNetmask(QSE_T("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 netmask ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}
	if (s.getIfceBroadcast(QSE_T("eth0"), &addr) >= 0)
	{
		qse_char_t buf[256];
		qse_printf (QSE_T("eth0 broadcast ===> [%s]\n"), addr.toStrBuf(buf, QSE_COUNTOF(buf)));
	}


	addr.set ("[::1]:9999");
	if (s.connect(addr) <= -1)
	{
		qse_printf (QSE_T("unable to connect to [::1]:9999\n"));
	}
	else
	{
		qse_ioptl_t k[3];
		k[0].ptr = (void*)"hello";
		k[0].len = 5;
		k[1].ptr = (void*)"world";
		k[1].len = 5;
		k[2].ptr = (void*)"forever";
		k[2].len = 7;
		s.sendx (k, 3);
	}

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
