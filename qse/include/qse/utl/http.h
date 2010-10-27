/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
 */

#ifndef _QSE_UTL_HTTP_H_
#define _QSE_UTL_HTTP_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/htb.h>


typedef struct qse_http_octb_t qse_http_octb_t;

struct qse_http_octb_t
{
	qse_size_t  capa;
	qse_size_t  size;
	qse_byte_t* data;
};


enum qse_http_errnum_t
{
	QSE_HTTP_ENOERR,
	QSE_HTTP_ENOMEM,
	QSE_HTTP_EBADREQ,
	QSE_HTTP_EBADHDR
};

typedef enum qse_http_errnum_t qse_http_errnum_t;

/*
struct qse_http_req_t
{
	enum
	{
		QSE_HTTP_REQ_HEAD,
		QSE_HTTP_REQ_GET,
		QSE_HTTP_REQ_POST
	} method;

	qse_char_t path[];
};
*/

typedef struct qse_http_t qse_http_t;

struct qse_http_t
{
	QSE_DEFINE_COMMON_FIELDS (http)
	qse_http_errnum_t errnum;

	struct
	{
		//qse_size_t pending;

		int crlf; /* crlf status */
		qse_size_t plen; /* raw request length excluding crlf */
	} state;

	struct
	{
		qse_http_octb_t raw;

		enum
		{
			QSE_HTTP_REQ_GET,
			QSE_HTTP_REQ_HEAD,
			QSE_HTTP_REQ_POST
		} method;

		struct
		{
			qse_byte_t* ptr;
			qse_size_t  len;
		} path;

		struct
		{
			qse_byte_t* ptr;
			qse_size_t  len;
		} args;

		struct
		{
			short major;
			short minor;
		} version;

		qse_htb_t hdr;
	} req;

};

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

QSE_DEFINE_COMMON_FUNCTIONS (http)

/**
 * The qse_http_open() function creates a http processor.
 */
qse_http_t* qse_http_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_http_close() function destroys a http processor.
 */
void qse_http_close (
	qse_http_t* http 
);

qse_http_t* qse_http_init (
	qse_http_t* http,
	qse_mmgr_t* mmgr
);

void qse_http_fini (
	qse_http_t* http
);

#ifdef __cplusplus
}
#endif

#endif
