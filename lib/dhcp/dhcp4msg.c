#include <qse/dhcp/dhcpmsg.h>
#include <qse/cmn/hton.h>
#include "../cmn/mem-prv.h"

#include <qse/pack1.h>
struct magic_cookie_t
{
	qse_uint32_t value;
};
typedef struct magic_cookie_t magic_cookie_t;
#include <qse/unpack.h>

int qse_dhcp4_initialize_pktbuf (qse_dhcp4_pktbuf_t* pkt, void* buf, qse_size_t capa)
{
	if (capa < QSE_SIZEOF(*pkt->hdr)) return -1;
	pkt->hdr = (qse_dhcp4_pkt_hdr_t*)buf;
	pkt->len = QSE_SIZEOF(*pkt->hdr);
	pkt->capa = capa;
	QSE_MEMSET (pkt->hdr, 0, QSE_SIZEOF(*pkt->hdr));
	return 0;
}

int qse_dhcp4_add_option (qse_dhcp4_pktbuf_t* pkt, int code, void* optr, qse_uint8_t olen)
{
	qse_dhcp4_opt_hdr_t* opthdr;
	magic_cookie_t* cookie; 
	int optlen;

/* TODO: support to override sname and file */
	if (pkt->len < QSE_SIZEOF(*pkt->hdr) || pkt->capa < pkt->len) 
	{
		/* the pktbuf_t structure got messy */
		return -1;
	}

	if (pkt->len == QSE_SIZEOF(*pkt->hdr))
	{
		/* the first option is being added */
		if (pkt->capa - pkt->len < QSE_SIZEOF(*cookie)) return -1;
		cookie = (magic_cookie_t*)((qse_uint8_t*)pkt->hdr + pkt->len);
		cookie->value = QSE_CONST_HTON32(QSE_DHCP4_MAGIC_COOKIE);
		pkt->len += QSE_SIZEOF(*cookie);
	}
	else if (pkt->len < QSE_SIZEOF(*pkt->hdr) + QSE_SIZEOF(*cookie))
	{
		/* no space for cookie */
		return -1;
	}
	else
	{
		cookie = (magic_cookie_t*)(pkt->hdr + 1);
		if (cookie->value != QSE_CONST_HTON32(QSE_DHCP4_MAGIC_COOKIE)) return -1;
	}

/* do i need to disallow adding a new option if END is found? */

	if (code == QSE_DHCP4_OPT_PADDING || code == QSE_DHCP4_OPT_END)
	{
		optlen = 1; /* no length field in the header and no option palyload */
		if (pkt->capa - pkt->len < optlen) return -1;
		opthdr = (qse_dhcp4_opt_hdr_t*)((qse_uint8_t*)pkt->hdr + pkt->len);
	}
	else
	{
		optlen = QSE_SIZEOF(*opthdr) + olen;

		if (pkt->capa - pkt->len < optlen) return -1;
		opthdr = (qse_dhcp4_opt_hdr_t*)((qse_uint8_t*)pkt->hdr + pkt->len);

		opthdr->len = olen;
		if (olen > 0) QSE_MEMCPY (opthdr + 1, optr, olen);
	}

	opthdr->code = code;
	pkt->len += optlen;

	return 0;
}

int qse_dhcp4_delete_option (qse_dhcp4_pktbuf_t* pkt, int code)
{
	qse_dhcp4_opt_hdr_t* ohdr;
	qse_size_t olen;
	qse_uint8_t* ovend;

	ohdr = qse_dhcp4_find_option((qse_dhcp4_pktinf_t*)pkt, code);
	if (!ohdr) return -1;

	olen = (code == QSE_DHCP4_OPT_PADDING || code == QSE_DHCP4_OPT_END)? 1: (ohdr->len) + QSE_SIZEOF(*ohdr);

	if ((ohdr >= pkt->hdr->file && ohdr < (ovend = (qse_uint8_t*)pkt->hdr->file + QSE_SIZEOF(pkt->hdr->file))) ||
	    (ohdr >= pkt->hdr->sname && ohdr < (ovend = (qse_uint8_t*)pkt->hdr->sname + QSE_SIZEOF(pkt->hdr->sname))))
	{
		/* the option resides in the overload area */
		QSE_MEMMOVE (ohdr, (qse_uint8_t*)ohdr + olen, ovend - ((qse_uint8_t*)ohdr + olen));
		QSE_MEMSET (ovend - olen, 0, olen);
		/* packet length remains unchanged */
	}
	else
	{
		QSE_MEMMOVE (ohdr, (qse_uint8_t*)ohdr + olen, ((qse_uint8_t*)pkt->hdr + pkt->len) - ((qse_uint8_t*)ohdr + olen));
		pkt->len -= olen;
	}
	return 0;
}

void qse_dhcp4_compact_options (qse_dhcp4_pktbuf_t* pkt)
{
	/* TODO: move some optiosn to sname or file fields if they are not in use. */
}

