/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#define PROXY_INIT_FAILED          (1u << 0)
#define PROXY_RAW                  (1u << 1)
#define PROXY_TRANSPARENT          (1u << 2)
#define PROXY_DNS_SERVER           (1u << 3) /* dns server address specified */
#define PROXY_URS_SERVER           (1u << 4) /* urs server address specified */
#define PROXY_OUTBAND_PEER_NAME    (1u << 5) /* the peer_name pointer points to
                                               a separate memory chunk outside
                                               the task_proxy_t chunk. explicit
                                               deallocatin is required */
#define PROXY_RESOLVE_PEER_NAME    (1u << 6)
#define PROXY_PEER_NAME_RESOLVING  (1u << 7)
#define PROXY_PEER_NAME_RESOLVED   (1u << 8)
#define PROXY_PEER_NAME_UNRESOLVED (1u << 9)
#define PROXY_REWRITE_URL          (1u << 10)
#define PROXY_URL_REWRITING        (1u << 11)
#define PROXY_URL_PREREWRITTEN     (1u << 12) /* URL has been prerewritten in prerewrite(). */
#define PROXY_URL_REWRITTEN        (1u << 13)
#define PROXY_URL_REDIRECTED       (1u << 14)
#define PROXY_X_FORWARDED          (1u << 15) /* Add X-Forwarded-For and X-Forwarded-Proto */
#define PROXY_VIA                  (1u << 16) /* Via: added to the request */
#define PROXY_VIA_RETURNING        (1u << 17) /* Via: added to the response */
#define PROXY_ALLOW_UPGRADE        (1u << 18)
#define PROXY_UPGRADE_REQUESTED    (1u << 19)
#define PROXY_PROTOCOL_SWITCHED    (1u << 20)
#define PROXY_GOT_BAD_REQUEST      (1u << 21)
	unsigned int flags;
	qse_httpd_t* httpd;
	qse_httpd_client_t* client;

	int method;
	qse_http_version_t version;
	int keepalive; /* taken from the request */

	qse_httpd_task_t* task;
	qse_mchar_t* url_to_rewrite;
	qse_size_t qpath_pos_in_reqfwdbuf; /* position where qpath begins */
	qse_size_t qpath_len_in_reqfwdbuf; /* length of qpath + qparams */

	qse_httpd_dns_server_t dns_server;
	qse_httpd_urs_server_t urs_server;
	qse_mchar_t* pseudonym;
	qse_htrd_t* peer_htrd;

	qse_httpd_mod_t* dns_preresolve_mod;
	qse_mchar_t* peer_name;
	qse_uint16_t peer_port;

	qse_httpd_peer_t* peer; /* it points to static_peer initially. it can get changed to something else */
	qse_httpd_peer_t static_peer;
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

/* ----------------------------------------------------------------- */

#if defined(QSE_HTTPD_DEBUG)
	#define DBGOUT_PROXY_ERROR(proxy, msg) \
	do { \
		qse_mchar_t tmp1[128], tmp2[128]; \
		qse_nwadtombs (&(proxy)->peer->nwad, tmp1, QSE_COUNTOF(tmp1), QSE_NWADTOMBS_ALL); \
		qse_nwadtombs (&(proxy)->client->remote_addr, tmp2, QSE_COUNTOF(tmp2), QSE_NWADTOMBS_ALL); \
		HTTPD_DBGOUT3 ("Proxy error with peer [%hs] client [%hs] - %hs\n", tmp1, tmp2, msg); \
	} while(0)
#else
	#define DBGOUT_PROXY_ERROR(proxy, msg)
#endif

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

#if 0
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
#endif

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

