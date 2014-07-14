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
#include <qse/cmn/pio.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/path.h>

#include <stdio.h> /* TODO: remove this */
#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf 
#endif

typedef struct task_cgi_arg_t task_cgi_arg_t;
struct task_cgi_arg_t 
{
	qse_mcstr_t path;
	qse_mcstr_t script;
	qse_mcstr_t suffix;
	qse_mcstr_t root;
	qse_mcstr_t shebang;
	int nph;
	qse_htre_t* req;
};

typedef struct task_cgi_t task_cgi_t;
struct task_cgi_t
{
	int init_failed;
	qse_httpd_t* httpd;

	const qse_mchar_t* path;
	const qse_mchar_t* script;
	const qse_mchar_t* suffix;
	const qse_mchar_t* root;
	const qse_mchar_t* shebang;

	int method;
	qse_http_version_t version;
	int keepalive; /* taken from the request */
	int nph;

	qse_htrd_t* script_htrd;
	qse_env_t* env;
	qse_pio_t pio;
	int pio_inited;

#define CGI_REQ_GOTALL     (1 << 0)
#define CGI_REQ_FWDALL     (1 << 1)
#define CGI_REQ_FWDERR     (1 << 2)
#define CGI_REQ_FWDCHUNKED (1 << 3)
	int          reqflags;
	qse_htre_t*  req; /* original request associated with this */
	qse_mbs_t*   reqfwdbuf; /* content from the request */

#define CGI_RES_CLIENT_DISCON        (1 << 0)
#define CGI_RES_CLIENT_CHUNK         (1 << 1)
#define CGI_RES_SCRIPT_LENGTH (1 << 2)
	int          resflags;
	qse_mbs_t*   res;
	qse_mchar_t* res_ptr;
	qse_size_t   res_left;	

	/* content-length that CGI returned */
	qse_size_t script_output_length; /* TODO: a script maybe be able to output more than the maximum value of qse_size_t */
	/* the number of octets received from the script */
	qse_size_t script_output_received; 

	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_size_t  buflen;
};

typedef struct cgi_script_htrd_xtn_t cgi_script_htrd_xtn_t;
struct cgi_script_htrd_xtn_t
{
	task_cgi_t* cgi;
	qse_httpd_client_t* client;
	qse_httpd_task_t* task;
};

typedef struct cgi_client_req_hdr_ctx_t cgi_client_req_hdr_ctx_t;
struct cgi_client_req_hdr_ctx_t
{
	qse_httpd_t* httpd;
	qse_env_t* env;
};

