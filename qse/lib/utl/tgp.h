/*
 * $Id$
 */

#ifndef _QSE_LIB_UTL_TGP_H_
#define _QSE_LIB_UTL_TGP_H_

#include <qse/utl/tgp.h>

struct qse_tgp_t
{
	QSE_DEFINE_COMMON_FIELDS (tgp)

	void* assoc_data;
	int errnum;

	struct
	{
		qse_tgp_io_t func;
		void* arg;
	} ih;

	struct 
	{
		qse_tgp_io_t func;
		void* arg;
	} oh;

	struct 
	{
		qse_tgp_io_t func;
		void* arg;
	} rh;

	struct
	{
		qse_size_t pos;
		qse_size_t len;
		qse_char_t ptr[512];
	} ib;

	struct
	{
		qse_size_t len;
		qse_char_t ptr[512];
	} ob;

	struct
	{
		qse_size_t len;
		qse_char_t ptr[512];
	} rb;

	int (*read) (qse_tgp_t* tgp, qse_char_t* buf, int len);
	int (*write) (qse_tgp_t* tgp, const qse_char_t* buf, int len);
	int (*run) (qse_tgp_t* tgp, const qse_char_t* buf, int len);
};

#endif
