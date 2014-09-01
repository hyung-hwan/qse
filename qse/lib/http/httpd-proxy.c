/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */


#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/hton.h>

typedef struct task_proxy_arg_t task_proxy_arg_t;
struct task_proxy_arg_t 
{
	const qse_httpd_rsrc_proxy_t* rsrc;
	qse_htre_t* req;
};

typedef struct task_proxy_t task_proxy_t;
struct task_proxy_t
{
#define PROXY_INIT_FAILED          (1 << 0)
#define PROXY_RAW                  (1 << 1)
#define PROXY_TRANSPARENT          (1 << 2)
#define PROXY_DNS_SERVER           (1 << 3) /* dns server address specified */
#define PROXY_URS_SERVER           (1 << 4) /* urs server address specified */
#define PROXY_OUTBAND_PEER_NAME    (1 << 5) /* the peer_name pointer points to
                                               a separate memory chunk outside
                                               the task_proxy_t chunk. explicit
                                               deallocatin is required */
#define PROXY_RESOLVE_PEER_NAME    (1 << 6)
#define PROXY_PEER_NAME_RESOLVED   (1 << 7)
#define PROXY_PEER_NAME_UNRESOLVED (1 << 8)
#define PROXY_REWRITE_URL          (1 << 9)
#define PROXY_URL_PREREWRITTEN     (1 << 10) /* URL has been prerewritten in prerewrite(). */
#define PROXY_URL_REWRITTEN        (1 << 11)
#define PROXY_URL_REDIRECTED       (1 << 12)
#define PROXY_X_FORWARDED_FOR      (1 << 13) /* X-Forwarded-For added */
#define PROXY_VIA                  (1 << 14) /* Via added to the request */
#define PROXY_VIA_RETURNING        (1 << 15) /* Via added to the response */
	int flags;
	qse_httpd_t* httpd;
	qse_httpd_client_t* client;

	int method;
	qse_http_version_t version;
	int keepalive; /* taken from the request */

	qse_httpd_task_t* task;
	qse_mchar_t* url_to_rewrite;
	qse_size_t qpath_pos_in_reqfwdbuf;
	qse_size_t qpath_len_in_reqfwdbuf;

	qse_httpd_natr_t dns_server;
	qse_httpd_natr_t urs_server;
	qse_mchar_t* pseudonym;
	qse_htrd_t* peer_htrd;

	qse_mchar_t* peer_name;
	qse_uint16_t peer_port;

	qse_httpd_peer_t peer;
#define PROXY_PEER_OPEN      (1 << 0)
#define PROXY_PEER_CONNECTED (1 << 1)
	int peer_status;

#define PROXY_REQ_FWDERR     (1 << 0)
#define PROXY_REQ_FWDCHUNKED (1 << 1)
	int          reqflags;
	qse_htre_t*  req; /* set to original client request associated with this if necessary */
	qse_mbs_t*   reqfwdbuf; /* content from the request */

	qse_mbs_t*   res;
	qse_size_t   res_consumed;
	qse_size_t   res_pending;
#define PROXY_RES_CLIENT_DISCON   (1 << 0)  /* disconnect client after task */
#define PROXY_RES_CLIENT_CHUNK    (1 << 1)  /* chunk chunked output to client */
#define PROXY_RES_PEER_CLOSE      (1 << 2)  /* read peer until close. 
                                             * no chunk nor no size specified */
#define PROXY_RES_PEER_CHUNK      (1 << 4)  /* peer's output is chunked */
#define PROXY_RES_PEER_LENGTH     (1 << 5)  /* peer's output is set with 
                                             * the content-length */
#define PROXY_RES_PEER_LENGTH_FAKE (1 << 6) /* peer_output_length is fake */
#define PROXY_RES_EVER_SENTBACK    (1 << 7) /* any single byte sent back to a client */
#define PROXY_RES_AWAIT_100       (1 << 10) /* waiting for 100 continue */
#define PROXY_RES_AWAIT_RESHDR    (1 << 11) /* waiting for response header */
#define PROXY_RES_AWAIT_RESCON    (1 << 12) /* waiting for response content. 
                                             * used iif PROXY_RES_CLIENT_CHUNK 
                                             * is on */
#define PROXY_RES_RECEIVED_100    (1 << 13) /* got 100 continue */
#define PROXY_RES_RECEIVED_RESHDR (1 << 14) /* got response header at least */
#define PROXY_RES_RECEIVED_RESCON (1 << 15) /* finished getting the response 
                                             * content fully. used iif  
                                             * PROXY_RES_CLIENT_CHUNK is on */
	int resflags;

	qse_size_t peer_output_length;
	qse_size_t peer_output_received; 

	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_size_t  buflen;
};

typedef struct proxy_peer_htrd_xtn_t proxy_peer_htrd_xtn_t;
struct proxy_peer_htrd_xtn_t
{
	task_proxy_t* proxy;
	qse_httpd_client_t* client;
	qse_httpd_task_t* task;
};

static void log_proxy_error (task_proxy_t* proxy, const qse_mchar_t* shortmsg)
{
	qse_httpd_act_t msg;
	qse_size_t pos = 0;

	msg.code = QSE_HTTPD_CATCH_MERRMSG;
	pos += qse_mbsxcpy (&msg.u.merrmsg[pos], QSE_COUNTOF(msg.u.merrmsg) - pos, shortmsg);
	pos += qse_nwadtombs (&proxy->peer.nwad, &msg.u.merrmsg[pos], QSE_COUNTOF(msg.u.merrmsg) - pos, QSE_NWADTOMBS_ALL);
	proxy->httpd->opt.rcb.logact (proxy->httpd, &msg);
}

