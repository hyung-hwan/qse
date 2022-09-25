#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/dhcp/dhcpmsg.h>
#include <qse/si/sio.h>
#include <string.h>
#include <qse/cmn/ipad.h>
#include <qse/cmn/test.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) goto oops; \
	} while (0)


static int test10()
{
	struct sockaddr_in sin;
	int s;
	qse_uint8_t buf[10000];
	qse_dhcp4_pktbuf_t pb;
	qse_ip4ad_t ip4ad[3];

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) return -1;

	memset (&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("192.168.1.2");
	sin.sin_port = htons(67);

	qse_dhcp4_initialize_pktbuf (&pb, buf, QSE_SIZEOF(buf));
	pb.hdr->op = QSE_DHCP4_OP_BOOTREQUEST;
	pb.hdr->htype = QSE_DHCP4_HTYPE_ETHERNET;
	pb.hdr->hlen = 6;

	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_HOST_NAME, "my.server", 9);
	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_PADDING, QSE_NULL, 0);
	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_PADDING, QSE_NULL, 0);
	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_PADDING, QSE_NULL, 0);
	qse_strtoip4ad (QSE_T("192.168.1.1"), &ip4ad[0]);
	qse_strtoip4ad (QSE_T("192.168.1.2"), &ip4ad[1]);
	qse_strtoip4ad (QSE_T("192.168.1.3"), &ip4ad[2]);
	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_NAME_SERVER, ip4ad, QSE_SIZEOF(ip4ad));
	qse_dhcp4_add_option (&pb, QSE_DHCP4_OPT_END, QSE_NULL, 0);
	sendto (s, pb.hdr, pb.len, 0, &sin, sizeof(sin));

	close (s);
	return 0;
}

int main ()
{
	qse_open_stdsios (); 

	R (test10);

oops:
	qse_close_stdsios ();
	return 0;
}

