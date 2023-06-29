#ifndef _QSE_DHCP_DHCP4MSG_H_
#define _QSE_DHCP_DHCP4MSG_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_DHCP4_SERVER_PORT   67
#define QSE_DHCP4_CLIENT_PORT   68
#define QSE_DHCP4_MAGIC_COOKIE  0x63825363

/* operation code */
enum qse_dhcp4_op_t
{
	QSE_DHCP4_OP_BOOTREQUEST = 1,
	QSE_DHCP4_OP_BOOTREPLY   = 2
};

enum qse_dhcp4_htype_t
{
	QSE_DHCP4_HTYPE_ETHERNET   = 1,
	QSE_DHCP4_HTYPE_IEEE802    = 6,
	QSE_DHCP4_HTYPE_ARCNET     = 7,
	QSE_DHCP4_HTYPE_APPLETALK  = 8,
	QSE_DHCP4_HTYPE_HDLC       = 17,
	QSE_DHCP4_HTYPE_ATM        = 19,
	QSE_DHCP4_HTYPE_INFINIBAND = 32
};

/* option codes (partial) */
enum qse_dhcp4_opt_t
{
	QSE_DHCP4_OPT_PADDING          = 0x00,
	QSE_DHCP4_OPT_SUBNET           = 0x01,
	QSE_DHCP4_OPT_TIME_OFFSET      = 0x02,
	QSE_DHCP4_OPT_ROUTER           = 0x03,
	QSE_DHCP4_OPT_TIME_SERVER      = 0x04,
	QSE_DHCP4_OPT_NAME_SERVER      = 0x05,
	QSE_DHCP4_OPT_DNS_SERVER       = 0x06,
	QSE_DHCP4_OPT_LOG_SERVER       = 0x07,
	QSE_DHCP4_OPT_COOKIE_SERVER    = 0x08,
	QSE_DHCP4_OPT_LPR_SERVER       = 0x09,
	QSE_DHCP4_OPT_HOST_NAME        = 0x0c,
	QSE_DHCP4_OPT_BOOT_SIZE        = 0x0d,
	QSE_DHCP4_OPT_DOMAIN_NAME      = 0x0f,
	QSE_DHCP4_OPT_SWAP_SERVER      = 0x10,
	QSE_DHCP4_OPT_ROOT_PATH        = 0x11,
	QSE_DHCP4_OPT_IP_TTL           = 0x17,
	QSE_DHCP4_OPT_MTU              = 0x1a,
	QSE_DHCP4_OPT_BROADCAST        = 0x1c,
	QSE_DHCP4_OPT_NTP_SERVER       = 0x2a,
	QSE_DHCP4_OPT_WINS_SERVER      = 0x2c,
	QSE_DHCP4_OPT_REQUESTED_IPADDR = 0x32,
	QSE_DHCP4_OPT_LEASE_TIME       = 0x33,
	QSE_DHCP4_OPT_OVERLOAD         = 0x34, /* overload sname or file */
	QSE_DHCP4_OPT_MESSAGE_TYPE     = 0x35,
	QSE_DHCP4_OPT_SERVER_ID        = 0x36,
	QSE_DHCP4_OPT_PARAM_REQ        = 0x37,
	QSE_DHCP4_OPT_MESSAGE          = 0x38,
	QSE_DHCP4_OPT_MAX_SIZE         = 0x39,
	QSE_DHCP4_OPT_T1               = 0x3a,
	QSE_DHCP4_OPT_T2               = 0x3b,
	QSE_DHCP4_OPT_VENDOR           = 0x3c,
	QSE_DHCP4_OPT_CLIENT_ID        = 0x3d,
	QSE_DHCP4_OPT_RELAY            = 0x52,
	QSE_DHCP4_OPT_SUBNET_SELECTION = 0x76,
	QSE_DHCP4_OPT_END              = 0xFF
};

/* flags for QSE_DHCP4_OPT_OVERLOAD */
enum qse_dhcp4_opt_overload_t
{
	QSE_DHCP4_OPT_OVERLOAD_FILE  = (1 << 0),
	QSE_DHCP4_OPT_OVERLOAD_SNAME = (1 << 1)
};

/* flags for QSE_DHCP4_OPT_OVERLOAD */
enum qse_dhcp4_opt_relay_t
{
	QSE_DHCP4_OPT_RELAY_CIRCUIT_ID  = 1,
	QSE_DHCP4_OPT_RELAY_REMOTE_ID   = 2
};