static qse_uint8_t* get_option_start (const qse_dhcp4_pkt_hdr_t* pkt, qse_size_t len, qse_size_t* olen)
{
	magic_cookie_t* cookie;
	qse_size_t optlen;

	/* check if a packet is large enough to hold the known header */
	if (len < QSE_SIZEOF(qse_dhcp4_pkt_hdr_t)) return QSE_NULL; 

	/* get the length of option fields */
	optlen = len - QSE_SIZEOF(qse_dhcp4_pkt_hdr_t);

	/* check if a packet is large enough to have a magic cookie */
	if (optlen < QSE_SIZEOF(*cookie)) return QSE_NULL; 

	/* get the pointer to the beginning of options */
	cookie = (magic_cookie_t*)(pkt + 1);

	/* check if the packet contains the right magic cookie */
	if (cookie->value != QSE_CONST_HTON32(QSE_DHCP4_MAGIC_COOKIE)) return QSE_NULL;

	*olen = optlen - QSE_SIZEOF(*cookie);
	return (qse_uint8_t*)(cookie + 1);
}

int qse_dhcp4_walk_options (const qse_dhcp4_pktinf_t* pkt, qse_dhcp4_opt_walker_t walker)
{
	const qse_uint8_t* optptr[3];
	qse_size_t optlen[3];
	int i;

	optptr[0] = get_option_start(pkt->hdr, pkt->len, &optlen[0]);
	if (optptr[0] == QSE_NULL) return -1;

	optptr[1] = (const qse_uint8_t*)pkt->hdr->file;
	optptr[2] = (const qse_uint8_t*)pkt->hdr->sname;
	optlen[1] = 0;
	optlen[2] = 0;

	for (i = 0; i < 3; i++)
	{
		const qse_uint8_t* opt = optptr[i];
		const qse_uint8_t* end = opt + optlen[i];

		while (opt < end)
		{
			/* option code */
			qse_dhcp4_opt_hdr_t* opthdr;

			if (opt + QSE_SIZEOF(*opthdr) >= end) return -1;
			opthdr = (qse_dhcp4_opt_hdr_t*)opt;
			opt += QSE_SIZEOF(*opthdr);

			/* no len field exists for PADDING and END */
			if (opthdr->code == QSE_DHCP4_OPT_PADDING) continue; 
			if (opthdr->code == QSE_DHCP4_OPT_END) break;

			if (opt + opthdr->len >= end) return -1; /* the length field is wrong */

			if (opthdr->code == QSE_DHCP4_OPT_OVERLOAD)
			{
				if (opthdr->len != 1) return -1;
				if (*opt & QSE_DHCP4_OPT_OVERLOAD_FILE) optlen[1] = QSE_SIZEOF(pkt->hdr->file);
				if (*opt & QSE_DHCP4_OPT_OVERLOAD_SNAME) optlen[2] = QSE_SIZEOF(pkt->hdr->sname);
			}
			else
			{
				int n;
				if ((n = walker(opthdr)) <= -1) return -1;
				if (n == 0) break; /* stop */
			}

			opt += opthdr->len;
		}
	}

	return 0;
}

qse_dhcp4_opt_hdr_t* qse_dhcp4_find_option (const qse_dhcp4_pktinf_t* pkt, int code)
{
	const qse_uint8_t* optptr[3];
	qse_size_t optlen[3];
	int i;

	optptr[0] = get_option_start(pkt->hdr, pkt->len, &optlen[0]);
	if (!optptr[0]) return QSE_NULL;

	optptr[1] = (const qse_uint8_t*)pkt->hdr->file;
	optptr[2] = (const qse_uint8_t*)pkt->hdr->sname;
	optlen[1] = 0;
	optlen[2] = 0;

	for (i = 0; i < 3; i++)
	{
		const qse_uint8_t* opt = optptr[i];
		const qse_uint8_t* end = opt + optlen[i];

		while (opt < end)
		{
			/* option code */
			qse_dhcp4_opt_hdr_t* opthdr;

			/* at least 1 byte is available. the check is because of PADDING or END */
			if (*opt == QSE_DHCP4_OPT_PADDING)
			{
				opt++;
				continue;
			}
			if (*opt == QSE_DHCP4_OPT_END)
			{
				if (code == QSE_DHCP4_OPT_END)
				{
					/* the caller must handle END specially becuase it is only 1 byte long
				 	 * for no length part in the header */
					return (qse_dhcp4_opt_hdr_t*)opt;
				}
				break;
			}

			if (opt + QSE_SIZEOF(*opthdr) > end) break;

			opthdr = (qse_dhcp4_opt_hdr_t*)opt;
			opt += QSE_SIZEOF(*opthdr);

			/* option length */

			if (opthdr->code == code)
			{
				if (opt + opthdr->len > end) break;
				return opthdr;
			}

			/*
			 * If option overload is used, the SName and/or File fields are read and
			 * interpreted in the same way as the Options field, after all options in
			 * the Option field are parsed. If the message actually does need to carry
			 * a server name or boot file, these are included as separate options
			 * (number 66 and number 67, respectively), which are variable-length and
			 * can therefore be made exactly the length needed.
			 */
			if (opthdr->code == QSE_DHCP4_OPT_OVERLOAD)
			{
				if (opthdr->len != 1) break;
				if (*opt & QSE_DHCP4_OPT_OVERLOAD_FILE) optlen[1] = QSE_SIZEOF(pkt->hdr->file);
				if (*opt & QSE_DHCP4_OPT_OVERLOAD_SNAME) optlen[2] = QSE_SIZEOF(pkt->hdr->sname);
			}

			opt += opthdr->len;
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


