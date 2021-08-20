#ifndef _QSE_DHCP_DHCP6MSG_H_
#define _QSE_DHCP_DHCP6MSG_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_DHCP6_SERVER_PORT     547
#define QSE_DHCP6_CLIENT_PORT     546
#define QSE_DHCP6_HOP_COUNT_LIMIT 32

enum qse_dhcp6_msg_t
{
	QSE_DHCP6_MSG_SOLICIT     = 1,
	QSE_DHCP6_MSG_ADVERTISE   = 2,
	QSE_DHCP6_MSG_REQUEST     = 3,
	QSE_DHCP6_MSG_CONFIRM     = 4,
	QSE_DHCP6_MSG_RENEW       = 5,
	QSE_DHCP6_MSG_REBIND      = 6,
	QSE_DHCP6_MSG_REPLY       = 7,
	QSE_DHCP6_MSG_RELEASE     = 8,
	QSE_DHCP6_MSG_DECLINE     = 9,
	QSE_DHCP6_MSG_RECONFIGURE = 10,
	QSE_DHCP6_MSG_INFOREQ     = 11,
	QSE_DHCP6_MSG_RELAYFORW   = 12,
	QSE_DHCP6_MSG_RELAYREPL   = 13,
};
typedef enum qse_dhcp6_msg_t qse_dhcp6_msg_t;

enum qse_dhcp6_opt_t
{
	QSE_DHCP6_OPT_CLIENTID = 1,
	QSE_DHCP6_OPT_SERVERID = 2,
	QSE_DHCP6_OPT_IA_NA = 3,
	QSE_DHCP6_OPT_IA_TA = 4,
	QSE_DHCP6_OPT_IAADDR = 5,
	QSE_DHCP6_OPT_PREFERENCE = 7,
	QSE_DHCP6_OPT_ELAPSED_TIME = 8,
	QSE_DHCP6_OPT_RELAY_MESSAGE = 9,
	QSE_DHCP6_OPT_RAPID_COMMIT = 14,
	QSE_DHCP6_OPT_USER_CLASS = 15,
	QSE_DHCP6_OPT_VENDOR_CLASS = 16,
	QSE_DHCP6_OPT_INTERFACE_ID = 18,
	QSE_DHCP6_OPT_IA_PD = 25,
	QSE_DHCP6_OPT_IAPREFIX = 26
};
typedef enum qse_dhcp6_opt_t qse_dhcp6_opt_t;

/* --------------------------------------------------- */
#include <qse/pack1.h>
/* --------------------------------------------------- */

struct qse_dhcp6_pkt_hdr_t
{
	qse_uint8_t msgtype;
	qse_uint8_t transid[3];
};
typedef struct qse_dhcp6_pkt_hdr_t qse_dhcp6_pkt_hdr_t;

struct qse_dhcp6_relay_hdr_t
{
	qse_uint8_t msgtype;  /* RELAY-FORW, RELAY-REPL */
	qse_uint8_t hopcount;
	qse_uint8_t linkaddr[16];
	qse_uint8_t peeraddr[16];
};
typedef struct qse_dhcp6_relay_hdr_t qse_dhcp6_relay_hdr_t;

struct qse_dhcp6_opt_hdr_t
{
	qse_uint16_t code;
	qse_uint16_t len; /* length of option data, excludes the option header */
};
typedef struct qse_dhcp6_opt_hdr_t qse_dhcp6_opt_hdr_t;

/* --------------------------------------------------- */
#include <qse/unpack.h>
/* --------------------------------------------------- */

struct qse_dhcp6_pktinf_t
{
	qse_dhcp6_pkt_hdr_t* hdr;
	qse_size_t           len;
};
typedef struct qse_dhcp6_pktinf_t qse_dhcp6_pktinf_t;


#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_dhcp6_opt_hdr_t* qse_dhcp6_find_option (
	const qse_dhcp6_pktinf_t* pkt,
	int                       code
);

#ifdef __cplusplus
}
#endif

#endif