static int proxy_add_header_to_buffer (
	task_proxy_t* proxy, qse_mbs_t* buf, const qse_mchar_t* key, const qse_htre_hdrval_t* val)
{
	QSE_ASSERT (val != QSE_NULL);

	do
	{
		if (qse_mbs_cat (buf, key) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, QSE_MT(": ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, val->ptr) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		val = val->next;
	}
	while (val);

	return 0;
}

static int proxy_add_header_to_buffer_with_extra_data (
	task_proxy_t* proxy, qse_mbs_t* buf, const qse_mchar_t* key, const qse_htre_hdrval_t* val, 
	const qse_mchar_t* fmt, ...)
{
	va_list ap;

	QSE_ASSERT (val != QSE_NULL);

	/* NOTE: append the extra data to each value */
	do
	{
		va_start (ap, fmt);
		if (qse_mbs_cat (buf, key) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, QSE_MT(": ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, val->ptr) == (qse_size_t)-1 ||
		    (fmt && qse_mbs_vfcat (buf, fmt, ap) == (qse_size_t)-1) || 
		    qse_mbs_cat (buf, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			va_end (ap);
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		va_end (ap);
		val = val->next;
	}
	while (val);

	return 0;
}

static int proxy_capture_peer_header (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_proxy_t* proxy = (task_proxy_t*)ctx;

	if (!(proxy->httpd->opt.trait & QSE_HTTPD_PROXYNOVIA) && !(proxy->flags & PROXY_VIA_RETURNING))
	{
		if (qse_mbscasecmp (key, QSE_MT("Via")) == 0)
		{
			qse_mchar_t extra[128];
			const qse_mchar_t* pseudonym;

			proxy->flags |= PROXY_VIA_RETURNING;

			if (proxy->pseudonym[0])
			{
				pseudonym = proxy->pseudonym;
			}
			else
			{
				qse_nwadtombs (&proxy->client->local_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ALL);
				pseudonym = extra;
			}

			return proxy_add_header_to_buffer_with_extra_data (
				proxy, proxy->res, key, val, 
				QSE_MT(", %d.%d %hs (%hs)"), 
				(int)proxy->version.major,
				(int)proxy->version.minor, 
				pseudonym,
				qse_httpd_getname(proxy->httpd));
		}
	}


	if (qse_mbscasecmp (key, QSE_MT("Connection")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0)
	{
		return proxy_add_header_to_buffer (proxy, proxy->res, key, val);
	}
	
	return 0;
}

static int proxy_capture_peer_trailer (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_proxy_t* proxy = (task_proxy_t*)ctx;

	if (qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Connection")) != 0)
	{
		return proxy_add_header_to_buffer (proxy, proxy->res, key, val);
	}

	return 0;
}

static int proxy_capture_client_header (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_proxy_t* proxy = (task_proxy_t*)ctx;

	if (!(proxy->flags & (PROXY_TRANSPARENT | PROXY_X_FORWARDED_FOR)))
	{
		if (qse_mbscasecmp (key, QSE_MT("X-Forwarded-For")) == 0)
		{
			/* append to X-Forwarded-For if it exists in the header.
			 * note that it add a comma even if the existing value is empty.
			 * actually, no such value must be sent in by a well-behaving
			 * client/proxy/load-balancer, etc. */
			qse_mchar_t extra[128];

			proxy->flags |= PROXY_X_FORWARDED_FOR;
			qse_nwadtombs (&proxy->client->remote_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ADDR);

			return proxy_add_header_to_buffer_with_extra_data (proxy, proxy->reqfwdbuf, key, val, QSE_MT(", %hs"), extra);
		}
	}

	if (!(proxy->httpd->opt.trait & QSE_HTTPD_PROXYNOVIA) && !(proxy->flags & PROXY_VIA))
	{
		if (qse_mbscasecmp (key, QSE_MT("Via")) == 0)
		{
			qse_mchar_t extra[128];
			const qse_mchar_t* pseudonym;

			proxy->flags |= PROXY_VIA;
			if (proxy->pseudonym[0])
			{
				pseudonym = proxy->pseudonym;
			}
			else
			{
				qse_nwadtombs (&proxy->client->local_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ALL);
				pseudonym = extra;
			}

			return proxy_add_header_to_buffer_with_extra_data (
				proxy, proxy->reqfwdbuf, key, val, 
				QSE_MT(", %d.%d %hs (%hs)"), 
				(int)proxy->version.major,
				(int)proxy->version.minor, 
				pseudonym,
				qse_httpd_getname(proxy->httpd));
		}
	}

	if (qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0)
	{
		return proxy_add_header_to_buffer (proxy, proxy->reqfwdbuf, key, val);
	}

	return 0;
}

static int proxy_capture_client_trailer (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_proxy_t* proxy = (task_proxy_t*)ctx;

	if (qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Connection")) != 0)
	{
		return proxy_add_header_to_buffer (proxy, proxy->reqfwdbuf, key, val);
	}

	return 0;
}

static int proxy_snatch_client_input_raw (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	/* it is a callback function set to the client-side htrd reader
	 * when the raw proxying is enabled. raw proxying doesn't parse
	 * requests. */

	qse_httpd_task_t* task;
	task_proxy_t* proxy; 

	task = (qse_httpd_task_t*)ctx;
	proxy = (task_proxy_t*)task->ctx;

	/* this function is never called with ptr of QSE_NULL
	 * because this callback is set manually after the request
	 * has been discarded or completed in task_init_proxy() and
	 * qse_htre_completecontent or qse-htre_discardcontent() is 
	 * not called again. Unlinkw proxy_snatch_client_input(), 
	 * it doesn't care about EOF indicated by ptr of QSE_NULL. */
	if (ptr && !(proxy->reqflags & PROXY_REQ_FWDERR))
	{
		if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1)
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
	}

	return 0;
}

static int proxy_snatch_client_input (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	/* it is a callback function set to the client-side htrd reader
	 * when the normal proxying is enabled. normal proxying requires
	 * request parsing. */

	qse_httpd_task_t* task;
	task_proxy_t* proxy; 

	task = (qse_httpd_task_t*)ctx;
	proxy = (task_proxy_t*)task->ctx;

#if 0
if (ptr) qse_printf (QSE_T("!!!PROXY SNATCHING [%.*hs]\n"), len, ptr);
else qse_printf (QSE_T("!!!PROXY SNATCHING DONE\n"));
#endif

	if (ptr == QSE_NULL)
	{
		/*
		 * this callback is called with ptr of QSE_NULL 
		 * when the request is completed or discarded. 
		 * and this indicates that there's nothing more to read
		 * from the client side. this can happen when the end of
		 * a request is seen or when an error occurs 
		 */
		QSE_ASSERT (len == 0);

		if (proxy->reqflags & PROXY_REQ_FWDCHUNKED)
		{
			/* add the 0-sized chunk and trailers */
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("0\r\n")) == (qse_size_t)-1 ||
			    qse_htre_walktrailers (req, proxy_capture_client_trailer, proxy) <= -1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1)
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		/* mark the there's nothing to read form the client side */
		qse_htre_unsetconcb (proxy->req);
		proxy->req = QSE_NULL; 

		/* since there is no more to read from the client side.
		 * the relay trigger is not needed any more. */
		task->trigger.cmask &= ~QSE_HTTPD_TASK_TRIGGER_READ;

		if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0 && 
		    (proxy->peer_status & PROXY_PEER_CONNECTED) &&
		    !(task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
		{
			/* there's nothing more to read from the client side.
			 * there's something to forward in the forwarding buffer.
			 * but no write trigger is set. add the write trigger 
			 * for task invocation. */
			task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		}
	}
	else if (!(proxy->reqflags & PROXY_REQ_FWDERR))
	{
		/* we can write to the peer if a forwarding error 
		 * didn't occur previously. we store data from the client side
		 * to the forwaring buffer only if there's no such previous
		 * error. if an error occurred, we simply drop the data. */
		if (proxy->reqflags & PROXY_REQ_FWDCHUNKED)
		{
			qse_mchar_t buf[64];
			qse_fmtuintmaxtombs (
				buf, QSE_COUNTOF(buf), len, 
				16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 
				-1, QSE_MT('\0'), QSE_NULL);

			if (qse_mbs_cat (proxy->reqfwdbuf, buf) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
			    qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) 
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else
		{
			if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1)
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
#if 0
qse_printf (QSE_T("!!!PROXY SNATCHED [%.*hs]\n"), len, ptr);
#endif
	}

	return 0;
}

static int proxy_snatch_peer_output (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	/* this is a content callback function called by the peer 
	 * response reader (proxy->peer_htrd). */

	qse_httpd_task_t* task;
	task_proxy_t* proxy; 

	task = (qse_httpd_task_t*)ctx;
	proxy = (task_proxy_t*)task->ctx;

	/* this callback is enabled if and only if the output back to
	 * the client should be chunked */
	QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);

	/* TODO: better condition for compaction??? */
	if (proxy->res_pending > 0 && proxy->res_consumed > 0)
	{
		qse_mbs_del (proxy->res, 0, proxy->res_consumed);
		proxy->res_consumed = 0;
	}

	if (ptr == QSE_NULL)
	{
		/* content completed */

		QSE_ASSERT (len == 0);

#if 0
qse_printf (QSE_T("PROXY GOT ALL RESPONSE>>>>>>>\n"));
#endif

		if (qse_mbs_cat (proxy->res, QSE_MT("0\r\n")) == (qse_size_t)-1 ||
		    qse_htre_walktrailers (req, proxy_capture_peer_trailer, proxy) <= -1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		proxy->resflags &= ~PROXY_RES_AWAIT_RESCON;
		proxy->resflags |= PROXY_RES_RECEIVED_RESCON; 
	}
	else
	{
		/* append the peer response content to the response buffer */
		qse_mchar_t buf[64];
		qse_fmtuintmaxtombs (
			buf, QSE_COUNTOF(buf), len, 
			16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 
			-1, QSE_MT('\0'), QSE_NULL);

		if (qse_mbs_cat (proxy->res, buf) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1 ||
		    qse_mbs_ncat (proxy->res, ptr, len) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}
	}

	proxy->res_pending = QSE_MBS_LEN(proxy->res) - proxy->res_consumed;
	return 0;
}

static int proxy_htrd_peek_peer_output (qse_htrd_t* htrd, qse_htre_t* res)
{
	proxy_peer_htrd_xtn_t* xtn;
	task_proxy_t* proxy;
	qse_httpd_t* httpd;

	xtn = (proxy_peer_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	proxy = xtn->proxy;
	httpd = proxy->httpd;

	QSE_ASSERT (!(res->state & QSE_HTRE_DISCARDED));

	if (proxy->resflags & PROXY_RES_RECEIVED_RESHDR)
	{
		/* this peek handler is being called again. 
		 * this can happen if qse_htrd_feed() is fed with
		 * multiple responses in task_main_proxy_2 (). */
		httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	if ((proxy->resflags & PROXY_RES_AWAIT_100) && qse_htre_getscodeval(res) == 100)
	{
		/* TODO: check if the request contained Expect... */

		/* 100 continue */
		proxy->resflags &= ~PROXY_RES_AWAIT_100;
		proxy->resflags |= PROXY_RES_RECEIVED_100;

		if (qse_mbs_cat (proxy->res, qse_htre_getverstr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getscodestr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getsmesg(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) 
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1; 
		}

		/* i don't relay any headers and contents in '100 continue' 
		 * back to the client */
		qse_htre_discardcontent (res); 
	}
	else
	{
		int keepalive;

		/* add initial line and headers to proxy->res */
		proxy->resflags &= ~PROXY_RES_AWAIT_100;
		proxy->resflags &= ~PROXY_RES_AWAIT_RESHDR;
		proxy->resflags |= PROXY_RES_RECEIVED_RESHDR;

		keepalive = proxy->keepalive;
		if (res->attr.flags & QSE_HTRE_ATTR_LENGTH)
		{
			/* the response from the peer is length based */
			proxy->resflags |= PROXY_RES_PEER_LENGTH;
			proxy->peer_output_length = res->attr.content_length;
		}
		else if (res->state & QSE_HTRE_COMPLETED)
		{
			/* the response from the peer is chunked or
			 * should be read until disconnection.
			 * but the whold response has already been
			 * received. so i dont' have to do complex 
			 * chunking or something when returning the 
			 * response back to the client. */
			proxy->resflags |= PROXY_RES_PEER_LENGTH | PROXY_RES_PEER_LENGTH_FAKE;
			proxy->peer_output_length = qse_htre_getcontentlen(res); 
		}
		else
		{
			if (qse_comparehttpversions (&proxy->version, &qse_http_v11) >= 0)
			{
				/* client supports chunking */

				/* chunk response when writing back to client */
				proxy->resflags |= PROXY_RES_CLIENT_CHUNK;

				if (res->attr.flags & QSE_HTRE_ATTR_CHUNKED)
				{
					/* mark the peer output is chunked */
					proxy->resflags |= PROXY_RES_PEER_CHUNK;
				}
				else
				{
					/* no chunk, no size. i should read
					 * the peer's output until it closes connection */
					proxy->resflags |= PROXY_RES_PEER_CLOSE;
				}
			}
			else
			{
				/* client doesn't support chunking */
				keepalive = 0;

				/* mark that the connection to client should be closed */
				proxy->resflags |= PROXY_RES_CLIENT_DISCON; 
				/* and push the actual disconnection task */
				if (qse_httpd_entaskdisconnect (httpd, xtn->client, xtn->task) == QSE_NULL) return -1;

				if (res->attr.flags & QSE_HTRE_ATTR_CHUNKED)
					proxy->resflags |= PROXY_RES_PEER_CHUNK;
				else
					proxy->resflags |= PROXY_RES_PEER_CLOSE;
			}
		}

		/* begin initial line */
		if (proxy->resflags & PROXY_RES_CLIENT_CHUNK &&
		    qse_comparehttpversions (&res->version, &qse_http_v11) < 0)
		{
			qse_mchar_t major[32], minor[32];

			qse_fmtuintmaxtombs (major, QSE_COUNTOF(major), proxy->version.major, 10, -1, QSE_MT('\0'), QSE_NULL);
			qse_fmtuintmaxtombs (minor, QSE_COUNTOF(minor), proxy->version.minor, 10, -1, QSE_MT('\0'), QSE_NULL);

			if (qse_mbs_cat (proxy->res, QSE_MT("HTTP/")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->res, major) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->res, QSE_MT(".")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->res, minor) == (qse_size_t)-1)
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else
		{
			if (qse_mbs_cat (proxy->res, qse_htre_getverstr(res)) == (qse_size_t)-1) 
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		if (qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getscodestr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getsmesg(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1; 
		}
		/* end initial line */


		if (proxy->resflags & PROXY_RES_PEER_LENGTH_FAKE)
		{
			qse_mchar_t buf[64];

			/* length should be added by force.
			 * let me add Content-Length event if it's 0 
			 * for less code */
			qse_fmtuintmaxtombs (
				buf, QSE_COUNTOF(buf),
				proxy->peer_output_length, 
				10, -1, QSE_MT('\0'), QSE_NULL);

			if (qse_mbs_cat (proxy->res, QSE_MT("Content-Length: ")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->res, buf) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1)
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
		{
			if (qse_mbs_cat (proxy->res, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1) 
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		if (qse_mbs_cat (proxy->res, (keepalive? QSE_MT("Connection: keep-alive\r\n"): QSE_MT("Connection: close\r\n"))) == (qse_size_t)-1) 
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		if (qse_htre_walkheaders (res, proxy_capture_peer_header, proxy) <= -1) return -1;

		if (!(httpd->opt.trait & QSE_HTTPD_PROXYNOVIA) && !(proxy->flags & PROXY_VIA_RETURNING) && qse_htre_getscodeval(res) != 100)
		{
			/* add the Via: header into the response if it is not 100. */
			qse_size_t tmp;
			qse_mchar_t extra[128];
			const qse_mchar_t* pseudonym;
			qse_http_version_t v;

			proxy->flags |= PROXY_VIA_RETURNING;

			v = *qse_htre_getversion(res);
			if (proxy->pseudonym[0])
			{
				pseudonym = proxy->pseudonym;
			}
			else
			{
				qse_nwadtombs (&proxy->client->local_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ALL);
				pseudonym = extra;
			}

			tmp = qse_mbs_fcat (
				proxy->res, QSE_MT("Via: %d.%d %hs (%hs)\r\n"), 
				(int)v.major, (int)v.minor,
				pseudonym, qse_httpd_getname(httpd));
			if (tmp == (qse_size_t)-1) 
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1; 
			}
		}
		/* end of headers */

		if (qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1; 

		/* content body begins here */
		proxy->peer_output_received = qse_htre_getcontentlen(res);
		if ((proxy->resflags & PROXY_RES_PEER_LENGTH) && 
		    proxy->peer_output_received > proxy->peer_output_length)
		{
			if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
				log_proxy_error (proxy, "proxy redundant output - ");

			httpd->errnum = QSE_HTTPD_EINVAL; /* TODO: change it to a better error code */
			return -1;
		}

		if (proxy->peer_output_received > 0)
		{
			/* the initial part of the content body has been received 
			 * along with the header. it needs to be added to the result 
			 * buffer. */
			if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
			{
				qse_mchar_t buf[64];
				qse_fmtuintmaxtombs (
					buf, QSE_COUNTOF(buf),
					proxy->peer_output_received, 
					16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 
					-1, QSE_MT('\0'), QSE_NULL);

				if (qse_mbs_cat (proxy->res, buf) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1 ||
				    qse_mbs_ncat (proxy->res, qse_htre_getcontentptr(res), qse_htre_getcontentlen(res)) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
				{
					httpd->errnum = QSE_HTTPD_ENOMEM;
					return -1;
				}
			}
			else
			{
				if (qse_mbs_ncat (proxy->res, qse_htre_getcontentptr(res), qse_htre_getcontentlen(res)) == (qse_size_t)-1) 
				{
					httpd->errnum = QSE_HTTPD_ENOMEM;
					return -1;
				}
			}
		}

		if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
		{
			/* arrange to store further contents received to proxy->res */
			qse_htre_setconcb (res, proxy_snatch_peer_output, xtn->task);
		}
	}

	proxy->res_pending = QSE_MBS_LEN(proxy->res) - proxy->res_consumed;
	return 0;
}

static int proxy_htrd_handle_peer_output (qse_htrd_t* htrd, qse_htre_t* req)
{
	/* finished reading response from the peer */
	return 0;
}

static qse_htrd_recbs_t proxy_peer_htrd_cbs =
{
	proxy_htrd_peek_peer_output,
	proxy_htrd_handle_peer_output
};

static void proxy_forward_client_input_to_peer (qse_httpd_t* httpd, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	QSE_ASSERT (proxy->reqfwdbuf != QSE_NULL);

	if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
	{
		/* there is something to forward in the forwarding buffer. */

		if (proxy->reqflags & PROXY_REQ_FWDERR)
		{
			/* a forwarding error has occurred previously.
			 * clear the forwarding buffer */
			qse_mbs_clear (proxy->reqfwdbuf);
		}
		else
		{
			/* normal forwarding */
			qse_ssize_t n;

#if 0
qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@@@WRITING[%.*hs]\n"),
	(int)QSE_MBS_LEN(proxy->reqfwdbuf),
	QSE_MBS_PTR(proxy->reqfwdbuf));
#endif
			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->opt.scb.peer.send (
				httpd, &proxy->peer,
				QSE_MBS_PTR(proxy->reqfwdbuf),
				QSE_MBS_LEN(proxy->reqfwdbuf)
			);

			if (n <= -1)
			{
				if (httpd->errnum != QSE_HTTPD_EAGAIN)
				{
					if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_proxy_error (proxy, "proxy send-to-peer error - ");

					proxy->reqflags |= PROXY_REQ_FWDERR;
					qse_mbs_clear (proxy->reqfwdbuf); 
					if (proxy->req) 
					{
						qse_htre_discardcontent (proxy->req);
						/* NOTE: proxy->req may be set to QSE_NULL
						 *       in proxy_snatch_client_input() triggered by
						 *       qse_htre_discardcontent() */
					}

					task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE; /* peer */
				}
			}
			else if (n > 0) 
			{
/* TODO: improve performance.. instead of copying the remaining part 
to the head all the time..  grow the buffer to a certain limit. */
				qse_mbs_del (proxy->reqfwdbuf, 0, n);
				if (QSE_MBS_LEN(proxy->reqfwdbuf) <= 0)
				{
					if (proxy->req == QSE_NULL) goto done;
					else task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}
		}
	}
	else if (proxy->req == QSE_NULL)
	{
	done:
		/* there is nothing to read from the client side and
		 * there is nothing more to forward in the forwarding buffer.
		 * clear the read and write triggers.
		 */
		task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE; /* peer */
		task->trigger.cmask &= ~QSE_HTTPD_TASK_TRIGGER_READ; /* client-side */
	}
}

/* ------------------------------------------------------------------------ */

static void adjust_peer_name_and_port (task_proxy_t* proxy)
{
	qse_mchar_t* colon;
	colon = qse_mbschr (proxy->peer_name, QSE_MT(':'));
	if (colon) 
	{
		/* handle a port number after the colon sign */
		*colon = QSE_MT('\0');
		proxy->peer_port = qse_mbstoui (colon + 1, 10);
		/* TODO: check if there is a garbage after the port number.
		 *       check if the port number has overflown */
	}
	else
	{
		if (proxy->flags & PROXY_RAW) proxy->peer_port = QSE_HTTPD_DEFAULT_SECURE_PORT;
		else proxy->peer_port = QSE_HTTPD_DEFAULT_PORT;
	}
}

static int task_init_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy;
	task_proxy_arg_t* arg;
	qse_size_t len;
	const qse_mchar_t* ptr;

	proxy = (task_proxy_t*)qse_httpd_gettaskxtn (httpd, task);
	arg = (task_proxy_arg_t*)task->ctx;

	QSE_MEMSET (proxy, 0, QSE_SIZEOF(*proxy));
	proxy->httpd = httpd;
	proxy->client = client;

	proxy->method = qse_htre_getqmethodtype(arg->req);
	proxy->version = *qse_htre_getversion(arg->req);
	proxy->keepalive = (arg->req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);

	proxy->task = task; /* needed for url rewriting */

	proxy->pseudonym = (qse_mchar_t*)(proxy + 1);
	if (arg->rsrc->pseudonym)
	{
		len = qse_mbscpy (proxy->pseudonym, arg->rsrc->pseudonym);
	}
	else
	{
		proxy->pseudonym[0] = QSE_MT('\0');
		len = 0;
	}

	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_RAW) proxy->flags |= PROXY_RAW;
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_TRANSPARENT) proxy->flags |= PROXY_TRANSPARENT;

	proxy->peer.local = arg->rsrc->src.nwad;
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_DST_STR)
	{
		if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_ENABLE_DNS)
		{
			proxy->peer_name = proxy->pseudonym + len + 1;

			qse_mbscpy (proxy->peer_name, arg->rsrc->dst.str);
			adjust_peer_name_and_port (proxy);

			proxy->flags |= PROXY_RESOLVE_PEER_NAME;
			if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_DNS_SERVER)
			{
				/* dns server specified */
				proxy->flags |= PROXY_DNS_SERVER;
				proxy->dns_server = arg->rsrc->dns_server;
			}
		}
		else
		{
			/* dns service is requried to resolve the destination.
			 * but it's not enabled */
			httpd->errnum = QSE_HTTPD_ENODNS;
			goto oops;
		}
	}
	else
	{
		proxy->peer.nwad = arg->rsrc->dst.nwad;
	}

	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_ENABLE_URS)
	{
		int x = httpd->opt.scb.urs.prerewrite (httpd, client, arg->req, arg->rsrc->host, &proxy->url_to_rewrite);
		if (x <= -1) goto oops;

printf (">>>>>>>>>>>>>>>>>>>>>>>> [%s] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", proxy->url_to_rewrite);
		/* enable url rewriting */
		proxy->flags |= PROXY_REWRITE_URL;
		if (x == 0) 
		{
			/* prerewrite() indicates that proxy->url_to_rewrite is the final
			 * rewriting result and no futher rewriting is required */
			proxy->flags |= PROXY_URL_PREREWRITTEN;
		}

		if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_URS_SERVER)
		{
			/* urs server address specified */
			proxy->flags |= PROXY_URS_SERVER;
			proxy->urs_server = arg->rsrc->urs_server;
		}
	}

	proxy->req = QSE_NULL;
/* -------------------------------------------------------------------- 
 * TODO: compose headers to send to peer and push them to fwdbuf... 
 * TODO: also change the content length check logic below...
 * -------------------------------------------------------------------- */

	/* TODO: DETERMINE THIS SIZE */
	len = MAX_SEND_SIZE;

	proxy->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
	if (proxy->reqfwdbuf == QSE_NULL) goto nomem_oops;

	if (proxy->flags & PROXY_RAW)
	{
/* TODO: when connect is attempted, no keep-alive must be hornored. 
 *       when connection fails, it returns failure page  followed by close....  */

		/* the caller must make sure that the actual content is discarded or completed
		 * and the following data is treated as contents */
		QSE_ASSERT (arg->req->state & (QSE_HTRE_DISCARDED | QSE_HTRE_COMPLETED));
		QSE_ASSERT (qse_htrd_getoption(client->htrd) & QSE_HTRD_DUMMY);

		proxy->req = arg->req;
		qse_htre_setconcb (proxy->req, proxy_snatch_client_input_raw, task);
	}
	else
	{
		int snatch_needed = 0;

		/* compose a request to send to the peer using the request
		 * received from the client */

		if (qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqmethodname(arg->req)) == (qse_size_t)-1 ||
			qse_mbs_cat (proxy->reqfwdbuf, QSE_MT(" ")) == (qse_size_t)-1) goto nomem_oops;
		
		proxy->qpath_pos_in_reqfwdbuf = QSE_STR_LEN(proxy->reqfwdbuf);
		if (qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqpath(arg->req)) == (qse_size_t)-1) goto nomem_oops;
		proxy->qpath_len_in_reqfwdbuf = QSE_STR_LEN(proxy->reqfwdbuf) - proxy->qpath_pos_in_reqfwdbuf;

		if (qse_htre_getqparam(arg->req))
		{
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("?")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqparam(arg->req)) == (qse_size_t)-1) goto nomem_oops;
		}

		if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getverstr(arg->req)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
		    qse_htre_walkheaders (arg->req, proxy_capture_client_header, proxy) <= -1) goto nomem_oops;

		if (!(proxy->flags & (PROXY_TRANSPARENT | PROXY_X_FORWARDED_FOR)))
		{
			/* X-Forwarded-For is not added by proxy_capture_client_header() 
			 * above. I don't care if it's included in the trailer. */
			qse_mchar_t extra[128];

			proxy->flags |= PROXY_X_FORWARDED_FOR;

			qse_nwadtombs (&proxy->client->remote_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ADDR);

			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("X-Forwarded-For: ")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, extra) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;
		}

		proxy->resflags |= PROXY_RES_AWAIT_RESHDR;
		if ((arg->req->attr.flags & QSE_HTRE_ATTR_EXPECT100) &&
		    (arg->req->version.major > 1 || (arg->req->version.major == 1 && arg->req->version.minor >= 1)))
		{
			proxy->resflags |= PROXY_RES_AWAIT_100;
		}

		if (!(httpd->opt.trait & QSE_HTTPD_PROXYNOVIA) && !(proxy->flags & PROXY_VIA))
		{
			/* add the Via: header into the request */
			qse_size_t tmp;
			qse_mchar_t extra[128];
			const qse_mchar_t* pseudonym;

			proxy->flags |= PROXY_VIA;

			if (proxy->pseudonym[0])
			{
				pseudonym = proxy->pseudonym;
			}
			else
			{
				qse_nwadtombs (&proxy->client->local_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ALL);
				pseudonym = extra;
			}
			tmp = qse_mbs_fcat (
				proxy->reqfwdbuf, QSE_MT("Via: %d.%d %hs (%hs)\r\n"), 
				(int)proxy->version.major, (int)proxy->version.minor, 
				pseudonym, qse_httpd_getname(httpd));
			if (tmp == (qse_size_t)-1) goto nomem_oops;
		}

		if (arg->req->state & QSE_HTRE_DISCARDED)
		{
			/* no content to add */
			if ((arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH) || 
			    (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
			{
				/* i don't add chunk traiers if the 
				 * request content has been discarded */
				if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Content-Length: 0\r\n\r\n")) == (qse_size_t)-1) goto nomem_oops;
			}
			else
			{
				if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;
			}
		}
		else if (arg->req->state & QSE_HTRE_COMPLETED)
		{
			if (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED)
			{
				/* add trailers if any */
				if (qse_htre_walktrailers (arg->req, proxy_capture_client_trailer, proxy) <= -1) goto nomem_oops;
			}

			len = qse_htre_getcontentlen(arg->req);
			if (len > 0 || (arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH) || 
			               (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
			{
				qse_mchar_t buf[64];

				qse_fmtuintmaxtombs (
					buf, QSE_COUNTOF(buf), len,
					10, -1, QSE_MT('\0'), QSE_NULL);

				/* force-insert content-length. content-length is added
				 * even if the original request dones't contain it */
				if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Content-Length: ")) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->reqfwdbuf, buf) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) goto nomem_oops;

				if (len > 0)
				{
					/* content */
					ptr = qse_htre_getcontentptr(arg->req);
					if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1) goto nomem_oops;
				}
			}
			else
			{
				if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;
			}
		}
		else if (arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH)
		{
			/* the Content-Length header field is contained in the request. */
			qse_mchar_t buf[64];
			qse_fmtuintmaxtombs (
				buf, QSE_COUNTOF(buf),
				arg->req->attr.content_length, 
				10, -1, QSE_MT('\0'), QSE_NULL);

			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Content-Length: ")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, buf) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) goto nomem_oops;

			len = qse_htre_getcontentlen(arg->req);
			if (len > 0)
			{
				/* content received so far */
				ptr = qse_htre_getcontentptr(arg->req);
				if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1) goto nomem_oops;
			}

			snatch_needed = 1;
		}
		else
		{
			/* if this request is not chunked nor not length based,
			 * the state should be QSE_HTRE_COMPLETED. so only a
			 * chunked request should reach here */
			QSE_ASSERT (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED);

			proxy->reqflags |= PROXY_REQ_FWDCHUNKED;
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 /* end of header */) goto nomem_oops; 

			len = qse_htre_getcontentlen(arg->req);
			if (len > 0)
			{
				qse_mchar_t buf[64];

				qse_fmtuintmaxtombs (
					buf, QSE_COUNTOF(buf), len,
					16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 
					-1, QSE_MT('\0'), QSE_NULL);

				ptr = qse_htre_getcontentptr(arg->req);

				/* chunk length and chunk content */
				if (qse_mbs_cat (proxy->reqfwdbuf, buf) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
				    qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1 ||
				    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;
			}

			snatch_needed = 1;
		}

		if (snatch_needed)
		{
			/* set up a callback to be called when the request content
			 * is fed to the htrd reader. qse_htre_addcontent() that 
			 * htrd calls invokes this callback. */
			proxy->req = arg->req;
			qse_htre_setconcb (proxy->req, proxy_snatch_client_input, task);
		}
	}

	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */
#if 0
qse_printf (QSE_T("GOING TO PROXY [%hs]\n"), QSE_MBS_PTR(proxy->reqfwdbuf));
#endif
	task->ctx = proxy;
	return 0;

nomem_oops:
	httpd->errnum = QSE_HTTPD_ENOMEM;
	/* goto oops */

oops:

printf ("init_proxy failed...........................................\n");
	
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_proxy()
	 * add an error task */

	if (proxy->url_to_rewrite)
	{
		qse_httpd_freemem (httpd, proxy->url_to_rewrite);
		proxy->url_to_rewrite = QSE_NULL;
		proxy->flags &= ~PROXY_REWRITE_URL;
	}

	if (proxy->reqfwdbuf)
	{
		qse_mbs_close (proxy->reqfwdbuf);
		proxy->reqfwdbuf = QSE_NULL;
	}

	proxy->flags |= PROXY_INIT_FAILED;
	task->ctx = proxy;

	
	return 0;
}

/* ------------------------------------------------------------------------ */

static void task_fini_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	if (proxy->peer_status & PROXY_PEER_OPEN) 
		httpd->opt.scb.peer.close (httpd, &proxy->peer);

	if (proxy->res) qse_mbs_close (proxy->res);
	if (proxy->peer_htrd) qse_htrd_close (proxy->peer_htrd);
	if (proxy->reqfwdbuf) qse_mbs_close (proxy->reqfwdbuf);
	if (proxy->req) qse_htre_unsetconcb (proxy->req);
	if (proxy->url_to_rewrite) qse_httpd_freemem (httpd, proxy->url_to_rewrite);
	if (proxy->flags & PROXY_OUTBAND_PEER_NAME) qse_httpd_freemem (httpd, proxy->peer_name);
}

/* ------------------------------------------------------------------------ */

static int task_main_proxy_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;

#if 0
printf ("task_main_proxy_5 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n", 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

#if 0
	if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		/* if the client side is readable */
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		/* if the peer side is writable while the client side is not readable*/
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}
#endif
	proxy_forward_client_input_to_peer (httpd, task);

	if (/*(task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITABLE) && */ proxy->buflen > 0)
	{
		/* wrote to the client socket as long as there's something to
		 * write. it's safe to do so as the socket is non-blocking. 
		 * i commented out the check in the 'if' condition above */

		/* TODO: check if proxy outputs more than content-length if it is set... */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				/* can't return internal server error any more... */
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy send-to-client error - ");
				return -1;
			}
		}
		else if (n > 0)
		{
			QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
			proxy->buflen -= n;
		}
	}

	/* if forwarding didn't finish, something is not really right... 
	 * so long as the output from CGI is finished, no more forwarding
	 * is performed */
	return (proxy->buflen > 0 || proxy->req ||
	        (proxy->reqfwdbuf && QSE_MBS_LEN(proxy->reqfwdbuf) > 0))? 1: 0;
}

static int task_main_proxy_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	
#if 0
printf ("task_main_proxy_4 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n", 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

	proxy_forward_client_input_to_peer (httpd, task);
/*
	if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}
*/

	if ((task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE) &&
	    proxy->buflen < QSE_SIZEOF(proxy->buf))
	{
		qse_ssize_t n;

		/* reading from the peer */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.peer.recv (
			httpd, &proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy recv-from-peer error - ");
				return -1;
			}

			/* carry on as if recv was't called at all */
		}
		else if (n == 0)
		{
			/* peer closed connection */
			if (proxy->resflags & PROXY_RES_PEER_LENGTH) 
			{
				QSE_ASSERT (!(proxy->flags & PROXY_RAW));

				if (proxy->peer_output_received < proxy->peer_output_length)
				{
					if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_proxy_error (proxy, "proxy premature eof(content) - ");
					return -1;
				}
			}

			task->main = task_main_proxy_5;

			/* nothing to read from peer. set the mask to 0 */
			task->trigger.v[0].mask = 0;
		
			/* arrange to be called if the client side is writable */
			task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;

			if (proxy->flags & PROXY_RAW)
			{
				/* peer connection has been closed.
				 * so no more forwarding from the client to the peer 
				 * is possible. get rid of the content callback on the
				 * client side. */
				qse_htre_unsetconcb (proxy->req);
				proxy->req = QSE_NULL; 
			}

			return 1;
		}
		else
		{
			proxy->buflen += n;
			proxy->peer_output_received += n;
	
			if (proxy->resflags & PROXY_RES_PEER_LENGTH) 
			{
				QSE_ASSERT (!(proxy->flags & PROXY_RAW));

				if (proxy->peer_output_received > proxy->peer_output_length)
				{
					/* proxy returning too much data... something is wrong in PROXY */
					if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_proxy_error (proxy, "proxy redundant output - ");
					return -1;
				}
				else if (proxy->peer_output_received == proxy->peer_output_length)
				{
					/* proxy has finished reading all */
					task->main = task_main_proxy_5;
					task->trigger.v[0].mask = 0;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}
			}
		}
	}

	if (proxy->buflen)
	{
		/* the main loop invokes the task function only if the client 
		 * side is writable. it should be safe to write whenever
		 * this task function is called. even if it's not writable,
		 * it should still be ok as the client socket is non-blocking. */
		qse_ssize_t n;

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				/* can't return internal server error any more... */
				if (httpd->opt.trait & QSE_HTTPD_LOGACT)
					log_proxy_error (proxy, "proxy send-to-client error - ");
				return -1;
			}
		}
		else if (n > 0)
		{
			QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
			proxy->buflen -= n;
		}
	}

	return 1;
}

