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

#ifndef _QSE_HTTP_STDHTTPD_H_
#define _QSE_HTTP_STDHTTPD_H_

#include <qse/http/httpd.h>


typedef int (*qse_httpd_serverstd_makersrc_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_htre_t*         req,
	qse_httpd_rsrc_t*   rsrc
); 

typedef void (*qse_httpd_serverstd_freersrc_t) (
	qse_httpd_t*        httpd, 
	qse_httpd_client_t* client,
	qse_htre_t*         req,
	qse_httpd_rsrc_t*   rsrc
);

enum qse_httpd_serverstd_root_type_t
{
	QSE_HTTPD_SERVERSTD_ROOT_PATH,
	QSE_HTTPD_SERVERSTD_ROOT_NWAD,
	QSE_HTTPD_SERVERSTD_ROOT_TEXT
}; 
typedef enum qse_httpd_serverstd_root_type_t qse_httpd_serverstd_root_type_t;

typedef struct qse_httpd_serverstd_root_t qse_httpd_serverstd_root_t;
struct qse_httpd_serverstd_root_t
{
	qse_httpd_serverstd_root_type_t type;
	union 
	{
		struct
		{
			const qse_mchar_t* val;
			qse_size_t rpl;  /* replacement length */
		} path;
		qse_nwad_t nwad;
		struct
		{
			const qse_mchar_t* ptr;
			const qse_mchar_t* mime;
		} text;
	} u;
};

typedef struct qse_httpd_serverstd_realm_t qse_httpd_serverstd_realm_t;
struct qse_httpd_serverstd_realm_t
{
	const qse_mchar_t* name;
	int authreq;
};

typedef struct qse_httpd_serverstd_auth_t qse_httpd_serverstd_auth_t;
struct qse_httpd_serverstd_auth_t
{
	qse_mcstr_t key;
	int authok;
};

typedef struct qse_httpd_serverstd_cgi_t qse_httpd_serverstd_cgi_t;
struct qse_httpd_serverstd_cgi_t
{
	int cgi: 1;
	int nph: 1;
	const qse_mchar_t* shebang; /* optional, can be #QSE_NULL */
};

typedef struct qse_httpd_serverstd_index_t qse_httpd_serverstd_index_t;
struct qse_httpd_serverstd_index_t
{
	qse_size_t count;
	const qse_mchar_t* files;  /* "value1\0value2\0" */
};

typedef struct qse_httpd_serverstd_ssl_t qse_httpd_serverstd_ssl_t;
struct qse_httpd_serverstd_ssl_t
{
	const qse_mchar_t* certfile;
	const qse_mchar_t* keyfile;
};

enum qse_httpd_serverstd_query_code_t
{
	QSE_HTTPD_SERVERSTD_SSL,            /* qse_httpd_serverstd_ssl_t */

	QSE_HTTPD_SERVERSTD_ROOT,           /* qse_httpd_serverstd_root_t */
	QSE_HTTPD_SERVERSTD_REALM,          /* qse_httpd_serverstd_realm_t */
	QSE_HTTPD_SERVERSTD_AUTH,           /* qse_httpd_serverstd_auth_t */
	QSE_HTTPD_SERVERSTD_ERRHEAD,        /* const qse_mchar_t* */
	QSE_HTTPD_SERVERSTD_ERRFOOT,        /* const qse_mchar_t* */

	QSE_HTTPD_SERVERSTD_DIRHEAD,        /* const qse_mchar_t* */
	QSE_HTTPD_SERVERSTD_DIRFOOT,        /* const qse_mchar_t* */

	QSE_HTTPD_SERVERSTD_INDEX,          /* qse_httpd_serverstd_index_t */
	QSE_HTTPD_SERVERSTD_CGI,            /* qse_httpd_serverstd_cgi_t */
	QSE_HTTPD_SERVERSTD_MIME,           /* const qse_mchar_t* */
	QSE_HTTPD_SERVERSTD_DIRACC,         /* int (http error code) */
	QSE_HTTPD_SERVERSTD_FILEACC,        /* int (http error code) */

};
typedef enum qse_httpd_serverstd_query_code_t qse_httpd_serverstd_query_code_t;


typedef int (*qse_httpd_serverstd_query_t) (
	qse_httpd_t*                     httpd, 
	qse_httpd_server_t*              server,
	qse_htre_t*                      req,
	const qse_mchar_t*               xpath,
	qse_httpd_serverstd_query_code_t code,
	void*                            result
);

enum qse_httpd_serverstd_opt_t
{
	QSE_HTTPD_SERVERSTD_QUERY,     /* qse_httpd_serverstd_query_t */
	QSE_HTTPD_SERVERSTD_MAKERSRC,  /* qse_httpd_serverstd_makersrc_t* */
	QSE_HTTPD_SERVERSTD_FREERSRC   /* qse_httpd_serverstd_freersrc_t* */
};
typedef enum qse_httpd_serverstd_opt_t qse_httpd_serverstd_opt_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_httpd_t* qse_httpd_openstd (
	qse_size_t xtnsize
);

QSE_EXPORT qse_httpd_t* qse_httpd_openstdwithmmgr (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize
);

QSE_EXPORT void* qse_httpd_getxtnstd (
	qse_httpd_t* httpd
);

QSE_EXPORT qse_httpd_server_t* qse_httpd_attachserverstd (
	qse_httpd_t*                   httpd,
	const qse_httpd_server_dope_t* dope,
	qse_size_t                     xtnsize
);

#if 0
QSE_EXPORT qse_httpd_server_t* qse_httpd_attachserverstdwithuri (
	qse_httpd_t*                 httpd,
	const qse_char_t*            uri,
	qse_httpd_server_detach_t    detach,	
	qse_httpd_server_impede_t    impede,	
	qse_httpd_serverstd_query_t  query,	
	qse_size_t                   xtnsize
);
#endif

QSE_EXPORT int qse_httpd_getserverstdopt (
	qse_httpd_t*              httpd,
	qse_httpd_server_t*       server,
	qse_httpd_serverstd_opt_t id,
	void*                     value
);

QSE_EXPORT int qse_httpd_setserverstdopt (
	qse_httpd_t*              httpd,
	qse_httpd_server_t*       server,
	qse_httpd_serverstd_opt_t id,
	const void*               value
);

QSE_EXPORT void* qse_httpd_getserverstdxtn (
	qse_httpd_t*         httpd,
	qse_httpd_server_t*  server
);

QSE_EXPORT int qse_httpd_loopstd (
	qse_httpd_t*       httpd
);

#ifdef __cplusplus
}
#endif

#endif
