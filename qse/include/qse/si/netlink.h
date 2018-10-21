#ifndef _QSE_SI_NETLINK_H_
#define _QSE_SI_NETLINK_H_

#include <qse/types.h>
#include <qse/macros.h>

#if defined(__linux)

struct qse_nlmsg_hdr_t 
{
	qse_uint32_t	nlmsg_len;
	qse_uint16_t	nlmsg_type;
	qse_uint16_t	nlmsg_flags;
	qse_uint32_t	nlmsg_seq;
	qse_uint32_t	nlmsg_pid;
};
typedef struct qse_nlmsg_hdr_t qse_nlmsg_hdr_t;


typedef int (*qse_nlenum_cb_t) (
	qse_nlmsg_hdr_t* hdr,
	void*            ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_nlenum (
	int             fd,
	unsigned int    seq,
	int             type,
	int             af,
	qse_nlenum_cb_t cb,
	void*           ctx
);

QSE_EXPORT int qse_nlenum_route (
	int             link_af,
	int             addr_af,
	qse_nlenum_cb_t cb,
	void*           ctx
);

#if defined(__cplusplus)
}
#endif

#endif /* __linux */

#endif
