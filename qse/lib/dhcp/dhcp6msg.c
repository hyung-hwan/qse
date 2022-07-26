#include <qse/dhcp/dhcp6msg.h>
#include <qse/cmn/hton.h>

qse_dhcp6_opt_hdr_t* qse_dhcp6_find_option (const qse_dhcp6_pktinf_t* pkt, int code)
{
	qse_dhcp6_opt_hdr_t* opt;
	qse_size_t rem, opt_len;

	if (pkt->len < QSE_SIZEOF(qse_dhcp6_pkt_hdr_t)) return QSE_NULL;

	if (pkt->hdr->msgtype == QSE_DHCP6_MSG_RELAYFORW || pkt->hdr->msgtype == QSE_DHCP6_MSG_RELAYREPL)
	{
		if (pkt->len < QSE_SIZEOF(qse_dhcp6_relay_hdr_t)) return QSE_NULL;

		rem = pkt->len - QSE_SIZEOF(qse_dhcp6_relay_hdr_t);
		opt = (qse_dhcp6_opt_hdr_t*)(((qse_dhcp6_relay_hdr_t*)pkt->hdr) + 1);
	}
	else
	{
		rem = pkt->len - QSE_SIZEOF(qse_dhcp6_pkt_hdr_t);
		opt = (qse_dhcp6_opt_hdr_t*)(pkt->hdr + 1);
	}

	while (rem >= QSE_SIZEOF(qse_dhcp6_opt_hdr_t))
	{
		if (qse_ntoh16(opt->code) == code) 
		{
			if (rem - QSE_SIZEOF(qse_dhcp6_opt_hdr_t) < qse_ntoh16(opt->len)) return QSE_NULL; /* probably the packet is ill-formed */
			return opt;
		}

		opt_len = QSE_SIZEOF(qse_dhcp6_opt_hdr_t) + qse_ntoh16(opt->len);
		if (rem < opt_len) break;
		rem -= opt_len;
		opt = (qse_dhcp6_opt_hdr_t*)((qse_uint8_t*)(opt + 1) + qse_ntoh16(opt->len));
		
	}

	return QSE_NULL;
}