static int cgi_capture_client_header (
	qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	cgi_client_req_hdr_ctx_t* hdrctx;

	hdrctx = (cgi_client_req_hdr_ctx_t*)ctx;

	if (qse_mbscasecmp (key, QSE_MT("Connection")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0)
	{
		qse_mchar_t* http_key, * ptr;
		int ret;

		http_key = qse_mbsdup2 (QSE_MT("HTTP_"), key, req->mmgr);
		if (http_key == QSE_NULL)
		{
			hdrctx->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		for (ptr = http_key; *ptr; ptr++)
		{
			*ptr = QSE_TOMUPPER((unsigned)*ptr);
			if (*ptr == QSE_MT('-')) *ptr = '_';
		}

		/* insert the last value only */
		while (val->next) val = val->next;
		ret = qse_env_insertmbs (hdrctx->env, http_key, val->ptr);
		if (ret <= -1) 
		{
			QSE_MMGR_FREE (req->mmgr, http_key);
			hdrctx->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		QSE_MMGR_FREE (req->mmgr, http_key);
	}

	return 0;
}

static int cgi_add_header_to_buffer (
	task_cgi_t* cgi, qse_mbs_t* buf, const qse_mchar_t* key, const qse_htre_hdrval_t* val)
{
	QSE_ASSERT (val != QSE_NULL);

	do
	{
		if (qse_mbs_cat (buf, key) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, QSE_MT(": ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, val->ptr) == (qse_size_t)-1 ||
		    qse_mbs_cat (buf, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			/* multiple items with the same keys are also 
			 * copied back to the response buffer */
			cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		val = val->next;
	}
	while (val);

	return 0;
}

static int cgi_capture_client_trailer (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_cgi_t* cgi = (task_cgi_t*)ctx;

	if (qse_mbscasecmp (key, QSE_MT("Content-Length")) != 0 && 
	    qse_mbscasecmp (key, QSE_MT("Connection")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0)
	{
		return cgi_add_header_to_buffer (cgi, cgi->reqfwdbuf, key, val);
	}

	return 0;
}

static int cgi_capture_script_header (qse_htre_t* req, const qse_mchar_t* key, const qse_htre_hdrval_t* val, void* ctx)
{
	task_cgi_t* cgi = (task_cgi_t*)ctx;

	/* capture a header except Status, Connection, Transfer-Encoding, and Server */
	if (qse_mbscasecmp (key, QSE_MT("Status")) != 0 && 
	    qse_mbscasecmp (key, QSE_MT("Connection")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Transfer-Encoding")) != 0 &&
	    qse_mbscasecmp (key, QSE_MT("Server")) != 0) 
	{
		return cgi_add_header_to_buffer (cgi, cgi->res, key, val);
	}

	return 0;
}

static void log_cgi_script_error (task_cgi_t* cgi, const qse_mchar_t* shortmsg)
{
	qse_httpd_act_t msg;
	qse_size_t pos = 0;

	msg.code = QSE_HTTPD_CATCH_MERRMSG;
	pos += qse_mbsxcpy (&msg.u.merrmsg[pos], QSE_COUNTOF(msg.u.merrmsg) - pos, shortmsg);
	pos += qse_mbsxcpy (&msg.u.merrmsg[pos], QSE_COUNTOF(msg.u.merrmsg) - pos, cgi->script);
	cgi->httpd->opt.rcb.logact (cgi->httpd, &msg);
}

static int cgi_htrd_peek_script_output (qse_htrd_t* htrd, qse_htre_t* req)
{
	cgi_script_htrd_xtn_t* xtn;
	task_cgi_t* cgi;
	int keepalive;

	xtn = (cgi_script_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	cgi = xtn->cgi;

	QSE_ASSERT (!cgi->nph);

	if (req->attr.status)
	{
		qse_mchar_t buf[128];
		int nstatus;
		qse_mchar_t* endptr;

/* TODO: check the syntax of status value??? if not numeric??? */
		QSE_MBSTONUM (nstatus, req->attr.status, &endptr, 10);

		snprintf (buf, QSE_COUNTOF(buf), 	
			QSE_MT("HTTP/%d.%d %d "),
			cgi->version.major, 
			cgi->version.minor, 
			nstatus
		);

		/* 
		Would it need this kind of extra work?
		while (QSE_ISMSPACE(*endptr)) endptr++;
		if (*endptr == QSE_MT('\0')) ....
		*/

		if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1 ||
		    qse_mbs_cat (cgi->res, endptr) == (qse_size_t)-1 ||
		    qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) 
		{
			cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}
	}
	else 
	{
		const qse_htre_hdrval_t* location;
		qse_mchar_t buf[128];

		location = qse_htre_getheaderval (req, QSE_MT("Location"));
		if (location)
		{
			snprintf (buf, QSE_COUNTOF(buf),
				QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\n"),
				cgi->version.major, cgi->version.minor
			);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) 
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}

			/* the actual Location header is added by 
			 * qse_htre_walkheaders() below */
		}
		else
		{
			snprintf (buf, QSE_COUNTOF(buf),
				QSE_MT("HTTP/%d.%d 200 OK\r\n"),
				cgi->version.major, cgi->version.minor
			);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) 
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
	}

	/* Add the server header. the server header in the cgi output will
	 * be ignored by cgi_capture_script_header() */
	if (qse_mbs_cat (cgi->res, QSE_MT("Server: ")) == (qse_size_t)-1 ||
	    qse_mbs_cat (cgi->res, qse_httpd_getname (cgi->httpd)) == (qse_size_t)-1 ||
	    qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1)
	{
		cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
		return -1;
	}

	if (qse_htre_getheaderval (req, QSE_MT("Date")) == QSE_NULL)
	{
		/* generate the Date header if it's not included in the script output */
		if (qse_mbs_cat (cgi->res, QSE_MT("Date: ")) == (qse_size_t)-1 ||
		    qse_mbs_cat (cgi->res, qse_httpd_fmtgmtimetobb (cgi->httpd, QSE_NULL, 0)) == (qse_size_t)-1 ||
		    qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1)
		{
			cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}
	}

	keepalive = cgi->keepalive;
	if (req->attr.flags & QSE_HTRE_ATTR_LENGTH)
	{
		cgi->resflags |= CGI_RES_SCRIPT_LENGTH;
		cgi->script_output_length = req->attr.content_length;
	}
	else
	{
		/* no Content-Length returned by CGI. */
		if (qse_comparehttpversions (&cgi->version, &qse_http_v11) >= 0) 
		{
			/* the client side supports chunking */
			cgi->resflags |= CGI_RES_CLIENT_CHUNK;
			if (qse_mbs_cat (cgi->res, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1) 
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else 
		{
			/* If the CGI script doesn't specify Content-Length, 
			 * i can't honor cgi->keepalive in HTTP/1.0
			 * or earlier. Closing the connection is the
			 * only way to specify the content length */
			cgi->resflags |= CGI_RES_CLIENT_DISCON;
			keepalive = 0; 

			if (qse_httpd_entaskdisconnect (
				cgi->httpd, xtn->client, xtn->task) == QSE_NULL) 
			{
				return -1;
			}
		}
	}

	/* NOTE: 
	 *   an explicit 'disconnect' task is added only if
	 *   the orignal keep-alive request can't be honored.
	 *   so if a client specifies keep-alive but doesn't
	 *   close connection, the connection will stay alive
	 *   until it's cleaned up for an error or idle timeout.
	 */

	if (qse_mbs_cat (cgi->res, (keepalive? QSE_MT("Connection: keep-alive\r\n"): QSE_MT("Connection: close\r\n"))) == (qse_size_t)-1) 
	{
		cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
		return -1;
	}

	/* add all other header fields excluding 
	 * Status, Connection, Transfer-Encoding */
	if (qse_htre_walkheaders (req, cgi_capture_script_header, cgi) <= -1) return -1;
	/* end of header */
	if (qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1; 

	/* content body begins here */
	cgi->script_output_received = qse_htre_getcontentlen(req);
	if ((cgi->resflags & CGI_RES_SCRIPT_LENGTH) &&
	    cgi->script_output_received > cgi->script_output_length)
	{
		/* cgi returning too much data... something is wrong in CGI */
		if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
			log_cgi_script_error (cgi, QSE_MT("cgi redundant output - "));
		cgi->httpd->errnum = QSE_HTTPD_EINVAL; /* TODO: change it to a better error code */
		return -1;
	}

	if (cgi->script_output_received > 0)
	{
		/* the initial part of the content body has been received 
		 * along with the header. it need to be added to the result 
		 * buffer. */
		if (cgi->resflags & CGI_RES_CLIENT_CHUNK)
		{
			qse_mchar_t buf[64];

			qse_fmtuintmaxtombs (
				buf, QSE_COUNTOF(buf), 
				cgi->script_output_received, 
				16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 
				-1, QSE_MT('\0'), QSE_NULL);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1 ||
			    qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1)
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		if (qse_mbs_ncat (cgi->res, qse_htre_getcontentptr(req), qse_htre_getcontentlen(req)) == (qse_size_t)-1) 
		{
			cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		if (cgi->resflags & CGI_RES_CLIENT_CHUNK)
		{
			if (qse_mbs_ncat (cgi->res, QSE_MT("\r\n"), 2) == (qse_size_t)-1) 
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
	}

	return 0; 
}

static qse_htrd_recbs_t cgi_script_htrd_cbs =
{
	cgi_htrd_peek_script_output,
	QSE_NULL /* not needed for CGI */
};

static int cgi_add_env (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_env_t* env, qse_htre_t* req, 
	const qse_mchar_t* path, 
	const qse_mchar_t* script,
	const qse_mchar_t* suffix,
	const qse_mchar_t* root,
	const qse_mchar_t* content_type,
	qse_size_t content_length, 
	int chunked)
{
/* TODO: error check for various insert... */

	cgi_client_req_hdr_ctx_t ctx;
	qse_mchar_t buf[128];
	const qse_http_version_t* v;
	const qse_mchar_t* qparam;

	v = qse_htre_getversion(req);
	qparam = qse_htre_getqparam(req);

#ifdef _WIN32
	qse_env_insert (env, QSE_T("PATH"), QSE_NULL);
#else
	qse_env_insertmbs (env, QSE_MT("LANG"), QSE_NULL);
	qse_env_insertmbs (env, QSE_MT("PATH"), QSE_NULL);
#endif

	qse_env_insertmbs (env, QSE_MT("GATEWAY_INTERFACE"), QSE_MT("CGI/1.1"));
	snprintf (buf, QSE_COUNTOF(buf), QSE_MT("HTTP/%d.%d"), (int)v->major, (int)v->minor);
	qse_env_insertmbs (env, QSE_MT("SERVER_PROTOCOL"), buf);

	qse_env_insertmbs (env, QSE_MT("SCRIPT_FILENAME"), path);
	qse_env_insertmbs (env, QSE_MT("SCRIPT_NAME"), script);
	qse_env_insertmbs (env, QSE_MT("DOCUMENT_ROOT"), root);
	if (suffix && suffix[0] != QSE_MT('\0')) 
	{
		const qse_mchar_t* tmp[3];
		qse_mchar_t* tr;

		tmp[0] = root; 
		tmp[1] = suffix; 
		tmp[2] = QSE_NULL;

		tr = qse_mbsadup  (tmp, QSE_NULL, httpd->mmgr);
		if (tr) 
		{
			qse_canonmbspath (tr, tr, 0);
			qse_env_insertmbs (env, QSE_MT("PATH_TRANSLATED"), tr);
			QSE_MMGR_FREE (httpd->mmgr, tr);
		}
		qse_env_insertmbs (env, QSE_MT("PATH_INFO"), suffix);
	}

	qse_env_insertmbs (env, QSE_MT("REQUEST_METHOD"), qse_htre_getqmethodname(req));
	qse_env_insertmbs (env, QSE_MT("REQUEST_URI"), qse_htre_getqpath(req));
	if (qparam) qse_env_insertmbs (env, QSE_MT("QUERY_STRING"), qparam);

	qse_fmtuintmaxtombs (buf, QSE_COUNTOF(buf), content_length, 10, -1, QSE_MT('\0'), QSE_NULL);
	qse_env_insertmbs (env, QSE_MT("CONTENT_LENGTH"), buf);
	if (content_type) qse_env_insertmbs (env, QSE_MT("CONTENT_TYPE"), content_type);

	if (chunked) qse_env_insertmbs (env, QSE_MT("TRANSFER_ENCODING"), QSE_MT("chunked"));

	qse_env_insertmbs (env, "SERVER_SOFTWARE", qse_httpd_getname(httpd));
	qse_nwadtombs (&client->local_addr, buf, QSE_COUNTOF(buf), QSE_NWADTOMBS_PORT);
	qse_env_insertmbs (env, QSE_MT("SERVER_PORT"), buf);
	qse_nwadtombs (&client->local_addr, buf, QSE_COUNTOF(buf), QSE_NWADTOMBS_ADDR);
	qse_env_insertmbs (env, QSE_MT("SERVER_ADDR"), buf);
	qse_env_insertmbs (env, QSE_MT("SERVER_NAME"), buf); /* TODO:  convert it to a host name */

	qse_nwadtombs (&client->remote_addr, buf, QSE_COUNTOF(buf), QSE_NWADTOMBS_PORT);
	qse_env_insertmbs (env, QSE_MT("REMOTE_PORT"), buf);
	qse_nwadtombs (&client->remote_addr, buf, QSE_COUNTOF(buf), QSE_NWADTOMBS_ADDR);
	qse_env_insertmbs (env, QSE_MT("REMOTE_ADDR"), buf);

	ctx.httpd = httpd;
	ctx.env = env;
	if (qse_htre_walkheaders (req, cgi_capture_client_header, &ctx) <= -1) return -1;
	if (qse_htre_walktrailers (req, cgi_capture_client_header, &ctx) <= -1) return -1;

	return 0;
}

static int cgi_snatch_client_input (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	qse_httpd_task_t* task;
	task_cgi_t* cgi; 

	task = (qse_httpd_task_t*)ctx;
	cgi = (task_cgi_t*)task->ctx;

#if 0
if (ptr) qse_printf (QSE_T("!!!CGI SNATCHING [%.*hs]\n"), len, ptr);
else qse_printf (QSE_T("!!!CGI SNATCHING DONE\n"));
#endif

	QSE_ASSERT (cgi->req);
	QSE_ASSERT (!(cgi->reqflags & CGI_REQ_GOTALL));

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

		if (cgi->reqflags & CGI_REQ_FWDCHUNKED)
		{
			/* add the 0-sized chunk and trailers */
			if (qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("0\r\n")) == (qse_size_t)-1 ||
			    qse_htre_walktrailers (req, cgi_capture_client_trailer, cgi) <= -1 ||
			    qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1)
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		/* mark the there's nothing to read form the client side */
		cgi->reqflags |= CGI_REQ_GOTALL;

		/* deregister the callback so that data fed to this 
		 * request object before the current task terminats 
		 * doesn't trigger this callback. */
		qse_htre_unsetconcb (cgi->req);
		cgi->req = QSE_NULL;

		/* since there is no more to read from the client side.
		 * the relay trigger is not needed any more. */
		task->trigger[2].mask = 0;

		if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0 && cgi->pio_inited &&
		    !(task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
		{
			/* there's nothing more to read from the client side.
			 * there's something to forward in the forwarding buffer.
			 * but no write trigger is set. add the write trigger 
			 * for task invocation. */
			task->trigger[1].mask = QSE_HTTPD_TASK_TRIGGER_WRITE;
		}
	}
	else if (!(cgi->reqflags & CGI_REQ_FWDERR))
	{
		/* we can write to the child process if a forwarding error 
		 * didn't occur previously. we store data from the client side
		 * to the forwaring buffer only if there's no such previous
		 * error. if an error occurred, we simply drop the data. */
		if (cgi->reqflags & CGI_REQ_FWDCHUNKED)
		{
			qse_mchar_t buf[64];
			qse_fmtuintmaxtombs (
				buf, QSE_COUNTOF(buf), len,
				16 | QSE_FMTUINTMAXTOMBS_UPPERCASE,
				-1, QSE_MT('\0'), QSE_NULL);

			if (qse_mbs_cat (cgi->reqfwdbuf, buf) == (qse_size_t)-1 ||
			    qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
			    qse_mbs_ncat (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1 ||
			    qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1)
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}
		else
		{
			if (qse_mbs_ncat (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1)
			{
				cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}
		}

		/* output pipe to child */
		task->trigger[1].mask = QSE_HTTPD_TASK_TRIGGER_WRITE;
#if 0
qse_printf (QSE_T("!!!CGI SNATCHED [%.*hs]\n"), len, ptr);
#endif
	}

	return 0;
}

static void cgi_forward_client_input_to_script (
	qse_httpd_t* httpd, qse_httpd_task_t* task, int writable)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;

	QSE_ASSERT (cgi->reqfwdbuf != QSE_NULL);

	if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
	{
		/* there is something to forward in the forwarding buffer. */

		if (cgi->reqflags & CGI_REQ_FWDERR)
		{
			/* a forwarding error has occurred previously.
			 * clear the forwarding buffer */
#if 0
qse_printf (QSE_T("FORWARD: CLEARING REQCON FOR ERROR\n"));
#endif
			qse_mbs_clear (cgi->reqfwdbuf);
		}
		else
		{
			/* normal forwarding */
			qse_ssize_t n;

			if (writable) goto forward;

			n = httpd->opt.scb.mux.writable (
				httpd, qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_IN), 0);
			if (n >= 1)
			{
			forward:
				/* writable */
#if 0
qse_printf (QSE_T("FORWARD: @@@@@@@@@@WRITING[%.*hs]\n"),
	(int)QSE_MBS_LEN(cgi->reqfwdbuf),
	QSE_MBS_PTR(cgi->reqfwdbuf));
#endif
				n = qse_pio_write (
					&cgi->pio, QSE_PIO_IN,
					QSE_MBS_PTR(cgi->reqfwdbuf),
					QSE_MBS_LEN(cgi->reqfwdbuf)
				);
				if (n > 0) 
				{
/* TODO: improve performance.. instead of copying the remaing part 
to the head all the time..  grow the buffer to a certain limit. */
					qse_mbs_del (cgi->reqfwdbuf, 0, n);
					if (QSE_MBS_LEN(cgi->reqfwdbuf) <= 0)
					{
						if (cgi->reqflags & CGI_REQ_GOTALL) goto done;
						else task->trigger[1].mask = 0; /* pipe output to child */
					}
				}
			}

			if (n <= -1)
			{
				if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_cgi_script_error (cgi, QSE_MT("cgi pio write error - "));

				cgi->reqflags |= CGI_REQ_FWDERR;
				qse_mbs_clear (cgi->reqfwdbuf); 

				if (!(cgi->reqflags & CGI_REQ_GOTALL))
				{
					QSE_ASSERT (cgi->req);
					qse_htre_discardcontent (cgi->req);

					/* NOTE: 
					 *  this qse_htre_discardcontent() invokes
					 *  cgi_snatch_client_input() 
					 *  which sets cgi->req to QSE_NULL
					 *  and toggles on CGI_REQ_GOTALL. */
					QSE_ASSERT (!cgi->req);
					QSE_ASSERT (cgi->reqflags & CGI_REQ_GOTALL);
				}

				/* mark the end of input to the child explicitly. */
				qse_pio_end (&cgi->pio, QSE_PIO_IN);
				task->trigger[1].mask = 0; /* pipe output to child */
			}
		}
	}
	else if (cgi->reqflags & CGI_REQ_GOTALL)
	{
	done:
		/* there is nothing to read from the client side and
		 * there is nothing more to forward in the forwarding buffer.
		 * clear the relay and write triggers for the time being.
		 */
#if 0
qse_printf (QSE_T("FORWARD: @@@@@@@@NOTHING MORE TO WRITE TO CGI\n"));
#endif
		QSE_ASSERT (cgi->req == QSE_NULL);

		/* mark the end of input to the child explicitly. */
		qse_pio_end (&cgi->pio, QSE_PIO_IN);

		task->trigger[1].mask = 0; /* pipe output to child */
		task->trigger[2].mask = 0; /* client-side */
	}
}

static int task_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi;
	task_cgi_arg_t* arg;
	qse_size_t content_length;
	qse_size_t len;
	const qse_mchar_t* ptr;
	const qse_htre_hdrval_t* tmp;
	
	cgi = (task_cgi_t*)qse_httpd_gettaskxtn (httpd, task);
	arg = (task_cgi_arg_t*)task->ctx;

/* TODO: can content length be a different type???
 *  maybe qse_uintmax_t.... it thinks the data size can be larger than the max pointer size 
 * qse_htre_t and qse_htrd_t also needs changes to support it
 */

	QSE_MEMSET (cgi, 0, QSE_SIZEOF(*cgi));
	cgi->httpd = httpd;

	cgi->path = (qse_mchar_t*)(cgi + 1);
	cgi->script = cgi->path + arg->path.len + 1;
	cgi->suffix = cgi->script + arg->script.len + 1;
	cgi->root = cgi->suffix + arg->suffix.len + 1;
	cgi->shebang = cgi->root + arg->root.len + 1;
	qse_mbscpy ((qse_mchar_t*)cgi->path, arg->path.ptr);
	qse_mbscpy ((qse_mchar_t*)cgi->script, arg->script.ptr);
	qse_mbscpy ((qse_mchar_t*)cgi->suffix, arg->suffix.ptr);
	qse_mbscpy ((qse_mchar_t*)cgi->root, arg->root.ptr);
	qse_mbscpy ((qse_mchar_t*)cgi->shebang, arg->shebang.ptr);

	cgi->method = qse_htre_getqmethodtype(arg->req);
	cgi->version = *qse_htre_getversion(arg->req);
	cgi->keepalive = (arg->req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);
	cgi->nph = arg->nph;
	cgi->req = QSE_NULL;

	content_length = 0;
	if (arg->req->state & QSE_HTRE_DISCARDED) goto done;

	len = qse_htre_getcontentlen(arg->req);
	if ((arg->req->state & QSE_HTRE_COMPLETED) && len <= 0)
	{
		/* the content part is completed and no content 
		 * in the content buffer. there is nothing to forward */
		cgi->reqflags |= CGI_REQ_GOTALL;
		goto done;
	}

	if (!(arg->req->state & QSE_HTRE_COMPLETED) &&
	    !(arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH))
	{
		/* if the request is not completed and doesn't have
		 * content-length set, it's not really possible to
		 * pass the content. this function, however, allows
		 * such a request to entask a cgi script dropping the
		 * content */

		if (httpd->opt.trait & QSE_HTTPD_CGINOCHUNKED)
		{
			qse_htre_discardcontent (arg->req);
			cgi->reqflags |= CGI_REQ_GOTALL;
		}
		else 
		{
			/* do chunking to cgi */
			cgi->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
			if (cgi->reqfwdbuf == QSE_NULL) 
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				goto oops;
			}

			if (len > 0)
			{
				qse_mchar_t buf[64];

				qse_fmtuintmaxtombs (
					buf, QSE_COUNTOF(buf), len,
					16 | QSE_FMTUINTMAXTOMBS_UPPERCASE,
					-1, QSE_MT('\0'), QSE_NULL);

				ptr = qse_htre_getcontentptr(arg->req);
				if (qse_mbs_cat (cgi->reqfwdbuf, buf) == (qse_size_t)-1 ||
				    qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1 ||
				    qse_mbs_ncat (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1 ||
				    qse_mbs_cat (cgi->reqfwdbuf, QSE_MT("\r\n")) == (qse_size_t)-1) 
				{
					httpd->errnum = QSE_HTTPD_ENOMEM;
					goto oops;
				}

			}

			cgi->reqflags |= CGI_REQ_FWDCHUNKED;
			cgi->req = arg->req; 
			qse_htre_setconcb (cgi->req, cgi_snatch_client_input, task);
		}
	}
	else
	{	
		/* create a buffer to hold request content from the client
		 * and copy content received already */
		cgi->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
		if (cgi->reqfwdbuf == QSE_NULL) 
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			goto oops;
		}

		ptr = qse_htre_getcontentptr(arg->req);
		if (qse_mbs_ncpy (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1) 
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			goto oops;
		}

		if (arg->req->state & QSE_HTRE_COMPLETED)
		{
			/* no furthur forwarding is needed. 
			 * even a chunked request entaksed when completed 
			 * should reach here. if content-length is set
			 * the length should match len. */
			QSE_ASSERT (len > 0);
			QSE_ASSERT (!(arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH) ||
			            ((arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH) && 
			             arg->req->attr.content_length == len));
			cgi->reqflags |= CGI_REQ_GOTALL;
			content_length = len;
		}
		else
		{
			/* CGI entasking is invoked probably from the peek handler
			 * that was triggered after the request header is received.
			 * you can know this because the request is not completed.
			 * In this case, arrange to forward content
			 * bypassing the buffer in the request object itself. */

/* TODO: callback chain instead of a single pointer??? 
       if the request is already set up with a callback, something will go wrong.
*/
			/* set up a callback to be called when the request content
			 * is fed to the htrd reader. qse_htre_addcontent() that 
			 * htrd calls invokes this callback. */

			/* remember this request pointer if and only if the content 
			 * is not fully read. so reading into this request object
			 * is guaranteed not to break integrity of this object */
			cgi->req = arg->req; 
			qse_htre_setconcb (cgi->req, cgi_snatch_client_input, task);

			QSE_ASSERT (arg->req->attr.flags & QSE_HTRE_ATTR_LENGTH);
			content_length = arg->req->attr.content_length;
		}
	}

done:
	cgi->env = qse_env_open (httpd->mmgr, 0, 0);
	if (cgi->env == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		goto oops;
	}

	/* get the content type header value */
	tmp = qse_htre_getheaderval(arg->req, QSE_MT("Content-Type"));
	if (tmp) while (tmp->next) tmp = tmp->next; /* get the last value */

	if (cgi_add_env (
		httpd, client, cgi->env, arg->req, 
		cgi->path, cgi->script, cgi->suffix, cgi->root, 
		(tmp? tmp->ptr: QSE_NULL), content_length, 
		(cgi->reqflags & CGI_REQ_FWDCHUNKED)) <= -1)
	{
		goto oops;
	}

	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */

	task->ctx = cgi;
	return 0;

oops:
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_cgi()
	 * add an error task */
	cgi->init_failed = 1;
	task->ctx = cgi;

	if (cgi->env)
	{
		qse_env_close (cgi->env);
		cgi->env = QSE_NULL;
	}
	if (cgi->reqfwdbuf)
	{
		qse_mbs_close (cgi->reqfwdbuf);
		cgi->reqfwdbuf = QSE_NULL;
	}

	return 0;
}

static void task_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	if (cgi->env) qse_env_close (cgi->env);
	if (cgi->pio_inited) 
	{
		/* kill cgi in case it is still alive.
		 * qse_pio_wait() in qse_pio_fini() can block. */
		qse_pio_kill (&cgi->pio); 
		qse_pio_fini (&cgi->pio);
	}
	if (cgi->res) qse_mbs_close (cgi->res);
	if (cgi->script_htrd) qse_htrd_close (cgi->script_htrd);
	if (cgi->reqfwdbuf) qse_mbs_close (cgi->reqfwdbuf);
	if (cgi->req) 
	{
		/* this task is destroyed but the request
		 * associated is still alive. so clear the 
		 * callback to prevent the callback call. */
		QSE_ASSERT (!(cgi->reqflags & CGI_REQ_GOTALL));
		qse_htre_unsetconcb (cgi->req);
	}
}

static QSE_INLINE qse_ssize_t cgi_read_script_output_to_buffer (
	qse_httpd_t* httpd, qse_httpd_client_t* client, task_cgi_t* cgi)
{
	qse_ssize_t n;

	n = qse_pio_read (
		&cgi->pio, QSE_PIO_OUT,
		&cgi->buf[cgi->buflen], 
		QSE_SIZEOF(cgi->buf) - cgi->buflen
	);
	if (n > 0) cgi->buflen += n;

	if (n <= -1 && cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
		log_cgi_script_error (cgi, QSE_MT("cgi pio read error - "));

	return n;
}

static QSE_INLINE qse_ssize_t cgi_write_script_output_to_client (
	qse_httpd_t* httpd, qse_httpd_client_t* client, task_cgi_t* cgi)
{
	qse_ssize_t n;

	n = httpd->opt.scb.client.send (httpd, client, cgi->buf, cgi->buflen);
	if (n > 0)
	{
		QSE_MEMCPY (&cgi->buf[0], &cgi->buf[n], cgi->buflen - n);
		cgi->buflen -= n;
	}

	if (n <= -1 && cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
		log_cgi_script_error (cgi, QSE_MT("cgi write error to client - "));

	return n;
}

static int task_main_cgi_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;

	QSE_ASSERT (cgi->pio_inited);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 1);
	}

	if (!(task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) ||
	    (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		if (cgi->buflen > 0)
		{
			if (cgi_write_script_output_to_client (httpd, client, cgi) <= -1)
			{
				/* can't return internal server error any more... */
				return -1;
			}
		}
	}

	/* if forwarding didn't finish, something is not really right... 
	 * so long as the output from CGI is finished, no more forwarding
	 * is performed */
	return (cgi->buflen > 0 || !(cgi->reqflags & CGI_REQ_GOTALL) ||
	        (cgi->reqfwdbuf && QSE_MBS_LEN(cgi->reqfwdbuf) > 0))? 1: 0;
}

static int task_main_cgi_4_nph (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;

	QSE_ASSERT (cgi->nph);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		if (cgi->buflen < QSE_SIZEOF(cgi->buf))
		{
			n = cgi_read_script_output_to_buffer (httpd, client, cgi);
			if (n <= -1) return -1; /* TODO: logging */
			if (n == 0) 
			{
				/* switch to the next phase */
				task->main = task_main_cgi_5;
				task->trigger[0].mask = 0;
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				return 1;
			}
		}

		QSE_ASSERT (cgi->buflen > 0);
		if (cgi_write_script_output_to_client (httpd, client, cgi) <= -1) return -1;
	}

	return 1;
}

static int task_main_cgi_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (!cgi->nph);
	QSE_ASSERT (cgi->pio_inited);

#if 0
qse_printf (QSE_T("task_main_cgi_4 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);
#endif

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		if (cgi->resflags & CGI_RES_CLIENT_CHUNK)
		{
			qse_size_t count, extra;

			/* this function assumes that the chunk length does not 
			 * exceed 4 hexadecimal digits. */
			QSE_ASSERT (QSE_SIZEOF(cgi->buf) <= 0xFFFF);
	
#define CHLEN_RESERVE 6 

			extra = CHLEN_RESERVE + 2;
			count = QSE_SIZEOF(cgi->buf) - cgi->buflen;
			if (count > extra)
			{
				n = qse_pio_read (
					&cgi->pio, QSE_PIO_OUT,
					&cgi->buf[cgi->buflen + CHLEN_RESERVE], 
					count - extra
				);
				if (n <= -1)
				{
					if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_cgi_script_error (cgi, QSE_MT("cgi pio read error - "));
					return -1;
				}
				if (n == 0) 
				{
					/* the cgi script closed the output */
					cgi->buf[cgi->buflen++] = QSE_MT('0');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
	
					task->main = task_main_cgi_5;
					task->trigger[0].mask = 0;
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}
	
				/* set the chunk length. if the length string is less 
				 * than 4 digits, the right side of the string is filled
				 * with space letters. for example, the chunk length line
				 * for the length 10 will be "A   \r\n". */
				cgi->buflen += qse_fmtuintmaxtombs (
					&cgi->buf[cgi->buflen], CHLEN_RESERVE - 2 + 1,
					n, 16 | QSE_FMTUINTMAXTOMBS_UPPERCASE | QSE_FMTUINTMAXTOMBS_FILLRIGHT, 
					-1, QSE_MT(' '), QSE_NULL
				); 
				cgi->buf[cgi->buflen++] = QSE_MT('\r');
				cgi->buf[cgi->buflen++] = QSE_MT('\n');

				cgi->buflen += n; /* +n for the data read above */

				/* set the trailing CR & LF for a chunk */
				cgi->buf[cgi->buflen++] = QSE_MT('\r');
				cgi->buf[cgi->buflen++] = QSE_MT('\n');
	
				cgi->script_output_received += n;

				if ((cgi->resflags & CGI_RES_SCRIPT_LENGTH) &&
				    cgi->script_output_received > cgi->script_output_length)
				{
					/* cgi returning too much data... something is wrong in CGI */
					if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_cgi_script_error (cgi, QSE_MT("cgi redundant output - "));
					return -1;
				}
			}
		}
		else
		{
			if (cgi->buflen < QSE_SIZEOF(cgi->buf))
			{
				n = cgi_read_script_output_to_buffer (httpd, client, cgi);
				if (n <= -1) return -1; /* TODO: logging */
				if (n == 0) 
				{
					/* switch to the next phase */
					task->main = task_main_cgi_5;
					task->trigger[0].mask = 0;
					task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					return 1;
				}

				cgi->script_output_received += n;
				if ((cgi->resflags & CGI_RES_SCRIPT_LENGTH) &&
				    cgi->script_output_received > cgi->script_output_length)
				{
					/* cgi returning too much data... something is wrong in CGI */
					if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
						log_cgi_script_error (cgi, QSE_MT("cgi redundant output - "));
					return -1;
				}
			}
		}
	
		/* the main loop invokes the task function only if the client 
		 * side is writable. it should be safe to write whenever
		 * this task function is called. */
		QSE_ASSERT (cgi->buflen > 0);
		if (cgi_write_script_output_to_client (httpd, client, cgi) <= -1) return -1;
	}

	return 1;
}

static int task_main_cgi_3 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* send the http initial line and headers built using the headers
	 * returned by CGI. it may include some contents as well */

	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	qse_size_t count;

	QSE_ASSERT (!cgi->nph);

#if  0
qse_printf (QSE_T("task_main_cgi_3 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);
#endif
	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 1);
	}

	/* send the partial reponse received with the initial line and headers
	 * so long as the client-side handle is writable... */
	if (!(task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) ||
	    (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		count = MAX_SEND_SIZE;
		if (count >= cgi->res_left) count = cgi->res_left;

		if (count > 0)
		{
			n = httpd->opt.scb.client.send (httpd, client, cgi->res_ptr, count);
			if (n <= -1) 
			{
				if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
					log_cgi_script_error (cgi, QSE_MT("cgi initial write error to client - "));
				return -1;
			}

			cgi->res_ptr += n;
			cgi->res_left -= n;
		}

		if (cgi->res_left <= 0) 
		{
			qse_mbs_clear (cgi->res);

			if ((cgi->resflags & CGI_RES_SCRIPT_LENGTH) &&
			    cgi->script_output_received >= cgi->script_output_length)
			{	
				/* if a cgi script specified the content length
				 * and it has emitted as much as the length,
				 * i don't wait for the script to finish.
				 * one potential side-effect is that the script
				 * can be killed prematurely if it wants to do
				 * something extra after having done so.
				 * however, a CGI script shouln't do that... */ 
				task->main = task_main_cgi_5;
				task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			else
			{
				task->main = task_main_cgi_4;
				task->trigger[2].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
			return 1;
		}

	}

	return 1; /* more work to do */
}

static int task_main_cgi_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* several calls to this function will read output from the cgi 
	 * until the end of header is reached. when the end is reached, 
	 * it is possible that some contents are also read in.
	 * The callback function to qse_htrd_feed() handles this also.
	 */
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (!cgi->nph);
	QSE_ASSERT (cgi->pio_inited);

#if 0
qse_printf (QSE_T("task_main_cgi_2 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);
#endif

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_client_input_to_script (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		n = qse_pio_read (
			&cgi->pio, QSE_PIO_OUT,
			&cgi->buf[cgi->buflen], 
			QSE_SIZEOF(cgi->buf) - cgi->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
			if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
				log_cgi_script_error (cgi, QSE_MT("cgi pio read error - "));
			goto oops;
		}
		if (n == 0) 
		{
			/* end of output from cgi before it has seen a header.
			 * the cgi script must be crooked. */
			if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
				log_cgi_script_error (cgi, QSE_MT("cgi premature eof - "));
			goto oops;
		}
		cgi->buflen += n;

		if (qse_htrd_feed (cgi->script_htrd, cgi->buf, cgi->buflen) <= -1)
		{
			if (cgi->httpd->opt.trait & QSE_HTTPD_LOGACT) 
				log_cgi_script_error (cgi, QSE_MT("cgi feed error - "));
			goto oops;
		}

		cgi->buflen = 0;

		if (QSE_MBS_LEN(cgi->res) > 0)
		{
			/* the htrd handler composed some response.
			 * this means that at least it finished processing CGI headers.
			 * some contents could be in cgi->res, though.
			 *
			 * qse_htrd_feed() must have executed the peek handler
			 * (cgi_htrd_peek_script_output()) which handled the 
			 * request header. so i won't call qse_htrd_feed() any more.
			 * intead, i'll simply read directly from the pipe.
			 */
			cgi->res_ptr = QSE_MBS_PTR(cgi->res);
			cgi->res_left = QSE_MBS_LEN(cgi->res);

#if 0
qse_printf (QSE_T("TRAILING DATA=[%.*hs]\n"), (int)QSE_MBS_LEN(cgi->res), QSE_MBS_PTR(cgi->res));
#endif
			task->main = task_main_cgi_3;
			task->trigger[2].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
			return 1;
		}

	}

	/* complete headers not seen yet. i need to be called again */
	return 1;

oops:
	return (qse_httpd_entask_err (httpd, client, task, 500, cgi->method, &cgi->version, cgi->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	int pio_options, x;
	int http_errnum = 500;
	qse_mchar_t* xpath;

	if (cgi->init_failed) goto oops;

	if (cgi->nph)
	{
		/* i cannot know how long the content will be
		 * since i don't parse the header. so i have to close
		 * the connection regardless of content-length or transfer-encoding
		 * in the actual header. */
		if (qse_httpd_entaskdisconnect (
			httpd, client, task) == QSE_NULL) goto oops;
	}
	else
	{
		cgi_script_htrd_xtn_t* xtn;
		cgi->script_htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(cgi_script_htrd_xtn_t));
		if (cgi->script_htrd == QSE_NULL) goto oops;
		xtn = (cgi_script_htrd_xtn_t*) qse_htrd_getxtn (cgi->script_htrd);
		xtn->cgi = cgi;
		xtn->task = task;
		xtn->client = client;
		qse_htrd_setrecbs (cgi->script_htrd, &cgi_script_htrd_cbs);
		qse_htrd_setoption (
			cgi->script_htrd, 
			QSE_HTRD_SKIPINITIALLINE | 
			QSE_HTRD_PEEKONLY | 
			QSE_HTRD_REQUEST
		);

		cgi->res = qse_mbs_open (httpd->mmgr, 0, 256);
		if (cgi->res == QSE_NULL) goto oops;
	}

	pio_options = QSE_PIO_READOUT | QSE_PIO_WRITEIN | QSE_PIO_MBSCMD;
	if (httpd->opt.trait & QSE_HTTPD_CGIERRTONUL)
		pio_options |= QSE_PIO_ERRTONUL;
	else
		pio_options |= QSE_PIO_ERRTOOUT;
	if (httpd->opt.trait & QSE_HTTPD_CGINOCLOEXEC) 
		pio_options |= QSE_PIO_NOCLOEXEC;

	if (cgi->shebang[0] != QSE_MT('\0'))
	{
		const qse_mchar_t* tmp[4];
		tmp[0] = cgi->shebang;
		tmp[1] = QSE_MT(" ");
		tmp[2] = cgi->path;
		tmp[3] = QSE_NULL;
		xpath = qse_mbsadup (tmp, QSE_NULL, httpd->mmgr);
		if (xpath == QSE_NULL) goto oops;
	}
	else xpath = cgi->path;

	x = qse_pio_init (
		&cgi->pio, httpd->mmgr, (const qse_char_t*)xpath,
		cgi->env, pio_options);
	if (xpath != cgi->path) QSE_MMGR_FREE (httpd->mmgr, xpath);

	if (x <= -1)
	{
		qse_pio_errnum_t errnum;

		errnum = qse_pio_geterrnum (&cgi->pio);

		if (errnum == QSE_PIO_ENOENT) http_errnum = 404;
		else if (errnum == QSE_PIO_EACCES) http_errnum = 403;

		goto oops;
	}
	cgi->pio_inited = 1;
	
	/* set the trigger that the main loop can use this
	 * handle for multiplexing 
	 *
	 * it the output from the child is available, this task
	 * writes it back to the client. so add a trigger for
	 * checking the data availability from the child process */
	task->trigger[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger[0].handle = qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_OUT);
	task->trigger[1].handle = qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_IN);
	task->trigger[2].handle = client->handle;

	if (cgi->reqfwdbuf)
	{
		/* the existence of the forwarding buffer leads to a trigger
		 * for checking data availiability from the client side. */

		if (cgi->req)
		{
			/* there are still things to forward from the client-side. 
			 * i can rely on this relay trigger for task invocation. */
			task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
		}

		if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
		{
			/* there's nothing more to read from the client side but
			 * some contents are already read into the forwarding buffer.
			 * this is possible because the main loop can still read 
			 * between the initializer function (task_init_cgi()) and 
			 * this function. so let's forward it initially. */
			cgi_forward_client_input_to_script (httpd, task, 0);

			/* if the initial forwarding clears the forwarding 
			 * buffer, there is nothing more to forward. 
			 * (nothing more to read from the client side, nothing 
			 * left in the forwarding buffer). if not, this task should
			 * still be invoked for forwarding.
			 */
			if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
			{
				/* since the buffer is not cleared, this task needs
				 * a trigger for invocation. ask the main loop to
				 * invoke this task so long as it is able to write
				 * to the child process */
				task->trigger[1].mask = QSE_HTTPD_TASK_TRIGGER_WRITE;
			}
		}
	}

	task->main = cgi->nph? task_main_cgi_4_nph: task_main_cgi_2;
	return 1;

oops:
	if (cgi->res) 
	{
		qse_mbs_close (cgi->res);
		cgi->res = QSE_NULL;
	}
	if (cgi->script_htrd) 
	{
		qse_htrd_close (cgi->script_htrd);
		cgi->script_htrd = QSE_NULL;
	}

	return (qse_httpd_entask_err (
		httpd, client, task, http_errnum, 
		cgi->method, &cgi->version, cgi->keepalive) == QSE_NULL)? -1: 0;
}

/* TODO: global option or individual paramter for max cgi lifetime 
*        non-blocking pio read ...
*/

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred,
	const qse_httpd_rsrc_cgi_t* cgi,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_cgi_arg_t arg;
	qse_httpd_rsrc_cgi_t rsrc;

	rsrc = *cgi;
	if (rsrc.script == QSE_NULL) rsrc.script = qse_htre_getqpath(req);
	if (rsrc.suffix == QSE_NULL) rsrc.suffix = QSE_MT("");
	if (rsrc.root == QSE_NULL) rsrc.root = QSE_MT("");
	if (rsrc.shebang == QSE_NULL) rsrc.shebang = QSE_MT("");

	arg.path.ptr = rsrc.path;
	arg.path.len = qse_mbslen(rsrc.path);
	arg.script.ptr = rsrc.script;
	arg.script.len = qse_mbslen(rsrc.script);
	arg.suffix.ptr = rsrc.suffix;
	arg.suffix.len = qse_mbslen(rsrc.suffix);
	arg.root.ptr = rsrc.root;
	arg.root.len = qse_mbslen(rsrc.root);
	arg.nph = rsrc.nph;
	arg.shebang.ptr = rsrc.shebang;
	arg.shebang.len = qse_mbslen(rsrc.shebang);
	arg.req = req;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_cgi;
	task.fini = task_fini_cgi;
	task.main = task_main_cgi;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, 
		QSE_SIZEOF(task_cgi_t) + 
		((arg.path.len + 1) * QSE_SIZEOF(*arg.path.ptr)) + 
		((arg.script.len + 1) * QSE_SIZEOF(*arg.script.ptr)) + 
		((arg.suffix.len + 1) * QSE_SIZEOF(*arg.suffix.ptr)) +
		((arg.root.len + 1) * QSE_SIZEOF(*arg.root.ptr)) +
		((arg.shebang.len + 1) * QSE_SIZEOF(*arg.shebang.ptr))
	);
}

