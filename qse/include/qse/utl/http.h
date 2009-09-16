/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
 */

#ifndef _QSE_UTL_HTTP_H_
#define _QSE_UTL_HTTP_H_

#include <qse/types.h>
#include <qse/macros.h>

/* returns the type of http method */
typedef struct qse_http_req_t qse_http_req_t;
typedef struct qse_http_hdr_t qse_http_hdr_t;

struct qse_http_req_t
{
	qse_char_t* method;

	struct
	{
		qse_char_t* ptr;
		qse_size_t len;
	} path;

	struct
	{
		qse_char_t* ptr;
		qse_size_t len;
	} args;

	struct
	{
		char major;
		char minor;
	} vers;
};

struct qse_http_hdr_t
{
	qse_cstr_t name;
	qse_cstr_t value;
};

#ifdef __cplusplus
extern "C" {
#endif

qse_char_t* qse_parsehttpreq (qse_char_t* buf, qse_http_req_t* req);
qse_char_t* qse_parsehttphdr (qse_char_t* buf, qse_http_hdr_t* hdr);

#ifdef __cplusplus
}
#endif

#endif
