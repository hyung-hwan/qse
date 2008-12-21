/*
 * $Id: tgp.h 235 2008-07-05 07:25:54Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_TGP_TGP_H_
#define _QSE_TGP_TGP_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_tgp_t qse_tgp_t;

enum qse_tgp_iocmd_t
{
	QSE_TGP_IO_OPEN = 0,
	QSE_TGP_IO_CLOSE = 1,
	QSE_TGP_IO_READ = 2,
	QSE_TGP_IO_WRITE = 3
};

typedef qse_ssize_t (*qse_tgp_io_t) (
	int cmd, void* arg, qse_char_t* data, qse_size_t count);

#ifdef __cplusplus
extern "C" {
#endif

qse_tgp_t* qse_tgp_open (qse_mmgr_t* mmgr);
void qse_tgp_close (qse_tgp_t* tgp);

void qse_tgp_setassocdata (qse_tgp_t* tgp, void* data);
void* qse_tgp_getassocdata (qse_tgp_t* tgp);

#ifdef __cplusplus
}
#endif

#endif
