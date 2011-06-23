/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
 */

#ifndef _QSE_UTL_HTTP_H_
#define _QSE_UTL_HTTP_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/htb.h>

typedef struct qse_http_t qse_http_t;

/*typedef qse_byte_t qse_http_oct_t;*/
typedef qse_mchar_t qse_http_oct_t;

typedef struct qse_http_octb_t qse_http_octb_t;

struct qse_http_octb_t
{
	qse_size_t      capa;
	qse_size_t      size;
	qse_http_oct_t* data;
};

enum qse_http_errnum_t
{
	QSE_HTTP_ENOERR,
	QSE_HTTP_ENOMEM,
	QSE_HTTP_EBADRE,
	QSE_HTTP_EBADHDR,
	QSE_HTTP_EREQCBS
};

typedef enum qse_http_errnum_t qse_http_errnum_t;

enum qse_http_option_t
{
	QSE_HTTP_LEADINGEMPTYLINES = (1 << 0)
};

typedef enum qse_http_option_t qse_http_option_t;

typedef struct qse_http_req_t qse_http_req_t;
typedef struct qse_http_res_t qse_http_res_t;
typedef struct qse_http_rhc_t qse_http_rhc_t;

/* header and contents of request/response */
struct qse_http_rhc_t 
{
	/* header table */
	qse_htb_t hdrtab;

	/* special attributes derived from the header */
	struct
	{
		int chunked;		
		int content_length;
		int connection_close;
		struct
		{
			qse_http_oct_t* ptr;
			qse_size_t      len;
		} content_type;
		struct
		{
			qse_http_oct_t* ptr;
			qse_size_t      len;
		} host;

		int expect_continue;
	} attr;

	qse_http_octb_t con;

	/* if set, the rest of the contents are discarded */
	int discard;
};

struct qse_http_req_t
{
	enum
	{
		QSE_HTTP_REQ_GET,
		QSE_HTTP_REQ_HEAD,
		QSE_HTTP_REQ_POST,
		QSE_HTTP_REQ_PUT,
		QSE_HTTP_REQ_DELETE,
		QSE_HTTP_REQ_TRACE,
		QSE_HTTP_REQ_OPTIONS,
		QSE_HTTP_REQ_CONNECT
	} method;

	struct
	{
		qse_http_oct_t* ptr;
		qse_size_t      len;
	} path;

#if 0
	struct
	{
		qse_http_oct_t* ptr;
		qse_size_t      len;
	} args;
#endif

	struct
	{
		short major;
		short minor;
	} version;

	qse_http_rhc_t* rhc;
};

struct qse_http_res_t
{
	struct
	{
		short major;
		short minor;
	} version;

	int code;

	struct
	{
		qse_http_oct_t* ptr;
		qse_size_t      len;
	} message;

	qse_http_rhc_t* rhc;
};

typedef struct qse_http_recbs_t qse_http_recbs_t;

struct qse_http_recbs_t
{
	int (*request)         (qse_http_t* http, qse_http_req_t* req);
	int (*response)        (qse_http_t* http, qse_http_res_t* rep);
	int (*expect_continue) (qse_http_t* http, qse_http_req_t* req);
};

struct qse_http_t
{
	QSE_DEFINE_COMMON_FIELDS (http)
	qse_http_errnum_t errnum;
	int option;

	qse_http_recbs_t recbs;

	struct
	{
		struct
		{
			int crlf; /* crlf status */
			qse_size_t plen; /* raw request length excluding crlf */
			qse_size_t need; /* number of octets needed for contents */

			struct
			{
				qse_size_t len;
				qse_size_t count;
				int        phase;
			} chunk;
		} s; /* state */


		/* buffers needed to for processing a request */
		struct
		{
			qse_http_octb_t raw; /* buffer to hold raw octets */
			qse_http_octb_t tra; /* buffer for handling trailers */
			qse_http_octb_t pen; /* buffer for raw octets during pending period */
		} b; 

		/* points to the head of the combined header list */
		void* chl;
	} fed; 

	
	enum 
	{
		QSE_HTTP_RETYPE_Q,
		QSE_HTTP_RETYPE_S
	} retype;

	union
	{
		qse_http_req_t q;
		qse_http_res_t s;
	} re;

	qse_http_rhc_t rhc;
};

#ifdef __cplusplus
extern "C" {
#endif

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

void qse_http_clear (
	qse_http_t* http
);

int qse_http_getoption (
	qse_http_t* http	
);

void qse_http_setoption (
	qse_http_t* http,
	int         opts
);

const qse_http_recbs_t* qse_http_getrecbs (
	qse_http_t* http
);

void qse_http_setrecbs (
	qse_http_t*             http,
	const qse_http_recbs_t* recbs
);

/**
 * The qse_http_feed() function accepts http request octets and invokes a 
 * callback function if it has processed a proper http request. 
 */
int qse_http_feed (
	qse_http_t*           http, /**< http */
	const qse_http_oct_t* req,  /**< request octets */
	qse_size_t            len   /**< number of octets */
);

#ifdef __cplusplus
}
#endif

#endif