static int task_main_proxy_3 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* let's send up the http initial line and headers before
	 * attempting to read the reset of content. it may already 
	 * include some contents as well received together with 
	 * the header. */

	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

#if  0
qse_printf (QSE_T("task_main_proxy_3 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

	proxy_forward_client_input_to_peer (httpd, task);
/*
	if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}
*/

	if (/*(task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITABLE) &&*/ proxy->res_pending > 0)
	{
		/* the client socket is non-blocking. so attempt to send
		 * so long as there's something to send regardless of writability 
		 * of the client socket. see the check commented out in the 'if'
		 * condition above.*/

		qse_ssize_t n;
		qse_size_t count;

		count = proxy->res_pending;
		if (count > MAX_SEND_SIZE) count = MAX_SEND_SIZE;

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (
			httpd, client, 
			&QSE_MBS_CHAR(proxy->res,proxy->res_consumed), 
			count
		);
		if (n <= -1) 
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy send-to-client error - ");
				return -1;
			}
		}
		else if (n > 0)
		{
			proxy->resflags |= PROXY_RES_EVER_SENTBACK;
			proxy->res_consumed += n;
			proxy->res_pending -= n;

			if (proxy->res_pending <= 0)
			{
				/* all data received from the peer so far(including those injected)
				 * have been sent back to the client-side */

				qse_mbs_clear (proxy->res);
				proxy->res_consumed = 0;

				if ((proxy->resflags & PROXY_RES_CLIENT_CHUNK) ||
					((proxy->resflags & PROXY_RES_PEER_LENGTH) && proxy->peer_output_received >= proxy->peer_output_length))
				{
					/* received all contents */
					task->main = task_main_proxy_5;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
				else
				{
					/* there are still more to read from the peer.
					 * arrange to read the remaining contents from the peer */
					task->main = task_main_proxy_4;
					/* nothing to write in proxy->res. so clear WRITE from the
					 * client side */
					task->trigger.cmask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
				return 1;
			}
		}
	}

	return 1; /* more work to do */
}

static int task_main_proxy_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	int http_errnum = 500;

#if 0
printf ("task_main_proxy_2 trigger[0].mask=%d trigger[1].mask=%d cmask=%d\n", 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

	proxy_forward_client_input_to_peer (httpd, task);
#if 0
	if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		/* client is readable */
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		/* client is not readable but peer is writable */
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}
#endif

	if (/*(task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITABLE) && */ proxy->res_pending > 0)
	{
		/* the 'if' condition becomes true only if '100 Continue'
		 * is received without an actual reply in a previous call to 
		 * qse_htrd_feed() far below. Since the actual reply is not
		 * received yet, i just want to read more while relaying 
		 * '100 Continue' to the client. 
		 *
		 * attempt to write to the client regardless of writability of
		 * the cleint socket as it is non-blocking. see the check commented
		 * in the 'if' condition above. */

		qse_ssize_t n;
		qse_size_t count;

		QSE_ASSERT ((proxy->resflags & PROXY_RES_AWAIT_RESHDR) || 
		            (proxy->resflags & PROXY_RES_CLIENT_CHUNK));

		count = proxy->res_pending;
		if (count > MAX_SEND_SIZE) count = MAX_SEND_SIZE;

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (
			httpd, client, 
			QSE_MBS_CPTR(proxy->res,proxy->res_consumed), 
			count
		);
		if (n <= -1) 
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy send-to-client error - ");
				goto oops;
			}
		}
		else if (n > 0)
		{
			proxy->resflags |= PROXY_RES_EVER_SENTBACK;
			proxy->res_consumed += n;
			proxy->res_pending -= n;

			if (proxy->res_pending <= 0)
			{
				/* '100 Continue' and payload received together
				 * has all been relayed back. no need for writability
				 * check of the client side as there's nothing to write.
				 * when something is read from the peer and proxy->res
				 * becomes loaded, this cmask is added with WRITE
				 * in the 'if' block below that takes care of reading
				 * from the peer. */
				task->trigger.cmask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
		}
	}

	if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		qse_ssize_t n;

		/* there is something to read from peer */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.peer.recv (
			httpd, &proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy recv-from-peer error - ");
				goto oops;
			}
		}
		else if (n == 0) 
		{
			if (!(proxy->resflags & PROXY_RES_RECEIVED_RESHDR))
			{
				/* end of output from peer before it has seen a header.
				 * the proxy peer must be bad. */
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy premature eof(header) - ");

				if (!(proxy->resflags & PROXY_RES_RECEIVED_100)) http_errnum = 502;
				goto oops;
			}
			else 
			{
				QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);

				if (proxy->resflags & PROXY_RES_PEER_CLOSE)
				{
					/* i should stop the reader manually since the 
					 * end of content is indicated by close in this
					 * case. call qse_htrd_halt() for this. */
					qse_htrd_halt (proxy->peer_htrd);
					task->main = task_main_proxy_3;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}

				/* premature eof from the peer */
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy no content(chunked) - ");
				goto oops;
			}
		}
		else 
		{
			proxy->buflen += n;
		}

