/*
 * $Id$
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licentgp under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
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
