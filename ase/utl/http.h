/*
 * $Id: http.h 116 2008-03-03 11:15:37Z baconevi $
 */

#ifndef _ASE_UTL_HTTP_H_
#define _ASE_UTL_HTTP_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

/* returns the type of http method */
typedef struct ase_http_req_t ase_http_req_t;
typedef struct ase_http_hdr_t ase_http_hdr_t;

struct ase_http_req_t
{
	ase_char_t* method;

	struct
	{
		ase_char_t* ptr;
		ase_size_t len;
	} path;

	struct
	{
		ase_char_t* ptr;
		ase_size_t len;
	} args;

	struct
	{
		char major;
		char minor;
	} vers;
};

struct ase_http_hdr_t
{
	struct
	{
		ase_char_t* ptr;
		ase_size_t len;
	} name;

	struct
	{
		ase_char_t* ptr;
		ase_size_t len;
	} value;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_char_t* ase_parsehttpreq (ase_char_t* buf, ase_http_req_t* req);
ase_char_t* ase_parsehttphdr (ase_char_t* buf, ase_http_hdr_t* hdr);

#ifdef __cplusplus
}
#endif

#endif