/* EXPERIMENTAL: REMOVE HEADERS.
 * FOR EXAMPLE, You can remove Referer or forge it to give analysis systems harder time  */
	if (qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Proxy-Connection")) != 0 /* EXPERIMENTAL */ /* &&
	    qse_mbscasecmp (key, QSE_MT("Referer")) != 0*/)
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
	    qse_mbscasecmp (key, QSE_MT("Connection")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Proxy-Connection")) != 0)
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
	 * qse_htre_completecontent() or qse_htre_discardcontent() is 
	 * not called again. Unlike proxy_snatch_client_input(), 
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
		/* content completed. got the entire response */

		QSE_ASSERT (len == 0);

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
		if (res->flags & QSE_HTRE_ATTR_LENGTH)
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

				if (res->flags & QSE_HTRE_ATTR_CHUNKED)
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

				if (res->flags & QSE_HTRE_ATTR_CHUNKED)
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
			/* length should be added by force.
			 * let me add Content-Length event if it's 0 
			 * for less code */
			if (qse_mbs_cat (proxy->res, QSE_MT("Content-Length: ")) == (qse_size_t)-1 ||
			    qse_mbs_fcat (proxy->res, QSE_MT("%zu"), (qse_size_t)proxy->peer_output_length) == (qse_size_t)-1 ||
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

		if ((proxy->flags & PROXY_UPGRADE_REQUESTED) && qse_htre_getscodeval(res) == 101) 
		{
			if (qse_mbs_cat (proxy->res, QSE_MT("Connection: Upgrade\r\n")) == (qse_size_t)-1)
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else
		{
			if (qse_mbs_cat (proxy->res, (keepalive? QSE_MT("Connection: keep-alive\r\n"): QSE_MT("Connection: close\r\n"))) == (qse_size_t)-1) 
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
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
			DBGOUT_PROXY_ERROR (proxy, "Redundant output from peer");
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

		if (proxy->flags & PROXY_UPGRADE_REQUESTED)
		{
			QSE_ASSERT (proxy->req != QSE_NULL);

			if (qse_htre_getscodeval(res) == 101) 
			{
				if (proxy->resflags & PROXY_RES_PEER_LENGTH) 
				{
					/* the response to 'Upgrade' must not contain contents */
					if (proxy->peer_output_length > 0) goto no_upgrade;
					else proxy->resflags &= ~PROXY_RES_PEER_LENGTH;
				}

				/* Unlike raw proxying entasked for CONNECT for which disconnection
				 * is supposed to be scheduled by the caller, protocol upgrade
				 * can be requested over a normal http stream. A stream whose 
				 * protocol has been switched must not be sustained after the
				 * task is over. */
				if (qse_httpd_entaskdisconnect (httpd, proxy->client, xtn->task) == QSE_NULL) return -1;
				proxy->flags |= PROXY_PROTOCOL_SWITCHED;
			}
			else
			{
			no_upgrade:
				/* the update request is not granted. restore the reader
				 * to the original state so that HTTP packets can be handled
				 * later on. */
				qse_htrd_undummify (proxy->client->htrd);
				qse_htre_unsetconcb (proxy->req);
				proxy->req = QSE_NULL;
			}

			/* let the reader accept data to be fed */
			qse_htrd_resume (proxy->client->htrd);

			/*task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;*/ /* peer */
			proxy->task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_READ; /* client-side */
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

			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->opt.scb.peer.send (
				httpd, proxy->peer,
				QSE_MBS_PTR(proxy->reqfwdbuf),
				QSE_MBS_LEN(proxy->reqfwdbuf)
			);

			if (n <= -1)
			{
				if (httpd->errnum != QSE_HTTPD_EAGAIN)
				{
					DBGOUT_PROXY_ERROR (proxy, "Send failure to peer");

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
					/* the forwarding buffer is emptied after sending. */
					if (!proxy->req) goto done;
					task->trigger.v[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
					
				}
			}
		}
	}
	else if (!proxy->req)
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
		else 
		{
			if (proxy->peer->flags & QSE_HTTPD_PEER_SECURE)
				proxy->peer_port = QSE_HTTPD_DEFAULT_SECURE_PORT;
			else
				proxy->peer_port = QSE_HTTPD_DEFAULT_PORT;
		}
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
	proxy->keepalive = (arg->req->flags & QSE_HTRE_ATTR_KEEPALIVE);

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
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_X_FORWARDED) proxy->flags |= PROXY_X_FORWARDED;
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_ALLOW_UPGRADE) proxy->flags |= PROXY_ALLOW_UPGRADE;

	proxy->peer = &proxy->static_peer;
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_DST_SECURE) proxy->peer->flags |= QSE_HTTPD_PEER_SECURE;

	proxy->peer->local = arg->rsrc->src.nwad;
	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_DST_STR)
	{
		/* the destination given is a string.
		 * arrange to make a DNS query in task_main_proxy() */

		if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_ENABLE_DNS)
		{
			/* dns service is enabled. carry on with the arrangement */

			proxy->peer_name = proxy->pseudonym + len + 1;
			qse_mbscpy (proxy->peer_name, arg->rsrc->dst.str);
			adjust_peer_name_and_port (proxy);
			proxy->dns_preresolve_mod = arg->rsrc->dns_preresolve_mod;

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
		proxy->peer->nwad = arg->rsrc->dst.nwad;
	}

	if (arg->rsrc->flags & QSE_HTTPD_RSRC_PROXY_ENABLE_URS)
	{
		int x;

		if (arg->rsrc->urs_prerewrite_mod && arg->rsrc->urs_prerewrite_mod->urs_prerewrite)
			x = arg->rsrc->urs_prerewrite_mod->urs_prerewrite (arg->rsrc->urs_prerewrite_mod, client, arg->req, arg->rsrc->host, &proxy->url_to_rewrite);
		else
			x = httpd->opt.scb.urs.prerewrite (httpd, client, arg->req, arg->rsrc->host, &proxy->url_to_rewrite);
		if (x <= -1) goto oops;

		/* enable url rewriting */
		proxy->flags |= PROXY_REWRITE_URL;
		if (x == 0) 
		{
			/* prerewrite() indicates that proxy->url_to_rewrite is the final
			 * rewriting result and no further rewriting is required */
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
		/*QSE_ASSERT (qse_htrd_getoption(client->htrd) & QSE_HTRD_DUMMY);*/

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
		if (arg->req->flags & QSE_HTRE_QPATH_PERDEC)
		{
			/* the query path has been percent-decoded. get the original qpath*/

		#if 0
			/* percent-encoding back doesn't work all the time because
			 * some characters not encoded in the original request may get
			 * encoded. some picky servers has thrown errors for such requests */
			qse_mchar_t* qpath, * qpath_enc;
			qse_size_t x;

			qpath = qse_htre_getqpath(arg->req);
			qpath_enc = qse_perenchttpstrdup (QSE_PERENCHTTPSTR_KEEP_SLASH, qpath, httpd->mmgr);
			if (qpath_enc == QSE_NULL) goto nomem_oops;
			

			x = qse_mbs_cat (proxy->reqfwdbuf, qpath_enc);
			if (qpath != qpath_enc) QSE_MMGR_FREE (httpd->mmgr, qpath_enc);

			if (x == (qse_size_t)-1) goto nomem_oops;
		#else
			/* using the original query path minimizes the chance of side-effects */
			if (qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getorgqpath(arg->req)) == (qse_size_t)-1) goto nomem_oops;
		#endif
		}
		else
		{
			/* the query path doesn't require encoding or it's never decoded */
			if (qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqpath(arg->req)) == (qse_size_t)-1) goto nomem_oops;
		}

		if (qse_htre_getqparam(arg->req))
		{
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("?")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqparam(arg->req)) == (qse_size_t)-1) goto nomem_oops;
		}

		/* length must include the parameters also */
		proxy->qpath_len_in_reqfwdbuf = QSE_STR_LEN(proxy->reqfwdbuf) - proxy->qpath_pos_in_reqfwdbuf;