/* message type */
enum qse_dhcp4_msg_t
{
	QSE_DHCP4_MSG_DISCOVER         = 1,
	QSE_DHCP4_MSG_OFFER            = 2,
	QSE_DHCP4_MSG_REQUEST          = 3,
	QSE_DHCP4_MSG_DECLINE          = 4,
	QSE_DHCP4_MSG_ACK              = 5,
	QSE_DHCP4_MSG_NAK              = 6,
	QSE_DHCP4_MSG_RELEASE          = 7,
	QSE_DHCP4_MSG_INFORM           = 8,

	/*QSE_DHCP4_MSG_RENEW            = 9,*/

	QSE_DHCP4_MSG_LEASE_QUERY      = 10,
	QSE_DHCP4_MSG_LEASE_UNASSIGNED = 11,
	QSE_DHCP4_MSG_LEASE_UNKNOWN    = 12,
	QSE_DHCP4_MSG_LEASE_ACTIVE     = 13,

	QSE_DHCP4_MSG_BULK_LEASE_QUERY = 14,
	QSE_DHCP4_MSG_LEASE_QUERY_DONE = 15
};

/* --------------------------------------------------- */
#include <qse/pack1.h>
/* --------------------------------------------------- */

struct qse_dhcp4_pkt_hdr_t
{
	qse_uint8_t  op; 
	qse_uint8_t  htype; 
	qse_uint8_t  hlen; 
	qse_uint8_t  hops;
	qse_uint32_t xid;        /* transaction id */
	qse_uint16_t secs;       /* seconds elapsed */
	qse_uint16_t flags;      /* bootp flags */
	qse_uint32_t ciaddr;     /* client ip */
	qse_uint32_t yiaddr;     /* your ip */
	qse_uint32_t siaddr;     /* next server ip */
	qse_uint32_t giaddr;     /* relay agent ip */
	qse_uint8_t  chaddr[16]; /* client mac */

	char     sname[64];      /* server host name */
	char     file[128];      /* boot file name */

	/* options are placed after the header.
	 * the first four bytes of the options compose a magic cookie  
	 * 0x63 0x82 0x53 0x63 */
};
typedef struct qse_dhcp4_pkt_hdr_t qse_dhcp4_pkt_hdr_t;

struct qse_dhcp4_opt_hdr_t
{
	qse_uint8_t code;
	qse_uint8_t len;
};
typedef struct qse_dhcp4_opt_hdr_t qse_dhcp4_opt_hdr_t;

/* --------------------------------------------------- */
#include <qse/unpack.h>
/* --------------------------------------------------- */



typedef int (*qse_dhcp4_opt_walker_t) (qse_dhcp4_opt_hdr_t* opt);

struct qse_dhcp4_pktinf_t
{
	qse_dhcp4_pkt_hdr_t* hdr;
	qse_size_t           len;
};
typedef struct qse_dhcp4_pktinf_t qse_dhcp4_pktinf_t;

struct qse_dhcp4_pktbuf_t
{
	qse_dhcp4_pkt_hdr_t* hdr;
	qse_size_t           len;
	qse_size_t           capa;
};
typedef struct qse_dhcp4_pktbuf_t qse_dhcp4_pktbuf_t;


#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT int qse_dhcp4_initialize_pktbuf (
	qse_dhcp4_pktbuf_t* pkt,
	void*               buf,
	qse_size_t          capa
);

QSE_EXPORT int qse_dhcp4_add_option (
	qse_dhcp4_pktbuf_t* pkt,
	int                 code,
	void*               optr, /**< option data pointer */
	qse_uint8_t         olen  /**< option data length */
);

QSE_EXPORT int qse_dhcp4_delete_option (
	qse_dhcp4_pktbuf_t* pkt,
	int                 code
);

QSE_EXPORT void qse_dhcp4_compact_options (
	qse_dhcp4_pktbuf_t* pkt
);

#if 0
QSE_EXPORT int qse_dhcp4_add_options (
	qse_dhcp4_pkt_hdr_t* pkt,
	qse_size_t       len,
	qse_size_t       max,
	int              code,
	qse_uint8_t*     optr, /* option data */
	qse_uint8_t      olen  /* option length */
);
#endif

QSE_EXPORT int qse_dhcp4_walk_options (
	const qse_dhcp4_pktinf_t* pkt,
	qse_dhcp4_opt_walker_t    walker
);

QSE_EXPORT qse_dhcp4_opt_hdr_t* qse_dhcp4_find_option (
	const qse_dhcp4_pktinf_t* pkt,
	int                       code
);

QSE_EXPORT qse_uint8_t* qse_dhcp4_get_relay_suboption (
	const qse_uint8_t* ptr, 
	qse_uint8_t        len,
	int                code,
	qse_uint8_t*       olen
);

#ifdef __cplusplus
}
#endif
#endif
