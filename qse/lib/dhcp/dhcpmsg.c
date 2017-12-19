#include <qse/dhcp/dhcpmsg.h>
#include <qse/cmn/hton.h>
#include "../cmn/mem-prv.h"

qse_uint8_t* qse_dhcp4_get_options (const qse_dhcp4_pkt_t* pkt, qse_size_t len, qse_size_t* olen)
{
	qse_uint32_t cookie;
	qse_size_t optlen;
	qse_uint8_t* opt;

	/* check if a packet is large enough to hold the known header */
	if (len < QSE_SIZEOF(qse_dhcp4_pkt_t)) return QSE_NULL; 

	/* get the length of option fields */
	optlen = len - QSE_SIZEOF(qse_dhcp4_pkt_t);

	/* check if a packet is large enough to have a magic cookie */
	if (optlen < QSE_SIZEOF(cookie)) return QSE_NULL; 

	/* get the pointer to the beginning of options */
	opt = (qse_uint8_t*)(pkt + 1);

	/* use QSE_MEMCPY to prevent any alignment issues */
	QSE_MEMCPY (&cookie, opt, QSE_SIZEOF(cookie));
	/* check if the packet contains the right magic cookie */
	if (cookie != qse_hton32(QSE_DHCP4_MAGIC_COOKIE)) return QSE_NULL;

	*olen = optlen - QSE_SIZEOF(cookie);
	return (qse_uint8_t*)(opt + QSE_SIZEOF(cookie));
}

qse_uint8_t* qse_dhcp4_get_option (const qse_dhcp4_pkt_t* pkt, qse_size_t len, int code, qse_uint8_t* olen)
{
	const qse_uint8_t* optptr[3];
	qse_size_t optlen[3];
	int i;

	optptr[0] = qse_dhcp4_get_options (pkt, len, &optlen[0]);
	if (optptr[0] == QSE_NULL) return QSE_NULL;

	optptr[1] = (const qse_uint8_t*)pkt->file;
	optptr[2] = (const qse_uint8_t*)pkt->sname;
	optlen[1] = 0;
	optlen[2] = 0;

	for (i = 0; i < 3; i++)
	{
		const qse_uint8_t* opt = optptr[i];
		const qse_uint8_t* end = opt + optlen[i];

		while (opt < end)
		{
			/* option code */
			qse_uint8_t oc, ol;

			oc = *opt++;

			if (oc == QSE_DHCP4_OPT_PADDING) continue;
			if (oc == QSE_DHCP4_OPT_END) break;
		
			if (opt >= end) 
			{
				/*return QSE_NULL; */
				break;
			}

			/* option length */
			ol = *opt++;

			if (oc == code)
			{
				if (opt + ol >= end) 
				{
					/*return QSE_NULL; */
					break;
				}

				*olen = ol;
				return (qse_uint8_t*)opt;
			}

			if (oc == QSE_DHCP4_OPT_OVERLOAD)
			{
				if (ol != 1) 
				{
					/*return QSE_NULL; */
					break;
				}

				if (*opt & QSE_DHCP4_OPT_OVERLOAD_FILE) 
					optlen[1] = QSE_SIZEOF(pkt->file);
				if (*opt & QSE_DHCP4_OPT_OVERLOAD_SNAME) 
					optlen[2] = QSE_SIZEOF(pkt->sname);
			}

			opt += ol;
		}
	}

	return QSE_NULL;
}

qse_uint8_t* qse_dhcp4_get_relay_suboption (const qse_uint8_t* ptr, qse_uint8_t len, int code, qse_uint8_t* olen)
{
	const qse_uint8_t* end = ptr + len;

	while (ptr < end)
	{
		qse_uint8_t oc, ol;

		oc = *ptr++;

		if (ptr >= end) break;
		ol = *ptr++;

		if (oc == code)
		{
			*olen = ol;
			return (qse_uint8_t*)ptr;
		}

		ptr += ol;
	}

	return QSE_NULL;
}


int qse_dhcp4_add_option (qse_dhcp4_pkt_t* pkt, qse_size_t len, qse_size_t max, int code, qse_uint8_t* optr, qse_uint8_t olen)
{
	qse_size_t optlen;

#if 0
	/* check if a packet is large enough to hold the known header */
	if (len < QSE_SIZEOF(qse_dhcp4_pkt_t)) return -1; 

	/* get the length of option fields */
	optlen = len - QSE_SIZEOF(qse_dhcp4_pkt_t);

	/* check if a packet is large enough to have a magic cookie */
	if (optlen < QSE_SIZEOF(cookie)) return QSE_NULL; 

	/* get the pointer to the beginning of options */
	opt = (qse_uint8_t*)(pkt + 1);

	/* use QSE_MEMCPY to prevent any alignment issues */
	QSE_MEMCPY (&cookie, opt, QSE_SIZEOF(cookie));
	/* check if the packet contains the right magic cookie */
	if (cookie != qse_hton32(QSE_DHCP4_MAGIC_COOKIE)) return QSE_NULL;

	*olen = optlen - QSE_SIZEOF(cookie);
	optr = (qse_uint8_t*)(opt + QSE_SIZEOF(cookie));
#endif

	return -1;
}