#if 0
{
/* EXPERIMENTAL */
/* KT FILTERING WORKAROUND POC. KT seems to check the Host: the first packet
 * only.I add 1500 byte space octets between the URL and the HTTP version string.
 * the header is likely to be placed in the second packet. it seems to work. */
qse_mchar_t spc[1500];
QSE_MEMSET (spc, QSE_MT(' '), QSE_COUNTOF(spc));
qse_mbs_ncat (proxy->reqfwdbuf, spc, QSE_COUNTOF(spc));
}
#endif

		if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getverstr(arg->req)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
		    qse_htre_walkheaders (arg->req, proxy_capture_client_header, proxy) <= -1) goto nomem_oops;

		if ((proxy->flags & (PROXY_TRANSPARENT | PROXY_X_FORWARDED)) == PROXY_X_FORWARDED)
		{
			qse_mchar_t extra[128];

			/* client's ip address */
			qse_nwadtombs (&proxy->client->remote_addr, extra, QSE_COUNTOF(extra), QSE_NWADTOMBS_ADDR);
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("X-Forwarded-For: ")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, extra) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;

			/* client's protocol*/
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("X-Forwarded-Proto: ")) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, ((client->status & QSE_HTTPD_CLIENT_SECURE)? QSE_MT("https"): QSE_MT("http"))) == (qse_size_t)-1 ||
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto nomem_oops;

