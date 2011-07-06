/*
 * $Id$
 */

#include <qse/net/htre.h>
#include "../cmn/mem.h"

qse_htre_t* qse_htre_init (qse_htre_t* re, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (re, 0, QSE_SIZEOF(*re));
	re->mmgr = mmgr;	

	if (qse_htb_init (&re->hdrtab, mmgr, 60, 70, 1, 1) == QSE_NULL)
	{
		return QSE_NULL;
	}

	qse_mbs_init (&re->content, mmgr, 0);

	qse_mbs_init (&re->qpath_or_smesg, mmgr, 0);
	qse_mbs_init (&re->qparamstr, mmgr, 0);

	return re;
}

void qse_htre_fini (qse_htre_t* re)
{
	qse_mbs_fini (&re->qparamstr);
	qse_mbs_fini (&re->qpath_or_smesg);
	qse_mbs_fini (&re->content);
	qse_htb_fini (&re->hdrtab);
}

void qse_htre_clear (qse_htre_t* re)
{
	QSE_MEMSET (&re->version, 0, QSE_SIZEOF(re->version));
	QSE_MEMSET (&re->attr, 0, QSE_SIZEOF(re->attr));

	qse_htb_clear (&re->hdrtab);

	qse_mbs_clear (&re->content);
	qse_mbs_clear (&re->qpath_or_smesg);
	qse_mbs_clear (&re->qparamstr);

	re->discard = 0;
}

int qse_htre_setbuf (
	qse_htre_t* re, qse_htob_t* buf, const qse_htos_t* str)
{
	qse_mbs_clear (buf);
	return (qse_mbs_ncat (buf, str->ptr, str->len) == (qse_size_t)-1)? -1: 0;
}

void qse_htre_getbuf (
	qse_htre_t* re, const qse_htob_t* buf, qse_htos_t* str)
{
	str->ptr = QSE_MBS_PTR(buf);
	str->len = QSE_MBS_LEN(buf);
}

int qse_htre_setqparamstr (qse_htre_t* re, const qse_htoc_t* str)

{
	return (qse_mbs_cpy (&re->qparamstr, str) == (qse_size_t)-1)? -1: 0;
}


