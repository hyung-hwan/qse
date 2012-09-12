/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
/* UNSUPPORTED YET..  */
/* TODO: IMPLEMENT THIS */
#else

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>

#include <qse/cmn/stdio.h> /* TODO: remove this.*/

typedef struct task_proxy_arg_t task_proxy_arg_t;
struct task_proxy_arg_t 
{
	qse_nwad_t peer_nwad;
	qse_htre_t* req;
	int nph;
};

typedef struct task_proxy_t task_proxy_t;
struct task_proxy_t
{
	int init_failed;
	qse_httpd_t* httpd;

	const qse_mchar_t* host;
	qse_http_version_t version;
	int keepalive; /* taken from the request */

	qse_htrd_t* peer_htrd;

	qse_httpd_peer_t peer;
#define PROXY_PEER_OPEN      (1 << 0)
#define PROXY_PEER_CONNECTED (1 << 1)
	int peer_status;

#define PROXY_REQ_FWDERR     (1 << 0)
#define PROXY_REQ_FWDCHUNKED (1 << 1)
	int          reqflags;
	qse_htre_t*  req; /* original client request associated with this */
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

static int proxy_capture_peer_header (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_proxy_t* proxy = (task_proxy_t*)ctx;

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

static int proxy_snatch_client_input (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	qse_httpd_task_t* task;
	task_proxy_t* proxy; 

	task = (qse_httpd_task_t*)ctx;
	proxy = (task_proxy_t*)task->ctx;

if (ptr) qse_printf (QSE_T("!!!PROXY SNATCHING [%.*hs]\n"), len, ptr);
else qse_printf (QSE_T("!!!PROXY SNATCHING DONE\n"));

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
		task->trigger[2].mask = 0;

		if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0 && 
		    (proxy->peer_status & PROXY_PEER_CONNECTED) &&
		    !(task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
		{
			/* there's nothing more to read from the client side.
			 * there's something to forward in the forwarding buffer.
			 * but no write trigger is set. add the write trigger 
			 * for task invocation. */
			task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
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

		task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
qse_printf (QSE_T("!!!PROXY SNATCHED [%.*hs]\n"), len, ptr);
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

qse_printf (QSE_T("PROXY GOT ALL RESPONSE>>>>>>>\n"));

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

	xtn = (proxy_peer_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	proxy = xtn->proxy;

	QSE_ASSERT (!(res->state & QSE_HTRE_DISCARDED));

	if (proxy->resflags & PROXY_RES_RECEIVED_RESHDR)
	{
		/* this peek handler is being called again. 
		 * this can happen if qse_htrd_feed() is fed with
		 * multiple responses in task_main_proxy_2 (). */
		proxy->httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	if ((proxy->resflags & PROXY_RES_AWAIT_100) && qse_htre_getscodeval(res) == 100)
	{
/* TODO: check if the request contained Expect... */
qse_printf (QSE_T("10000000000000000000000000000 CONTINUE 10000000000000000000000000000000\n"));
		proxy->resflags &= ~PROXY_RES_AWAIT_100;
		proxy->resflags |= PROXY_RES_RECEIVED_100;

		if (qse_mbs_cat (proxy->res, qse_htre_getverstr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getscodestr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getsmesg(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
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

qse_printf (QSE_T("NORMAL REPLY 222222222222222222222 NORMAL REPLY\n"));

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
				if (qse_httpd_entaskdisconnect (proxy->httpd, xtn->client, xtn->task) == QSE_NULL) return -1;

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
			qse_mchar_t vbuf[64];
			snprintf (vbuf, QSE_COUNTOF(vbuf), QSE_MT("HTTP/%d.%d"), 
				(int)proxy->version.major, (int)proxy->version.minor);
			if (qse_mbs_cat (proxy->res, vbuf) == (qse_size_t)-1) 
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else
		{
			if (qse_mbs_cat (proxy->res, qse_htre_getverstr(res)) == (qse_size_t)-1) 
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		if (qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getscodestr(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT(" ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, qse_htre_getsmesg(res)) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
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
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
		{
			if (qse_mbs_cat (proxy->res, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1) 
			{
				proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		if (qse_mbs_cat (proxy->res, (keepalive? QSE_MT("Connection: keep-alive\r\n"): QSE_MT("Connection: close\r\n"))) == (qse_size_t)-1) 
		{
			proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		if (qse_htre_walkheaders (res, proxy_capture_peer_header, proxy) <= -1) return -1;
		/* end of headers */
		if (qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1; 

		/* content body begins here */
		proxy->peer_output_received = qse_htre_getcontentlen(res);
		if ((proxy->resflags & PROXY_RES_PEER_LENGTH) && 
		    proxy->peer_output_received > proxy->peer_output_length)
		{
			/* TODO: logging?? */
qse_printf (QSE_T("PROXY PEER FUCKED - RETURNING TOO MUCH...\n"));
			proxy->httpd->errnum = QSE_HTTPD_EINVAL; /* TODO: change it to a better error code */
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
					proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
					return -1;
				}
			}
			else
			{
				if (qse_mbs_ncat (proxy->res, qse_htre_getcontentptr(res), qse_htre_getcontentlen(res)) == (qse_size_t)-1) 
				{
					proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
					return -1;
				}
			}
		}

		if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
		{
			/* arrange to store further contents received to proxy->res */
			qse_htre_setconcb (res, proxy_snatch_peer_output, xtn->task);
		}
qse_printf (QSE_T("NORMAL REPLY 222222222222222222222 NORMAL REPLY OK\n"));
	}

	proxy->res_pending = QSE_MBS_LEN(proxy->res) - proxy->res_consumed;
	return 0;
}

static int proxy_htrd_handle_peer_output (qse_htrd_t* htrd, qse_htre_t* req)
{
qse_printf (QSE_T("FINISHED READING RESPONSE...\n"));
	return 0;
}

static qse_htrd_recbs_t proxy_peer_htrd_cbs =
{
	proxy_htrd_peek_peer_output,
	proxy_htrd_handle_peer_output
};

static void proxy_forward_client_input_to_peer (
	qse_httpd_t* httpd, qse_httpd_task_t* task, int writable)
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
qse_printf (QSE_T("FORWARD: CLEARING REQCON FOR ERROR\n"));
			qse_mbs_clear (proxy->reqfwdbuf);
		}
		else
		{
			/* normal forwarding */
			qse_ssize_t n;

			if (writable) goto forward;

			n = httpd->scb->mux.writable (httpd, proxy->peer.handle, 0);
if (n == 0) qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@@NOT WRITABLE\n"));
			if (n >= 1)
			{
			forward:
				/* writable */
qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@@@WRITING[%.*hs]\n"),
	(int)QSE_MBS_LEN(proxy->reqfwdbuf),
	QSE_MBS_PTR(proxy->reqfwdbuf));
				n = httpd->scb->peer.send (
					httpd, &proxy->peer,
					QSE_MBS_PTR(proxy->reqfwdbuf),
					QSE_MBS_LEN(proxy->reqfwdbuf)
				);

/* TODO: improve performance.. instead of copying the remaing part 
to the head all the time..  grow the buffer to a certain limit. */
				if (n > 0) 
				{
					qse_mbs_del (proxy->reqfwdbuf, 0, n);
					if (QSE_MBS_LEN(proxy->reqfwdbuf) <= 0)
					{
						if (proxy->req == QSE_NULL) goto done;
						else task->trigger[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
					}
				}
			}

			if (n <= -1)
			{
qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@WRITE TO PROXY FAILED\n"));
/* TODO: logging ... */
				proxy->reqflags |= PROXY_REQ_FWDERR;
				qse_mbs_clear (proxy->reqfwdbuf); 
				if (proxy->req) 
				{
					qse_htre_discardcontent (proxy->req);
					/* NOTE: proxy->req may be set to QSE_NULL
					 *       in proxy_snatch_client_input() triggered by
					 *       qse_htre_discardcontent() */
				}

				task->trigger[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE; /* peer */
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
qse_printf (QSE_T("FORWARD: @@@@@@@@NOTHING MORE TO WRITE TO PROXY\n"));
		task->trigger[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE; /* peer */
		task->trigger[2].mask = 0; /* client-side */
	}
}

static int task_init_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy;
	task_proxy_arg_t* arg;
	qse_size_t len;
	const qse_mchar_t* ptr;
	int snatch_needed;

	proxy = (task_proxy_t*)qse_httpd_gettaskxtn (httpd, task);
	arg = (task_proxy_arg_t*)task->ctx;

	QSE_MEMSET (proxy, 0, QSE_SIZEOF(*proxy));
	proxy->httpd = httpd;

	proxy->version = *qse_htre_getversion(arg->req);
	proxy->keepalive = (arg->req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);
	proxy->peer.nwad = arg->peer_nwad;
	proxy->req = QSE_NULL;

/* -------------------------------------------------------------------- 
 * TODO: compose headers to send to peer and push them to fwdbuf... 
 * TODO: also change the content length check logic below...
 * -------------------------------------------------------------------- */

/* TODO: DETERMINE THIS SIZE */
len = 1024;
	proxy->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
	if (proxy->reqfwdbuf == QSE_NULL) goto oops;

	if (qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqmethodname(arg->req)) == (qse_size_t)-1 ||
	    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT(" ")) == (qse_size_t)-1 ||
	    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqpath(arg->req)) == (qse_size_t)-1) goto oops;

	if (qse_htre_getqparam(arg->req))
	{
		if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("?")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getqparam(arg->req)) == (qse_size_t)-1) goto oops;
	}
	if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT(" ")) == (qse_size_t)-1 ||
	    qse_mbs_cat (proxy->reqfwdbuf, qse_htre_getverstr(arg->req)) == (qse_size_t)-1 ||
	    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
	    qse_htre_walkheaders (arg->req, proxy_capture_client_header, proxy) <= -1) goto oops;

	proxy->resflags |= PROXY_RES_AWAIT_RESHDR;
	if ((arg->req->attr.flags & QSE_HTRE_ATTR_EXPECT100) &&
	    (arg->req->version.major > 1 || 
	     (arg->req->version.major == 1 && arg->req->version.minor >= 1)))
	{
		proxy->resflags |= PROXY_RES_AWAIT_100;
	}

	snatch_needed = 0;
	if (arg->req->state & QSE_HTRE_DISCARDED)
	{
		/* no content to add */
		if ((arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH) || 
		    (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
		{
			/* i don't add chunk traiers if the 
			 * request content has been discarded */
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Content-Length: 0\r\n\r\n")) == (qse_size_t)-1) goto oops;
		}
		else
		{
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto oops;
		}
	}
	else if (arg->req->state & QSE_HTRE_COMPLETED)
	{
		if (arg->req->attr.flags & QSE_HTRE_ATTR_CHUNKED)
		{
			/* add trailers if any */
			if (qse_htre_walktrailers (
				arg->req, proxy_capture_client_trailer, proxy) <= -1) goto oops;
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
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) goto oops;

			if (len > 0)
			{
				/* content */
				ptr = qse_htre_getcontentptr(arg->req);
				if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1) goto oops;
			}
		}
		else
		{
			if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto oops;
		}
	}
	else if (arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH)
	{
		qse_mchar_t buf[64];
		qse_fmtuintmaxtombs (
			buf, QSE_COUNTOF(buf),
			arg->req->attr.content_length, 
			10, -1, QSE_MT('\0'), QSE_NULL);

		if (qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("Content-Length: ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, buf) == (qse_size_t)-1 ||
		    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n\r\n")) == (qse_size_t)-1) goto oops;

		len = qse_htre_getcontentlen(arg->req);
		if (len > 0)
		{
			/* content received so far */
			ptr = qse_htre_getcontentptr(arg->req);
			if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1) goto oops;
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
		    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 /* end of header */) goto oops; 

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
			    qse_mbs_cat (proxy->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) goto oops;
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

	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */
qse_printf (QSE_T("GOING TO PROXY [%hs]\n"), QSE_MBS_PTR(proxy->reqfwdbuf));
	task->ctx = proxy;
	return 0;

oops:
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_proxy()
	 * add an error task */
	if (proxy->reqfwdbuf)
	{
		qse_mbs_close (proxy->reqfwdbuf);
		proxy->reqfwdbuf = QSE_NULL;
	}
	proxy->init_failed = 1;
	task->ctx = proxy;

	proxy->httpd->errnum = QSE_HTTPD_ENOMEM;
	return 0;
}

static void task_fini_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	if (proxy->peer_status & PROXY_PEER_OPEN) 
		httpd->scb->peer.close (httpd, &proxy->peer);

	if (proxy->res) qse_mbs_close (proxy->res);
	if (proxy->peer_htrd) qse_htrd_close (proxy->peer_htrd);
	if (proxy->reqfwdbuf) qse_mbs_close (proxy->reqfwdbuf);
	if (proxy->req) qse_htre_unsetconcb (proxy->req);
}

static int task_main_proxy_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;

	//QSE_ASSERT (proxy->pio_inited);
qse_printf (QSE_T("task_main_proxy_5 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		/* if the client side is readable */
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		/* if the peer side is writable */
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}

	if (!(task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) ||
	    (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		if (proxy->buflen > 0)
		{
/* TODO: check if proxy outputs more than content-length if it is set... */
			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->scb->client.send (httpd, client, proxy->buf, proxy->buflen);
			if (n <= -1)
			{
			/* can't return internal server error any more... */
/* TODO: logging ... */
				return -1;
			}

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
	
	//QSE_ASSERT (proxy->pio_inited);

qse_printf (QSE_T("task_main_proxy_4 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}

qse_printf (QSE_T("task_main_proxy_4 about to read from PEER...\n"));
	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		qse_ssize_t n;

		if (proxy->buflen < QSE_SIZEOF(proxy->buf))
		{
qse_printf (QSE_T("task_main_proxy_4 reading from PEER... %d %d\n"), (int)proxy->peer_output_length, (int)proxy->peer_output_received);
			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->scb->peer.recv (
				httpd, &proxy->peer,
				&proxy->buf[proxy->buflen], 
				QSE_SIZEOF(proxy->buf) - proxy->buflen
			);
qse_printf (QSE_T("task_main_proxy_4 read from PEER...%d\n"), (int)n);
			if (n <= -1)
			{
				/* can't return internal server error any more... */
	/* TODO: loggig ... */
				return -1;
			}
			if (n == 0)
			{
				if (proxy->resflags & PROXY_RES_PEER_LENGTH) 
				{
					if (proxy->peer_output_received < proxy->peer_output_length)
					{
	qse_printf (QSE_T("PROXY FUCKED UP...PEER CLOSING PREMATURELY\n"));
						return -1;
					}
				}	
				
				task->main = task_main_proxy_5;
				task->trigger[0].mask = 0;
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				return 1;
			}

			proxy->buflen += n;
			proxy->peer_output_received += n;
	
			if (proxy->resflags & PROXY_RES_PEER_LENGTH) 
			{
				if (proxy->peer_output_received > proxy->peer_output_length)
				{
	/* TODO: proxy returning too much data... something is wrong in PROXY */
	qse_printf (QSE_T("PROXY FUCKED UP...RETURNING TOO MUCH DATA\n"));
					return -1;
				}
				else if (proxy->peer_output_received == proxy->peer_output_length)
				{
	qse_printf (QSE_T("PROXY DONE READING\n"));
					task->main = task_main_proxy_5;
					task->trigger[0].mask = 0;
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}
			}
		}

		/* the main loop invokes the task function only if the client 
		 * side is writable. it should be safe to write whenever
		 * this task function is called. */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->scb->client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
	/* TODO: logging ... */
	qse_printf (QSE_T("CGI SEND FAILURE\n"));
			return -1;
		}
	
		QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
		proxy->buflen -= n;
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

qse_printf (QSE_T("task_main_proxy_3 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}

	if (!(task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) ||
	    (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		qse_ssize_t n;
		qse_size_t count;

qse_printf (QSE_T("[PROXY-----3 SENDING XXXXX]\n"));
		count = proxy->res_pending;
		if (count > MAX_SEND_SIZE) count = MAX_SEND_SIZE;

		if (count > 0)
		{
qse_printf (QSE_T("[proxy_3 sending %d bytes]\n"), (int)count);
			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->scb->client.send (
				httpd, client, 
				&QSE_MBS_CHAR(proxy->res,proxy->res_consumed), 
				count
			);

			if (n <= -1) 
			{
qse_printf (QSE_T("[proxy-3 send failure....\n"));
				return -1;
			}

			proxy->resflags |= PROXY_RES_EVER_SENTBACK;
			proxy->res_consumed += n;
			proxy->res_pending -= n;
		}

		if (proxy->res_pending <= 0)
		{
			qse_mbs_clear (proxy->res);
			proxy->res_consumed = 0;

			if ((proxy->resflags & PROXY_RES_CLIENT_CHUNK) ||
			    ((proxy->resflags & PROXY_RES_PEER_LENGTH) && proxy->peer_output_received >= proxy->peer_output_length))
			{
qse_printf (QSE_T("SWITINCG TO 55555555555555555555555555 %d %d %d %d\n"), 
	(proxy->resflags & PROXY_RES_CLIENT_CHUNK), (proxy->resflags & PROXY_RES_PEER_LENGTH),
	(int)proxy->peer_output_received, (int)proxy->peer_output_length);
				task->main = task_main_proxy_5;
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else
			{
qse_printf (QSE_T("SWITICHING TO 4444444444444444444444444444\n"));
				task->main = task_main_proxy_4;
				task->trigger[2].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			return 1;
		}
	}

	return 1; /* more work to do */
}

static int task_main_proxy_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	int http_errnum = 500;

qse_printf (QSE_T("task_main_proxy_2 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_client_input_to_peer (httpd, task, 1);
	}

	if (!(task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) ||
	    (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		if (proxy->res_pending > 0)
		{
			/* the 'if' condition becomes true only if '100 Continue'
			 * is received without an actual reply in a previous call to 
			 * qse_htrd_feed() below. Since the actual reply is not
			 * received yet, i just want to read more while realying 
			 * '100 Continue' to the client. this task handler is called
			 * only if the client side handle is writable. i can safely
			 * write to the client without a check. */
	
			qse_ssize_t n;
			qse_size_t count;
	
			QSE_ASSERT ((proxy->resflags & PROXY_RES_AWAIT_RESHDR) || 
			            (proxy->resflags & PROXY_RES_CLIENT_CHUNK));

			count = proxy->res_pending;
			if (count > MAX_SEND_SIZE) count = MAX_SEND_SIZE;

qse_printf (QSE_T("[proxy_2 sending %d bytes (index %d)] ["), 
	(int)count, (int)proxy->res_consumed);
{
int i;
for (i = 0; i < count; i++) qse_printf (QSE_T("%hc"), QSE_MBS_CHAR(proxy->res,proxy->res_consumed+i));
}
qse_printf (QSE_T("]\n"));

			httpd->errnum = QSE_HTTPD_ENOERR;
			n = httpd->scb->client.send (
				httpd, client, 
				QSE_MBS_CPTR(proxy->res,proxy->res_consumed), 
				count
			);
			if (n <= -1) 
			{
qse_printf (QSE_T("[proxy-2 send failure....\n"));
				goto oops;
			}

			proxy->resflags |= PROXY_RES_EVER_SENTBACK;
			proxy->res_consumed += n;
			proxy->res_pending -= n;

			if (proxy->res_pending <= 0)
			{
				/* '100 Continue' and payload received together
				 * has all been relayed back. no need for writability
				 * check of the client side */
				task->trigger[2].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
		}
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		qse_ssize_t n;

		/* there is something to read from peer */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->scb->peer.recv (
			httpd, &proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
/* TODO: logging ... */
			goto oops;
		}
		if (n == 0) 
		{
			if (!(proxy->resflags & PROXY_RES_RECEIVED_RESHDR))
			{
				/* end of output from peer before it has seen a header.
				 * the proxy script must be crooked. */
/* TODO: logging */
qse_printf (QSE_T("#####PREMATURE EOF FROM PEER\n"));
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
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}

qse_printf (QSE_T("#####PREMATURE EOF FROM PEER CLIENT CHUNK\n"));
				goto oops;
			}
		}
			
		proxy->buflen += n;

qse_printf (QSE_T("#####PROXY FEEDING %d [\n"), (int)proxy->buflen);
{
int i;
for (i = 0; i < proxy->buflen; i++) qse_printf (QSE_T("%hc"), proxy->buf[i]);
}
qse_printf (QSE_T("]\n"));

		if (qse_htrd_feed (proxy->peer_htrd, proxy->buf, proxy->buflen) <= -1)
		{
/* TODO: logging */
qse_printf (QSE_T("#####INVALID HEADER FROM PEER [%.*hs]\n"), (int)proxy->buflen, proxy->buf);
			goto oops;
		}

		proxy->buflen = 0;

		if (QSE_MBS_LEN(proxy->res) > 0)
		{
			if (proxy->resflags & PROXY_RES_RECEIVED_RESCON)
			{
				QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);
				task->main = task_main_proxy_3;
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else if (proxy->resflags & PROXY_RES_AWAIT_RESCON)
			{
				QSE_ASSERT (proxy->resflags & PROXY_RES_CLIENT_CHUNK);
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else if (proxy->resflags & PROXY_RES_RECEIVED_RESHDR)
			{
				/* the actual response header has been received 
				 * with or without '100 continue'. you can
				 * check it with proxy->resflags & PROXY_RES_RECEIVED_100 */
				if (proxy->resflags & PROXY_RES_CLIENT_CHUNK)
				{
					proxy->resflags |= PROXY_RES_AWAIT_RESCON;
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
				else
				{
qse_printf (QSE_T("TRAILING DATA=%d, [%hs]\n"), (int)QSE_MBS_LEN(proxy->res), QSE_MBS_CPTR(proxy->res,proxy->res_consumed));
					/* switch to the next phase */
					task->main = task_main_proxy_3;
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}
			else if (proxy->resflags & PROXY_RES_RECEIVED_100) 
			{
				/* 100 continue has been received but 
				 * the actual response has not. */
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
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
	return (qse_httpd_entask_error (httpd, client, task, http_errnum, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_proxy_1 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	int http_errnum = 500;

	/* wait for peer to get connected */
qse_printf (QSE_T("task_main_proxy_1....\n"));

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE ||
	    task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		int n;

		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->scb->peer.connected (httpd, &proxy->peer);
		if (n <= -1) 
		{
			/* improve error conversion */
			if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
			else if (httpd->errnum == QSE_HTTPD_EACCES) http_errnum = 403;
qse_printf (QSE_T("task_main_proxy_1.... ERROR \n"));
			goto oops;
		}

		if (n >= 1) 
		{
			proxy->peer_status |= PROXY_PEER_CONNECTED;
qse_printf (QSE_T("FINALLY connected to peer ...............................\n"));

			if (proxy->req)
			{
				task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
			}

			task->trigger[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
			{
				/* forward the initial part of the input to the peer */
				proxy_forward_client_input_to_peer (httpd, task, 0);
				if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
				{
					/* there are still more to forward in the buffer
					 * request the task invocation when the peer
					 * is writable */
					task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}
			task->main = task_main_proxy_2;
		}
	}

	return 1;

oops:
	return (qse_httpd_entask_error (httpd, client, task, http_errnum, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	proxy_peer_htrd_xtn_t* xtn;
	int http_errnum = 500;
	int n;
qse_printf (QSE_T("task_main_proxy....\n"));

	if (proxy->init_failed) goto oops;

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

	proxy->res = qse_mbs_open (httpd->mmgr, 0, 256);
	if (proxy->res == QSE_NULL) goto oops;
	proxy->res_consumed = 0;
	proxy->res_pending = 0;

	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->scb->peer.open (httpd, &proxy->peer);
	if (n <= -1)
	{
/* TODO: translate error code to http error... */
		if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
		else if (httpd->errnum == QSE_HTTPD_EACCES) http_errnum = 403;
qse_printf (QSE_T("caanot open peer....\n"));
		goto oops;
	}

	proxy->peer_status |= PROXY_PEER_OPEN;
	task->trigger[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger[0].handle = proxy->peer.handle;
	task->trigger[2].handle = client->handle;

	if (n == 0)
	{
		/* peer not connected yet */
		task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		task->main = task_main_proxy_1;
	}
	else
	{
		/* peer connected already */
		proxy->peer_status |= PROXY_PEER_CONNECTED;
		if (proxy->req)
		{
			task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
		}

		if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
		{
			proxy_forward_client_input_to_peer (httpd, task, 0);
			if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
			{
				task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
		}
		task->main = task_main_proxy_2;
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

	return (qse_httpd_entask_error (
		httpd, client, task, http_errnum, 
		&proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_nwad_t* nwad,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_proxy_arg_t arg;

	arg.peer_nwad = *nwad;
	arg.req = req;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_proxy;
	task.fini = task_fini_proxy;
	task.main = task_main_proxy;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(task_proxy_t)
	);
}

#endif