#if 0
qse_printf (QSE_T("#####PROXY FEEDING %d [\n"), (int)proxy->buflen);
{
int i;
for (i = 0; i < proxy->buflen; i++) qse_printf (QSE_T("%hc"), proxy->buf[i]);
}
qse_printf (QSE_T("]\n"));
#endif
		if (proxy->buflen > 0)
		{
			if (qse_htrd_feed (proxy->peer_htrd, proxy->buf, proxy->buflen) <= -1)
			{
				if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_proxy_error (proxy, "proxy feed error - ");
				goto oops;
			}

			proxy->buflen = 0;
		}

		if (QSE_MBS_LEN(proxy->res) > 0)
		{
			if (proxy->resflags & PROXY_RES_RECEIVED_RESCON)
			{
				/* received the contents in full */
				QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);
				task->main = task_main_proxy_3;
				task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else if (proxy->resflags & PROXY_RES_AWAIT_RESCON)
			{
				/* waiting for contents */
				QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);
				task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else if (proxy->resflags & PROXY_RES_RECEIVED_RESHDR)
			{
				/* the actual response header has been received 
				 * with or without '100 continue'. you can
				 * check it with proxy->resflags & PROXY_RES_RECEIVED_100 */

				if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
				{
					proxy->resflags |= PROXY_RES_AWAIT_RESCON;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
				else
				{
#if 0
qse_printf (QSE_T("TRAILING DATA=%d, [%hs]\n"), (int)QSE_MBS_LEN(proxy->res), QSE_MBS_CPTR(proxy->res,proxy->res_consumed));
#endif
					/* switch to the next phase */
					task->main = task_main_proxy_3;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}
			else if (proxy->resflags & PROXY_RES_RECEIVED_100) 
			{
				/* 100 continue has been received but 
				 * the actual response has not. */
				task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else
			{
				/* anything to do? */
			}
		}
	}

	/* complete headers not seen yet. i need to be called again */
	return 1;

oops:
	if (proxy->resflags & PROXY_RES_EVER_SENTBACK) return -1;
	return (qse_httpd_entask_err (httpd, client, task, http_errnum, proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_proxy_1 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* scheduling of this function is made in task_main_proxy() if
	 * the connection to the peer isn't established. this function should
	 * check the connection state to the peer. */

	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	int http_errnum = 500;

	/* wait for peer to get connected */
	if (task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE ||
	    task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		int n;

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.peer.connected (httpd, &proxy->peer);
		if (n <= -1) 
		{
			/* TODO: translate more error codes to http error codes... */

			if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
			else if (httpd->errnum == QSE_HTTPD_EACCES || 
			         httpd->errnum == QSE_HTTPD_ECONN) http_errnum = 403;

			if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
				log_proxy_error (proxy, "proxy connect error - ");

			goto oops;
		}

		if (n >= 1) 
		{
			/* connected to the peer now */
			proxy->peer_status |= PROXY_PEER_CONNECTED;
			if (proxy->req)
			{
				/* need to read from the client-side as
				 * the content has not been received in full. */
				task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_READ;
			}

			task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
			{
				/* forward the initial part of the input to the peer */
				proxy_forward_client_input_to_peer (httpd, task);
				if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
				{
					/* there are still more to forward in the buffer
					 * request the task invocation when the peer
					 * is writable */
					task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}

			if (proxy->flags & PROXY_RAW)
			{
				/* inject http response */
				if (qse_mbs_fmt (proxy->res, QSE_MT("HTTP/%d.%d 200 Connection established\r\n\r\n"), 
				                 (int)proxy->version.major, (int)proxy->version.minor) == (qse_size_t)-1) 
				{
					proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
					goto oops;
				}
				proxy->res_pending = QSE_MBS_LEN(proxy->res) - proxy->res_consumed;

				/* arrange to be called if the client side is writable.
				 * it must write the injected response. */
				task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				task->main  = task_main_proxy_3;
			}
			else
			{
				task->main = task_main_proxy_2;
			}
		}
	}

	return 1;

oops:
	return (qse_httpd_entask_err (httpd, client, task, http_errnum, proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static void on_peer_name_resolved (qse_httpd_t* httpd, const qse_mchar_t* name, const qse_nwad_t* nwad, void* ctx)
{
	qse_httpd_task_t* task = (qse_httpd_task_t*)ctx;
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	QSE_ASSERT (proxy->flags & PROXY_RESOLVE_PEER_NAME);
	QSE_ASSERT (!(proxy->flags & (PROXY_PEER_NAME_RESOLVED | PROXY_PEER_NAME_UNRESOLVED)));

	proxy->flags &= ~PROXY_RESOLVE_PEER_NAME;

	if (nwad)
	{
		/* resolved successfully */
		
		proxy->peer.nwad = *nwad;
		qse_setnwadport (&proxy->peer.nwad, qse_hton16(proxy->peer_port));

		if (proxy->peer.local.type == QSE_NWAD_NX)
			proxy->peer.local.type = proxy->peer.nwad.type;

		proxy->flags |= PROXY_PEER_NAME_RESOLVED;
	}
	else
	{
		/* resolution failure. */
		proxy->flags |= PROXY_INIT_FAILED | PROXY_PEER_NAME_UNRESOLVED;
	}

	if (qse_httpd_activatetasktrigger (httpd, proxy->client, task) <= -1)
	{
		proxy->flags |= PROXY_INIT_FAILED;
	}

printf ("XXXXXXXXXXXXXXXXXXXXXXXXXX PEER NAME RESOLVED.....\n");
}

static void on_url_rewritten (qse_httpd_t* httpd, const qse_mchar_t* url, const qse_mchar_t* new_url, void* ctx)
{
	qse_httpd_task_t* task = (qse_httpd_task_t*)ctx;
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	if (new_url)
	{
		qse_nwad_t nwad;

		proxy->flags &= ~PROXY_REWRITE_URL;

printf ("XXXXXXXXXXXXXXXXXXXXXXXXXX URL REWRITTEN TO [%s].....\n", new_url);
		if (new_url[0] == QSE_MT('\0'))
		{
			/* no change. carry on */
		}
		else if (qse_mbstonwad (new_url, &nwad) >= 0)
		{
			/* if a network address is returned, change the peer address only */
			/* TODO: prevent proxying to self */
			proxy->peer.nwad = nwad;
			proxy->flags |= PROXY_URL_REWRITTEN;
			proxy->flags &= ~PROXY_RESOLVE_PEER_NAME; /* skip dns */
		}
		else if (new_url[0] >= QSE_MT('0') && new_url[0] <= QSE_MT('9'))
		{
			/* redirection */
			int redir_code = 0;
			qse_httpd_status_reloc_t reloc;
			const qse_mchar_t* nuptr = new_url;
			do
			{
				redir_code = redir_code * 10 + (*nuptr - QSE_MT('0'));
				nuptr++;
			} 
			while (*nuptr >= QSE_MT('0') && *nuptr <= QSE_MT('9'));
			if (*nuptr != QSE_MT(':'))  goto fail;
			if (redir_code != 301 && redir_code != 302 && redir_code != 307) redir_code = 301;
			nuptr++;

			reloc.dst = nuptr;
			reloc.redir = 0; /* don't want to append extra / */

			if (qse_httpd_entask_status (
				httpd, proxy->client, proxy->task, redir_code, &reloc,
				proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL) 
			{
				goto fail;
			}

			proxy->flags |= PROXY_URL_REDIRECTED;
		}
		else
		{
			if (proxy->flags & PROXY_RAW)
			{
				qse_mchar_t* tmp;

				QSE_ASSERT (QSE_STR_LEN(proxy->reqfwdbuf) == 0);

				tmp = qse_mbsdup (new_url, qse_httpd_getmmgr(httpd));
				if (tmp == QSE_NULL) 
				{
					qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
					goto fail;
				}

				proxy->flags |= PROXY_RESOLVE_PEER_NAME | PROXY_OUTBAND_PEER_NAME;
				proxy->peer_name = tmp;
				adjust_peer_name_and_port (proxy);
			}
			else
			{
				QSE_ASSERT (QSE_STR_LEN(proxy->reqfwdbuf) > 0);

				/* TODO: Host rewriting?? */
			/* TODO: Host rewriting - to support it, headers must be made available thru request cloning. 
			 *                        the request may not be valid after task_init_proxy */

				if (qse_mbszcasecmp (new_url, QSE_MT("http://"), 7) == 0)
				{
					const qse_mchar_t* host;

					host = new_url + 7;
					if (host[0] != QSE_MT('/') && host[0] != QSE_MT('\0'))
					{
						const qse_mchar_t* slash;
						qse_mchar_t* tmp;

						slash = qse_mbschr (host, QSE_MT('/'));
						if (slash) 
						{
							tmp = qse_mbsxdup (host, slash - host, qse_httpd_getmmgr(httpd));
							new_url = slash;
						}
						else
						{
							tmp = qse_mbsdup (host, qse_httpd_getmmgr(httpd));
							new_url = QSE_MT("/");
						}

						if (tmp == QSE_NULL)
						{
							qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
							goto fail;
						}

						proxy->flags |= PROXY_RESOLVE_PEER_NAME | PROXY_OUTBAND_PEER_NAME;
						proxy->peer_name = tmp;
						adjust_peer_name_and_port (proxy);
					}
				}

				if (qse_mbs_amend (proxy->reqfwdbuf, proxy->qpath_pos_in_reqfwdbuf, proxy->qpath_len_in_reqfwdbuf, new_url) == (qse_size_t)-1) 
				{
					qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
					goto fail;
				}
			}

			proxy->flags |= PROXY_URL_REWRITTEN;
		}
	}
	else
	{
	fail:
		/* url rewriting failed */
		proxy->flags |= PROXY_INIT_FAILED;
	}

	if (qse_httpd_activatetasktrigger (httpd, proxy->client, task) <= -1)
	{
		proxy->flags |= PROXY_INIT_FAILED;
	}

printf ("XXXXXXXXXXXXXXXXXXXXXXXXXX URL REWRITTEN ....\n");
}

static int task_main_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	proxy_peer_htrd_xtn_t* xtn;
	int http_errnum = 500;
	int n;

	if (proxy->flags & PROXY_INIT_FAILED) 
	{
		if (proxy->flags & PROXY_PEER_NAME_UNRESOLVED) http_errnum = 404; /* 404 Not Found */
		goto oops;
	}

	if (proxy->flags & PROXY_URL_REDIRECTED) return 0; /* URL redirected. task finished */ 

	if (proxy->flags & PROXY_REWRITE_URL)
	{
		if (proxy->flags & PROXY_URL_PREREWRITTEN)
		{
			/* proxy->url_to_rewrite is the final rewritten URL by prerewrite(). 
			 * pass QSE_NULL as the second parameter to on_url_rewritten() to
			 * indicate that the original URL is not known */
			on_url_rewritten (httpd, QSE_NULL, proxy->url_to_rewrite, task);
		}
		else
		{
			/* note that url_to_rewrite is URL + extra information. */
			if (qse_httpd_rewriteurl (httpd, proxy->url_to_rewrite, on_url_rewritten, 
									  ((proxy->flags & PROXY_URS_SERVER)? &proxy->urs_server: QSE_NULL), task) <= -1) goto oops;

			if (proxy->flags & PROXY_INIT_FAILED) goto oops;
			
			if ((proxy->flags & PROXY_REWRITE_URL) && 
			    qse_httpd_inactivatetasktrigger (httpd, client, task) <= -1) goto oops;
		}

		return 1;
	}

	if (proxy->flags & PROXY_RESOLVE_PEER_NAME)
	{
		/* arrange to resolve a host name and return */

		QSE_ASSERT (proxy->peer_name != QSE_NULL);

		if (qse_httpd_resolname (httpd, proxy->peer_name, on_peer_name_resolved, 
		                         ((proxy->flags & PROXY_DNS_SERVER)? &proxy->dns_server: QSE_NULL), task) <= -1) goto oops;

		/* if the name could be resolved without sending a request 
		 * in qse_httpd_resolname(), on_peer_name_resolve would be 
		 * called. */
		if (proxy->flags & PROXY_INIT_FAILED) 
		{
			if (proxy->flags & PROXY_PEER_NAME_UNRESOLVED) http_errnum = 404; /* 404 Not Found */
			goto oops;
		}

		/* peer name is not resolved yet. */
		if (!(proxy->flags & PROXY_PEER_NAME_RESOLVED) && 
		    qse_httpd_inactivatetasktrigger (httpd, client, task) <= -1) goto oops;

		return 1;
	}

	if (!(proxy->flags & PROXY_RAW))
	{
		/* set up a http reader to read a response from the peer */
		proxy->peer_htrd = qse_htrd_open (
			httpd->mmgr, QSE_SIZEOF(proxy_peer_htrd_xtn_t));
		if (proxy->peer_htrd == QSE_NULL) goto oops;
		xtn = (proxy_peer_htrd_xtn_t*) qse_htrd_getxtn (proxy->peer_htrd);
		xtn->proxy = proxy;
		xtn->client = client;
		xtn->task = task;
		qse_htrd_setrecbs (proxy->peer_htrd, &proxy_peer_htrd_cbs);
		qse_htrd_setoption (proxy->peer_htrd, QSE_HTRD_RESPONSE | QSE_HTRD_TRAILERS);
	}

	proxy->res = qse_mbs_open (httpd->mmgr, 0, 256);
	if (proxy->res == QSE_NULL) goto oops;
	proxy->res_consumed = 0;
	proxy->res_pending = 0;

	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->opt.scb.peer.open (httpd, &proxy->peer);
	if (n <= -1)
	{
		/* TODO: translate more error codes to http error codes... */
		if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
		else if (httpd->errnum == QSE_HTTPD_EACCES ||
		         httpd->errnum == QSE_HTTPD_ECONN) http_errnum = 403;

		if (httpd->opt.trait & QSE_HTTPD_LOGACT) 
			log_proxy_error (proxy, "proxy connect error - ");
		goto oops;
	}

	proxy->peer_status |= PROXY_PEER_OPEN;
	task->trigger.v[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger.v[0].handle = proxy->peer.handle;
	/*task->trigger.cmask = QSE_HTTPD_TASK_TRIGGER_READ;*/
	task->trigger.cmask = 0;

	if (n == 0)
	{
		/* peer not connected yet */
		/*task->trigger.cmask &= ~QSE_HTTPD_TASK_TRIGGER_READ;*/
		task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		task->main = task_main_proxy_1;
	}
	else
	{
		/* peer connected already */
		proxy->peer_status |= PROXY_PEER_CONNECTED;
		if (proxy->req)
		{
			/* need to read from the client-side as
			 * the content has not been received in full. */
			task->trigger.cmask = QSE_HTTPD_TASK_TRIGGER_READ;
		}

		if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
		{
			proxy_forward_client_input_to_peer (httpd, task);
			if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
			{
				task->trigger.v[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
		}

		if (proxy->flags & PROXY_RAW)
		{
			/* inject http response */
			if (qse_mbs_fmt (proxy->res, QSE_MT("HTTP/%d.%d 200 Connection established\r\n\r\n"), 
			                 (int)proxy->version.major, (int)proxy->version.minor) == (qse_size_t)-1) 
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				goto oops;
			}
			proxy->res_pending = QSE_MBS_LEN(proxy->res) - proxy->res_consumed;

			/* arrange to be called if the client side is writable.
			 * it must write the injected response. */
			task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			task->main = task_main_proxy_3;
		}
		else
		{
			task->main = task_main_proxy_2;
		}
	}

	return 1;

oops:
	if (proxy->res) 
	{
		qse_mbs_close (proxy->res);
		proxy->res = QSE_NULL;
	}
	if (proxy->peer_htrd) 
	{
		qse_htrd_close (proxy->peer_htrd);
		proxy->peer_htrd = QSE_NULL;
	}

	return (qse_httpd_entask_err (
		httpd, client, task, http_errnum, 
		proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

/* ------------------------------------------------------------------------ */

qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_httpd_rsrc_proxy_t* proxy,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_proxy_arg_t arg;
	qse_size_t xtnsize = 0;

	arg.rsrc = proxy;
	arg.req = req;

	if (proxy->pseudonym)
		xtnsize += qse_mbslen (proxy->pseudonym) + 1;
	else
		xtnsize += 1;

	if (proxy->flags & QSE_HTTPD_RSRC_PROXY_DST_STR)
		xtnsize += qse_mbslen (proxy->dst.str) + 1;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_proxy;
	task.fini = task_fini_proxy;
	task.main = task_main_proxy;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(task_proxy_t) + xtnsize
	);
}

