/*
 * $Id$
 */
#include <qse/http/htre.h>
#include "../cmn/mem.h"

qse_htre_t* qse_htre_init (qse_htre_t* re, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (re, 0, QSE_SIZEOF(*re));
	re->mmgr = mmgr;	

	if (qse_htb_init (&re->hdrtab, mmgr, 60, 70, 1, 1) == QSE_NULL)
	{
		return QSE_NULL;
	}

	if (qse_mbs_init (&re->content, mmgr, 0) == QSE_NULL)
	{
		return QSE_NULL;
	}

	return re;
}

void qse_htre_fini (qse_htre_t* re)
{
	qse_mbs_fini (&re->content);
	qse_htb_fini (&re->hdrtab);
}

void qse_htre_clear (qse_htre_t* re)
{
	QSE_MEMSET (&re->version, 0, QSE_SIZEOF(re->version));
	QSE_MEMSET (&re->re, 0, QSE_SIZEOF(re->re));
	QSE_MEMSET (&re->attr, 0, QSE_SIZEOF(re->attr));

	qse_htb_clear (&re->hdrtab);
	qse_mbs_clear (&re->content);

	re->discard = 0;
}

static QSE_INLINE int xdigit_to_num (qse_htoc_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
	if (c >= 'a' && c <= 'z') return c - 'a' + 10;
	return -1;
}

int qse_htre_decodereqpath (qse_htre_t* re, int )
{
	qse_htoc_t* p = re->re.quest.path.ptr;
	qse_htoc_t* tmp = re->re.quest.path.ptr;

	while (*p != '\0')
	{
		if (*p == '%')
		{
			int q = xdigit_to_num(*(p+1));
			int w = xdigit_to_num(*(p+2));

			if (q >= 0 && w >= 0)
			{
				int t = (q << 4) + w;
				if (t == 0)
				{
					/* percent enconding contains a null character */
					return -1;
				}

				*tmp++ = t;
				p += 3;
			}
			else *tmp++ = *p++;
		}
		else if (*p == '?')
		{
#if 0
			if (!http->re.re.quest.args.ptr)
			{
				/* ? must be explicit to be a argument instroducer. 
				 * %3f is just a literal. */
				http->re.re.quest.path.len = tmp - http->re.re.quest.path.ptr;
				*tmp++ = '\0';
				http->re.re.quest.args.ptr = tmp;
				p++;
			}
			else *tmp++ = *p++;
#endif
		}
		else *tmp++ = *p++;
	}
	*tmp = '\0';

	return 0;
}


