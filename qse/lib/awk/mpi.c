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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/awk/mpi.h>
#include "../cmn/mem.h"
#include <mpi.h>

typedef struct xtn_t xtn_t;

struct xtn_t
{
	int gbl_mpi_rank;
	int gbl_mpi_size;
};

typedef struct rxtn_t rxtn_t;
struct rxtn_t
{
	int dummy;
};

qse_awk_t* qse_awk_openmpi (qse_size_t xtnsize)
{
	return qse_awk_openmpiwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

qse_awk_t* qse_awk_openmpiwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_awk_t* awk;

	awk = qse_awk_openstdwithmmgr (
		QSE_MMGR_GETDFL(), QSE_SIZEOF(xtn_t) + xtnsize);

	if (awk)
	{
		xtn_t* xtn;

		xtn = (xtn_t*) qse_awk_getxtnstd (awk);
		QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));

		xtn->gbl_mpi_rank = qse_awk_addgbl (awk, QSE_T("MPI_RANK"), 8);
		xtn->gbl_mpi_size = qse_awk_addgbl (awk, QSE_T("MPI_SIZE"), 8);
		if (xtn->gbl_mpi_rank <= -1 || xtn->gbl_mpi_size <= -1)
		{	
			qse_awk_close (awk);
			return QSE_NULL;
		}
	}

	return awk;
}

void* qse_awk_getxtnmpi (qse_awk_t* awk)
{
     return (void*)((xtn_t*)qse_awk_getxtnstd(awk) + 1);
}

int qse_awk_parsempi (
	qse_awk_t* awk, qse_awk_parsempi_t* in, qse_awk_parsempi_t* out)
{
	return qse_awk_parsestd (awk, in, out);
}

qse_awk_rtx_t* qse_awk_rtx_openmpi (
	qse_awk_t*             awk,
	qse_size_t             xtnsize,
	const qse_char_t*      id,
	const qse_char_t*const icf[],
	const qse_char_t*const ocf[],
	qse_cmgr_t*            cmgr)
{
	qse_awk_rtx_t* rtx;

	rtx = qse_awk_rtx_openstd (
		awk, QSE_SIZEOF(rxtn_t) + xtnsize, id, icf, ocf, cmgr);
	if (rtx)
	{
		int rank, size;

		xtn_t* xtn;
		rxtn_t* rxtn;
		qse_awk_val_t* v_tmp;
	
		xtn = (xtn_t*) qse_awk_getxtnstd (awk);
		rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

		QSE_MEMSET (rxtn, 0, QSE_SIZEOF(*rxtn));

		MPI_Comm_rank (MPI_COMM_WORLD, &rank);
		MPI_Comm_size (MPI_COMM_WORLD, &size);

		v_tmp = qse_awk_rtx_makeintval (rtx, rank);
		if (v_tmp == QSE_NULL)
		{
			qse_awk_rtx_close (rtx);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (rtx, v_tmp);
		qse_awk_rtx_setgbl (rtx, xtn->gbl_mpi_rank, v_tmp);
		qse_awk_rtx_refdownval (rtx, v_tmp);

		v_tmp = qse_awk_rtx_makeintval (rtx, size);
		if (v_tmp == QSE_NULL)
		{
			qse_awk_rtx_close (rtx);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (rtx, v_tmp);
		qse_awk_rtx_setgbl (rtx, xtn->gbl_mpi_size, v_tmp);
		qse_awk_rtx_refdownval (rtx, v_tmp);
	}

	return rtx;
}

void* qse_awk_rtx_getxtnmpi (qse_awk_rtx_t* rtx)
{
	return (void*)((rxtn_t*)qse_awk_rtx_getxtnstd(rtx) + 1);
}

qse_cmgr_t* qse_awk_rtx_getcmgrmpi (
	qse_awk_rtx_t* rtx, const qse_char_t* ioname)
{
	return qse_awk_rtx_getcmgrstd (rtx, ioname);
}