/* TODO: support the Forwarded header in RFC7239.
 * Forwarded: for=xxx;by=xxx;prot=xxxx */
		}

		proxy->resflags |= PROXY_RES_AWAIT_RESHDR;
		if ((arg->req->flags & QSE_HTRE_ATTR_EXPECT100) &&
		     qse_comparehttpversions (&arg->req->version, &qse_http_v11) >= 0)
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
			if ((arg->req->flags & QSE_HTRE_ATTR_LENGTH) || 
			    (arg->req->flags & QSE_HTRE_ATTR_CHUNKED))
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
			if (arg->req->flags & QSE_HTRE_ATTR_CHUNKED)
			{
				/* add trailers if any */
				if (qse_htre_walktrailers (arg->req, proxy_capture_client_trailer, proxy) <= -1) goto nomem_oops;
			}

			len = qse_htre_getcontentlen(arg->req);
			if (len > 0 || (arg->req->flags & QSE_HTRE_ATTR_LENGTH) || 
			               (arg->req->flags & QSE_HTRE_ATTR_CHUNKED))
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
		else if (arg->req->flags & QSE_HTRE_ATTR_LENGTH)
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
			QSE_ASSERT (arg->req->flags & QSE_HTRE_ATTR_CHUNKED);

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

		if ((proxy->flags & PROXY_ALLOW_UPGRADE) && qse_htre_getheaderval(arg->req, QSE_MT("Upgrade")))
		{
			/* Upgrade: is found in the request header */
			const qse_htre_hdrval_t* hv;
			hv = qse_htre_getheaderval(arg->req, QSE_MT("Connection"));
			while (hv)
			{
				if (qse_mbscaseword (hv->ptr, QSE_MT("Upgrade"), QSE_MT(','))) break;
				hv = hv->next;
			}

			if (!hv) goto no_upgrade;

			if (snatch_needed)
			{
				/* The upgrade request can't have contents.
				 * Not allowing contents makes implementation easier. */
				httpd->errnum = QSE_HTTPD_EBADREQ;
				proxy->flags |= PROXY_GOT_BAD_REQUEST;
				goto oops;
			}

			/* cause feeding of client data to fail. i do this because
			 * it's unknown if upgrade will get granted or not. 
			 * if it's granted, the client input should be treated
			 * as an octet string. If not, it should still be handled
			 * as HTTP. */
			qse_htrd_suspend (client->htrd);

			/* prearrange to not parse client input when feeding is resumed */
			qse_htrd_dummify (client->htrd);

			proxy->flags |= PROXY_UPGRADE_REQUESTED;
			proxy->req = arg->req;

			/* prearrange to capature client input when feeding is resumed */
			qse_htre_setconcb (proxy->req, proxy_snatch_client_input_raw, proxy->task);
		}
		else 
		{
		no_upgrade:
			if (snatch_needed)
			{
				/* set up a callback to be called when the request content
				 * is fed to the htrd reader. qse_htre_addcontent() that 
				 * htrd calls invokes this callback. */
				proxy->req = arg->req;
				qse_htre_setconcb (proxy->req, proxy_snatch_client_input, task);
			}
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
	{
		int reuse = 0;

		/* check if the peer connection can be reused */
		if (!(proxy->flags & (PROXY_RAW | PROXY_UPGRADE_REQUESTED | PROXY_PROTOCOL_SWITCHED)) &&
		    !(proxy->resflags & PROXY_RES_PEER_CLOSE))
		{
			qse_mchar_t tmpch;

			/* check if the peer connection dropped connection or
			 * sending excessive data. don't reuse such a connection */
			if (httpd->opt.scb.peer.recv (httpd, proxy->peer, &tmpch, 1) <= -1 &&
			    httpd->errnum == QSE_HTTPD_EAGAIN) reuse = 1;
		}

		if (reuse && qse_httpd_cacheproxypeer (httpd, client, proxy->peer))
		{
			/* cache a reusable peer connection */

		#if defined(QSE_HTTPD_DEBUG)
			qse_mchar_t tmp[128];

			qse_nwadtombs (&proxy->peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
			HTTPD_DBGOUT2 ("Cached peer [%hs] - %zd\n", tmp, (qse_size_t)proxy->peer->handle);
		#endif
		}
		else
		{
		#if defined(QSE_HTTPD_DEBUG)
			qse_mchar_t tmp[128];

			qse_nwadtombs (&proxy->peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
			HTTPD_DBGOUT2 ("Closing peer [%hs] - %zd\n", tmp, (qse_size_t)proxy->peer->handle);
		#endif

			httpd->opt.scb.peer.close (httpd, proxy->peer);

			if (proxy->peer->flags & QSE_HTTPD_PEER_CACHED)
			{
				QSE_ASSERT (proxy->peer != &proxy->static_peer);
				qse_httpd_freemem (httpd, proxy->peer);
			}
		}
	}

	if ((proxy->flags & (PROXY_UPGRADE_REQUESTED | PROXY_PROTOCOL_SWITCHED)) == PROXY_UPGRADE_REQUESTED)
	{
		/* upgrade requested but protocol switching not completed yet.
		 * this can happen because dummification is performed before
		 * the 101 response is received. */

		/* no harm to call this multiple times */
		qse_htrd_undummify (proxy->client->htrd); 
	}

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

	proxy_forward_client_input_to_peer (httpd, task);

	if (/*(task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITABLE) && */ proxy->buflen > 0)
	{
		/* write to the client socket as long as there's something.
		 * it's safe to do so as the socket is non-blocking. 
		 * i commented out the check in the 'if' condition above */

		/* TODO: check if proxy outputs more than content-length if it is set... */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				/* can't return internal server error any more... */
				DBGOUT_PROXY_ERROR (proxy, "Send failrue to client");
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
	 * so long as the output from peer is finished, no more forwarding
	 * is performed */
	return (proxy->buflen > 0 || proxy->req ||
	        (proxy->reqfwdbuf && QSE_MBS_LEN(proxy->reqfwdbuf) > 0))? 1: 0;
}

static int task_main_proxy_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;

#if 0
printf ("task_main_proxy_4 trigger[0].mask=%d trigger[1].mask=%d trigger.cmask=%d\n", 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

	proxy_forward_client_input_to_peer (httpd, task);

	if ((task->trigger.v[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE) &&
	    proxy->buflen < QSE_SIZEOF(proxy->buf))
	{
	reread:
		/* reading from the peer */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.peer.recv (
			httpd, proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				DBGOUT_PROXY_ERROR (proxy, "Recv failure from peer");
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
					DBGOUT_PROXY_ERROR (proxy, "Premature content end");
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
			else if (proxy->flags & PROXY_PROTOCOL_SWITCHED)
			{
				qse_htrd_undummify (proxy->client->htrd); 
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
					DBGOUT_PROXY_ERROR (proxy, "Redundant output from peer");
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

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				/* can't return internal server error any more... */
				DBGOUT_PROXY_ERROR (proxy, "Send failure to client");
				return -1;
			}
		}
		else if (n > 0)
		{
			QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
			proxy->buflen -= n;
		}
	}

	if (proxy->peer->flags & QSE_HTTPD_PEER_PENDING)
	{
		/* this QSE_HTTPD_CLIENT_PENDING thing is a dirty hack for SSL.
		 * In SSL, data is transmitted in a record. a record can be
		 * as large as 16K bytes since its length field is 2 bytes.
		 * If SSL_read() has a record but it's given a smaller buffer
		 * than the actual record, the next call to select() won't return. 
		 * there is no data to read at the socket layer. SSL_pending() can 
		 * tell you the amount of data in the SSL buffer. I try to consume
		 * the pending data if the client.recv handler has set QSE_HTTPD_CLIENT_PENDING. */

		/* BUG BUG BUG.
		 * it jumps back to read more. If the client-side is not writable,
		 * unnecessary loop is made between this 'goto' and the target label. 
		 * HOW SHOULD I SOLVE THIS? USE A BIG BUFFER AS LARGE AS 16K? */
		/*if (proxy->buflen < QSE_SIZEOF(proxy->buf))*/ goto reread;
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

#if 0
printf ("task_main_proxy_3 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n", 
	task->trigger.v[0].mask, task->trigger.v[1].mask, task->trigger.cmask);
#endif

	proxy_forward_client_input_to_peer (httpd, task);

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
				DBGOUT_PROXY_ERROR (proxy, "Send failure to client");
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

				if (proxy->flags & PROXY_PROTOCOL_SWITCHED) 
				{
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_READ;
					goto read_more;
				}
			
				if ((proxy->resflags & PROXY_RES_CLIENT_CHUNK) ||
					((proxy->resflags & PROXY_RES_PEER_LENGTH) && proxy->peer_output_received >= proxy->peer_output_length))
				{
					/* received all contents */
					task->main = task_main_proxy_5;
					task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
				else
				{
				read_more:
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

	proxy_forward_client_input_to_peer (httpd, task);

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
		 * out in the 'if' condition above. */

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
				DBGOUT_PROXY_ERROR (proxy, "Send failure to client");
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
			httpd, proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			if (httpd->errnum != QSE_HTTPD_EAGAIN)
			{
				DBGOUT_PROXY_ERROR (proxy, "Recv failure from peer");
				goto oops;
			}
		}
		else if (n == 0) 
		{
			if (!(proxy->resflags & PROXY_RES_RECEIVED_RESHDR))
			{
				/* end of output from peer before it has seen a header.
				 * the proxy peer must be bad. */
				DBGOUT_PROXY_ERROR (proxy, "Premature header end from peer");
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
				DBGOUT_PROXY_ERROR (proxy, "No chunked content from peer");
				goto oops;
			}
		}
		else 
		{
			proxy->buflen += n;
		}

		if (proxy->buflen > 0)
		{
			if (qse_htrd_feed (proxy->peer_htrd, proxy->buf, proxy->buflen) <= -1)
			{
			#if defined(QSE_HTTPD_DEBUG)
				HTTPD_DBGOUT3 ("Failed to feed proxy peer response to handler - %d [%.*hs]\n",
					qse_htrd_geterrnum(proxy->peer_htrd), (int)proxy->buflen, proxy->buf);
			#endif
				goto oops;
			}

			proxy->buflen = 0;
		}

		if (proxy->flags & PROXY_PROTOCOL_SWITCHED)
		{
			task->trigger.cmask = QSE_HTTPD_TASK_TRIGGER_READ;
			if (QSE_MBS_LEN(proxy->res) > 0) task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			task->main = task_main_proxy_3;
		}
		else if (QSE_MBS_LEN(proxy->res) > 0)
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
	return (qse_httpd_entaskerrorwithmvk (httpd, client, task, http_errnum, proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
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
		n = httpd->opt.scb.peer.connected (httpd, proxy->peer);
		if (n <= -1) 
		{
			/* TODO: translate more error codes to http error codes... */

			if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
			else if (httpd->errnum == QSE_HTTPD_EACCES || 
			         httpd->errnum == QSE_HTTPD_ECONN) http_errnum = 403;

		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&proxy->peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT1 ("Cannnot connect to peer [%hs]\n", tmp);
			}
		#endif

			goto oops;
		}

		if (n >= 1) 
		{
			/* connected to the peer now */
			proxy->peer_status |= PROXY_PEER_CONNECTED;
			if (!(proxy->flags & PROXY_UPGRADE_REQUESTED) && proxy->req)
			{
				/* need to read from the client-side as
				 * the content has not been received in full. 
				 *
				 * proxy->req is set to the original request when snatching is
				 * required. it's also set to the original request when protocol
				 * upgrade is requested. however, a upgrade request containing
				 * contents is treated as a bad request. so i don't arrange
				 * to read from the client side when PROXY_UPGRADE_REQUESTED
				 * is on. */
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
	return (qse_httpd_entaskerrorwithmvk (httpd, client, task, http_errnum, proxy->method, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static void on_peer_name_resolved (qse_httpd_t* httpd, const qse_mchar_t* name, const qse_nwad_t* nwad, void* ctx)
{
	qse_httpd_task_t* task = (qse_httpd_task_t*)ctx;
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	QSE_ASSERT (proxy->flags & PROXY_RESOLVE_PEER_NAME);
	QSE_ASSERT (proxy->flags & PROXY_PEER_NAME_RESOLVING);
	QSE_ASSERT (!(proxy->flags & (PROXY_PEER_NAME_RESOLVED | PROXY_PEER_NAME_UNRESOLVED)));

	proxy->flags &= ~(PROXY_RESOLVE_PEER_NAME | PROXY_PEER_NAME_RESOLVING);

	if (nwad)
	{
		/* resolved successfully */
		
		proxy->peer->nwad = *nwad;
		qse_setnwadport (&proxy->peer->nwad, qse_hton16(proxy->peer_port));

		if (proxy->peer->local.type == QSE_NWAD_NX)
			proxy->peer->local.type = proxy->peer->nwad.type;

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


#if defined(QSE_HTTPD_DEBUG)
	if (proxy->flags & PROXY_PEER_NAME_RESOLVED)
	{
		qse_mchar_t tmp[128];
		qse_nwadtombs (&proxy->peer->nwad, tmp, 128, QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT2 ("Peer name [%hs] resolved to [%hs]\n", name, tmp);
	}
#endif
}

static void on_url_rewritten (qse_httpd_t* httpd, const qse_mchar_t* url, const qse_mchar_t* new_url, void* ctx)
{
	qse_httpd_task_t* task = (qse_httpd_task_t*)ctx;
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	if (new_url)
	{
		qse_nwad_t nwad;

		proxy->flags &= ~(PROXY_REWRITE_URL | PROXY_URL_REWRITING);

		HTTPD_DBGOUT2 ("URL rewritten from [%hs] to [%hs]\n", url, new_url);
		if (new_url[0] == QSE_MT('\0'))
		{
			/* no change. carry on */
		}
		else if (qse_mbstonwad (new_url, &nwad) >= 0)
		{
			/* if a network address is returned, change the peer address only */
			/* TODO: prevent proxying to self */

			if (qse_getnwadport(&nwad) == 0) 
			{
				/* i don't care if new_url is X.X.X.X:0 or just X.X.X.X */
				qse_setnwadport (&nwad, qse_hton16(QSE_HTTPD_DEFAULT_PORT));
			}

			proxy->peer->nwad = nwad;
			proxy->flags |= PROXY_URL_REWRITTEN;
			proxy->flags &= ~PROXY_RESOLVE_PEER_NAME; /* skip dns */
		}
		else if (new_url[0] >= QSE_MT('0') && new_url[0] <= QSE_MT('9'))
		{
			/* check if it begins with redirection code followed by a colon */
			int redir_code = 0;
			qse_httpd_rsrc_reloc_t reloc;
			const qse_mchar_t* nuptr = new_url;
			do
			{
				redir_code = redir_code * 10 + (*nuptr - QSE_MT('0'));
				nuptr++;
			} 
			while (*nuptr >= QSE_MT('0') && *nuptr <= QSE_MT('9'));
			if (*nuptr != QSE_MT(':'))  
			{
				/* no colon is found after digits. it's probably a normal url */
				goto normal_url;
			}
			if (redir_code != 301 && redir_code != 302 && redir_code != 303 && 
			    redir_code != 307 && redir_code != 308) redir_code = 302;
			nuptr++;

			/* relocation code is given explictly, no slash appending is needed.
			 * use qse_httpd_entask_status() rather than qse_httpd_entaskreloc(). */
			reloc.flags = 0; 
			reloc.target = nuptr;

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
		normal_url:
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
				int proto_len;

				QSE_ASSERT (QSE_STR_LEN(proxy->reqfwdbuf) > 0);

				/* TODO: Host rewriting?? */
			/* TODO: Host rewriting - to support it, headers must be made available thru request cloning. 
			 *                        the request may not be valid after task_init_proxy */

				if (qse_mbszcasecmp (new_url, QSE_MT("http://"), (proto_len = 7)) == 0 ||
				    qse_mbszcasecmp (new_url, QSE_MT("https://"), (proto_len = 8)) == 0)
				{
					const qse_mchar_t* host;

					host = new_url + proto_len;
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

/* TODO: antything todo when http is rewritten to HTTPS or vice versa */
						if (proto_len == 8)
							proxy->peer->flags |= QSE_HTTPD_PEER_SECURE;
						else
							proxy->peer->flags &= ~QSE_HTTPD_PEER_SECURE;

						if (qse_mbstonwad (tmp, &nwad) <= -1)
						{
							proxy->flags |= PROXY_RESOLVE_PEER_NAME | PROXY_OUTBAND_PEER_NAME;
							proxy->peer_name = tmp;
							adjust_peer_name_and_port (proxy);
						}
						else
						{
							if (qse_getnwadport(&nwad) == 0) 
							{
								/* i don't care if tmp is X.X.X.X:0 or just X.X.X.X */
								qse_setnwadport (&nwad, qse_hton16(proto_len == 8? QSE_HTTPD_DEFAULT_SECURE_PORT: QSE_HTTPD_DEFAULT_PORT));
							}

							proxy->peer->nwad = nwad;
							proxy->flags |= PROXY_URL_REWRITTEN;
							proxy->flags &= ~PROXY_RESOLVE_PEER_NAME; /* skip dns */

						#if defined(QSE_HTTPD_DEBUG)
							{
								qse_mchar_t tmp[128];
								qse_nwadtombs (&proxy->peer->nwad, tmp, 128, QSE_NWADTOMBS_ALL);
								HTTPD_DBGOUT4 ("Peer name resolved to [%hs] in url rewriting. new_url [%hs] %d %d\n", 
									tmp, new_url,
									(int)proxy->qpath_pos_in_reqfwdbuf,
									(int)proxy->qpath_len_in_reqfwdbuf
								);
							}
						#endif

							/* the temporary string is not used. kill it */
							qse_httpd_freemem (httpd, tmp);
						}
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
}

static int task_main_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	proxy_peer_htrd_xtn_t* xtn;
	int http_errnum = 500;
	int n;
	qse_httpd_peer_t* peer_from_cache;

	if (proxy->flags & PROXY_INIT_FAILED) 
	{
		if (proxy->flags & PROXY_GOT_BAD_REQUEST) 
		{
			http_errnum = 400; /* 400 Bad Request */
			proxy->keepalive = 0; /* force Connect: close in the response */
		}
		else if (proxy->flags & PROXY_PEER_NAME_UNRESOLVED) http_errnum = 404; /* 404 Not Found */
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
			on_url_rewritten (httpd, QSE_NULL, (proxy->url_to_rewrite? proxy->url_to_rewrite: QSE_MT("")), task);
		}
		else
		{
			if (!(proxy->flags & PROXY_URL_REWRITING))
			{
				/* note that url_to_rewrite is URL + extra information. */
				proxy->flags |= PROXY_URL_REWRITING; /* to prevent double calls */

				if (qse_httpd_rewriteurl (httpd, proxy->url_to_rewrite, on_url_rewritten, 
				                          ((proxy->flags & PROXY_URS_SERVER)? &proxy->urs_server: QSE_NULL), task) <= -1) goto oops;

				if (proxy->flags & PROXY_INIT_FAILED) goto oops;
			
				if ((proxy->flags & PROXY_REWRITE_URL) && 
				    qse_httpd_inactivatetasktrigger (httpd, client, task) <= -1) goto oops;
			}
		}

		return 1; /* not finished yet */
	}

	if (proxy->flags & PROXY_RESOLVE_PEER_NAME)
	{
		/* arrange to resolve a host name and return */
		int x;

		QSE_ASSERT (proxy->peer_name != QSE_NULL);

		if (proxy->flags & PROXY_PEER_NAME_RESOLVING)
		{
			return 1; /* not finished yet */
		}

		if (proxy->dns_preresolve_mod && proxy->dns_preresolve_mod->dns_preresolve)
			x = proxy->dns_preresolve_mod->dns_preresolve (proxy->dns_preresolve_mod, client, proxy->peer_name, &proxy->peer->nwad);
		else
			x = httpd->opt.scb.dns.preresolve (httpd, client, proxy->peer_name, &proxy->peer->nwad);
		if (x <= -1) goto oops;

		if (x == 0)
		{
			/* preresolve() indicates that proxy->peer->nwad contains the
			 * final address. no actual dns resolution is required */
			proxy->flags |= PROXY_PEER_NAME_RESOLVED;
			proxy->flags &= ~PROXY_RESOLVE_PEER_NAME;
			qse_setnwadport (&proxy->peer->nwad, qse_hton16(proxy->peer_port));
		}
		else
		{
			/* this function can be called more than once if a socket 
			 * descriptor appears multiple times in the event result 
			 * of a single event polling cycle in the main loop.
			 * e.g.) the mux implementation doesn't collapse multiple events
			 *       for a socket descriptor into 1 event.
			 * 
			 * if this happens, qse_http_resolvename() can be called 
			 * multiple times. set this flag to prevent double resolution.
			 */
			proxy->flags |= PROXY_PEER_NAME_RESOLVING; 

			x = qse_httpd_resolvename (httpd, proxy->peer_name, on_peer_name_resolved, ((proxy->flags & PROXY_DNS_SERVER)? &proxy->dns_server: QSE_NULL), task);
			if (x <= -1) goto oops;
		}

		/* if the name could be resolved without sending a request 
		 * in qse_httpd_resolvename(), on_peer_name_resolve would be 
		 * called. */
		if (proxy->flags & PROXY_INIT_FAILED) 
		{
			if (proxy->flags & PROXY_PEER_NAME_UNRESOLVED) http_errnum = 404; /* 404 Not Found */
			goto oops;
		}

		/* peer name is not resolved yet. */
		if (!(proxy->flags & PROXY_PEER_NAME_RESOLVED) && 
		    qse_httpd_inactivatetasktrigger (httpd, client, task) <= -1) goto oops;

		return 1; /* not finished yet */
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

	/* get a cached peer connection */
	peer_from_cache = qse_httpd_decacheproxypeer (httpd, client, &proxy->peer->nwad, &proxy->peer->local, (proxy->peer->flags & QSE_HTTPD_PEER_SECURE));
	if (peer_from_cache)
	{
		qse_mchar_t tmpch;

		QSE_ASSERT (peer_from_cache->flags & QSE_HTTPD_PEER_CACHED);

		/* test if the cached connection is still ok */
		if (httpd->opt.scb.peer.recv (httpd, peer_from_cache, &tmpch, 1) <= -1 && 
		    httpd->errnum == QSE_HTTPD_EAGAIN)
		{
			/* this connection seems to be ok. it didn't return EOF nor any data. 
			 * A valid connection can't return data yes as no request has been sent.*/
		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&peer_from_cache->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT2 ("Decached peer [%hs] - %zd\n", tmp, (qse_size_t)peer_from_cache->handle);
			}
		#endif
		}
		else
		{
			/* the cached connection seems to be stale or invalid */
	#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&peer_from_cache->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT2 ("Decached and closed stale peer [%hs] - %zd\n", tmp, (qse_size_t)peer_from_cache->handle);
			}
	#endif
			httpd->opt.scb.peer.close (httpd, peer_from_cache);
			qse_httpd_freemem (httpd, peer_from_cache);

			peer_from_cache = QSE_NULL;
		}
	}

	if (peer_from_cache)
	{
		proxy->peer = peer_from_cache; /* switch the peer pointer to the one acquired from the cache */
		n = 1; /* act as if it just got connected */
	}
	else
	{
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->opt.scb.peer.open (httpd, proxy->peer);
		if (n <= -1)
		{
			/* TODO: translate more error codes to http error codes... */
			if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
			else if (httpd->errnum == QSE_HTTPD_EACCES ||
					 httpd->errnum == QSE_HTTPD_ECONN) http_errnum = 403;

		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&proxy->peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT1 ("Cannnot open peer [%hs]\n", tmp);
			}
		#endif

			goto oops;
		}

	#if defined(QSE_HTTPD_DEBUG)
		{
			qse_mchar_t tmp[128];
			qse_nwadtombs (&proxy->peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
			HTTPD_DBGOUT2 ("Opened peer [%hs] - %zd\n", tmp, (qse_size_t)proxy->peer->handle);
		}
	#endif
	}

	proxy->peer_status |= PROXY_PEER_OPEN;
	task->trigger.v[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger.v[0].handle = proxy->peer->handle;
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
		if (!(proxy->flags & PROXY_UPGRADE_REQUESTED) && proxy->req)
		{
			/* need to read from the client-side as
			 * the content has not been received in full. 
			 *
			 * proxy->req is set to the original request when snatching is
			 * required. it's also set to the original request when protocol
			 * upgrade is requested. however, a upgrade request containing
			 * contents is treated as a bad request. so i don't arrange
			 * to read from the client side when PROXY_UPGRADE_REQUESTED
			 * is on.
			 */
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

	return (qse_httpd_entaskerrorwithmvk (
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

