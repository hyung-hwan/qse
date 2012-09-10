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
#include <qse/cmn/mbwc.h>
#include "../cmn/mem.h"

#include <mpi.h>

typedef struct xtn_t xtn_t;

struct xtn_t
{
	int gbl_mpi[9];
};

typedef struct rxtn_t rxtn_t;
struct rxtn_t
{
	MPI_Comm comm;
	int size;
	int rank;
};

qse_awk_t* qse_awk_openmpi (qse_size_t xtnsize)
{
	return qse_awk_openmpiwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_openmpiwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_awk_t* awk;

	awk = qse_awk_openstdwithmmgr (
		QSE_MMGR_GETDFL(), QSE_SIZEOF(xtn_t) + xtnsize);

	if (awk)
	{
		xtn_t* xtn;
		qse_size_t i;

		xtn = (xtn_t*) qse_awk_getxtnstd (awk);
		QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));

		xtn->gbl_mpi[0] = qse_awk_addgbl (awk, QSE_T("MPI_HOST"), 8);
		xtn->gbl_mpi[1] = qse_awk_addgbl (awk, QSE_T("MPI_SIZE"), 8);
		xtn->gbl_mpi[2] = qse_awk_addgbl (awk, QSE_T("MPI_RANK"), 8);

		xtn->gbl_mpi[3] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_MIN"),  14);
		xtn->gbl_mpi[4] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_MAX"),  14);
		xtn->gbl_mpi[5] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_SUM"),  14);
		xtn->gbl_mpi[6] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_PROD"), 15);
		xtn->gbl_mpi[7] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_LAND"), 15);
		xtn->gbl_mpi[8] = qse_awk_addgbl (awk, QSE_T("MPI_REDUCE_LOR"),  14);

		for (i = 0; i < QSE_COUNTOF(xtn->gbl_mpi); i++) 
		{
			if (xtn->gbl_mpi[i] <= -1) 
			{
				qse_awk_close (awk);
				return QSE_NULL;
			}
		}

		if (add_functions (awk) <= -1)
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
		xtn_t* xtn;
		rxtn_t* rxtn;
		qse_size_t i;
	
		xtn = (xtn_t*) qse_awk_getxtnstd (awk);
		rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

		QSE_MEMSET (rxtn, 0, QSE_SIZEOF(*rxtn));
		rxtn->comm = MPI_COMM_WORLD;

		/* set the value of some MPI constants */
		for (i = 0; i < QSE_COUNTOF(xtn->gbl_mpi); i++)
		{
			int iv;
			qse_awk_val_t* v_tmp;

			switch (i)
			{
				case 0: /* MPI_HOST */
				{
					char buf[MPI_MAX_PROCESSOR_NAME];
					int len;
#if defined(QSE_CHAR_IS_MCHAR)
					/* nothing */
#else
					qse_mmgr_t* mmgr;
					qse_char_t* tmp;	
#endif
					if (MPI_Get_processor_name(buf, &len) != MPI_SUCCESS)
					{
						qse_awk_rtx_close (rtx);
						qse_awk_seterrnum (awk, QSE_AWK_ESYSERR, QSE_NULL);	
						return QSE_NULL;
					}
#if defined(QSE_CHAR_IS_MCHAR)
					v_tmp = qse_awk_rtx_makestrval (rtx, buf, len);
#else
		
					mmgr = qse_awk_getmmgr(awk);
					tmp = qse_mbstowcsdup (buf, mmgr);
					if (tmp == QSE_NULL)
					{
						qse_awk_rtx_close (rtx);
						qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);	
						return QSE_NULL;
					}
				
					v_tmp = qse_awk_rtx_makestrval0 (rtx, tmp);
					QSE_MMGR_FREE (mmgr, tmp);
#endif
					break;
				}

				case 1: /* MPI_SIZE */
					if (MPI_Comm_size (rxtn->comm, &rxtn->size) != MPI_SUCCESS)
					{
						qse_awk_rtx_close (rtx);
						qse_awk_seterrnum (awk, QSE_AWK_ESYSERR, QSE_NULL);	
						return QSE_NULL;
					}
					v_tmp = qse_awk_rtx_makeintval (rtx, rxtn->size);
					break;

				case 2: /* MPI_RANK */
					if (MPI_Comm_rank (rxtn->comm, &rxtn->rank) != MPI_SUCCESS)
					{
						qse_awk_rtx_close (rtx);
						qse_awk_seterrnum (awk, QSE_AWK_ESYSERR, QSE_NULL);	
						return QSE_NULL;
					}
					v_tmp = qse_awk_rtx_makeintval (rtx, rxtn->rank);
					break;

				default: /* MPI_REDUCE_XXXX */
					v_tmp = qse_awk_rtx_makeintval (rtx, i - 3);
					break;
			}

			if (v_tmp == QSE_NULL)
			{
				qse_awk_rtx_close (rtx);
				return QSE_NULL;
			}
	
			qse_awk_rtx_refupval (rtx, v_tmp);
			qse_awk_rtx_setgbl (rtx, xtn->gbl_mpi[i], v_tmp);
			qse_awk_rtx_refdownval (rtx, v_tmp);
		}
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

static int fnc_hash (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* tmp, * a0;
	qse_long_t hv;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	hv = qse_awk_rtx_hashval (rtx, a0);

	tmp = qse_awk_rtx_makeintval (rtx, hv);

	if (tmp == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, tmp);

	return 0;
}

static int fnc_assign (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* tmp, * a0;
	qse_long_t lv;
	rxtn_t* rxtn;
	qse_awk_nrflt_t nrflt;

	rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	if (qse_awk_rtx_valtolong (rtx, a0, &lv) <= -1) return -1;

	tmp = qse_awk_rtx_makeintval (rtx, lv);

	nrflt.limit = lv;
	nrflt.size = rxtn->size;
	nrflt.rank = rxtn->rank;
	qse_awk_rtx_setnrflt (rtx, &nrflt);

	if (tmp == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, tmp);

	return 0;
}

static int fnc_reduce (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* tmp, * a0, * a1;
	qse_long_t opidx, lv;
	qse_flt_t rv;
	int n;
	rxtn_t* rxtn;

	static MPI_Op optab[] =
	{
		MPI_MIN,
		MPI_MAX,
		MPI_SUM,
		MPI_PROD,
		MPI_LAND,
		MPI_LOR
	};

	rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	if (qse_awk_rtx_valtolong (rtx, a1, &opidx) <= -1) return -1;
	if (opidx < 0 || opidx >= QSE_COUNTOF(optab)) goto softfail;
	if ((n = qse_awk_rtx_valtonum (rtx, a0, &lv, &rv)) <= -1) return -1;

/* TODO: determine it to be MPI_LONG or MPI_INT, OR MPI_LONG_LONG_INT depending on the size of qse_long_t */
/* TODO: how to tell normal -1 from the soft failure??? */
	if (n == 0) 
	{
		qse_long_t lout;
		if (MPI_Allreduce (&lv, &lout, 1, MPI_LONG_LONG_INT, optab[opidx], rxtn->comm) != MPI_SUCCESS) goto softfail;
		tmp = qse_awk_rtx_makeintval (rtx, lout);
	}
	else
	{
		qse_flt_t fout;
		if (MPI_Allreduce (&rv, &fout, 1, MPI_LONG_DOUBLE, optab[opidx], rxtn->comm) != MPI_SUCCESS) goto softfail;
		tmp = qse_awk_rtx_makefltval (rtx, fout);
	}
	
	if (tmp == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, tmp);

	return 0;

softfail:
	tmp = qse_awk_rtx_makeintval (rtx, (qse_long_t)-1);
	if (tmp == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, tmp);
	return 0;
}

static int fnc_barrier (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	int x;
	qse_awk_val_t* tmp;
	rxtn_t* rxtn;

	rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

	x = (MPI_Barrier (rxtn->comm) == MPI_SUCCESS)? 0: -1;

	tmp = qse_awk_rtx_makeintval (rtx, x);
	if (tmp == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, tmp);
	return 0;
}

static int add_functions (qse_awk_t* awk)
{
	if (qse_awk_addfnc (awk, QSE_T("mpi_hash"), 8, 0, 1, 1, QSE_NULL, fnc_hash) == QSE_NULL) return -1;
	if (qse_awk_addfnc (awk, QSE_T("mpi_assign"), 10, 0, 1, 1, QSE_NULL, fnc_assign) == QSE_NULL) return -1;
	if (qse_awk_addfnc (awk, QSE_T("mpi_reduce"), 10, 0, 2, 2, QSE_NULL, fnc_reduce) == QSE_NULL) return -1;
	if (qse_awk_addfnc (awk, QSE_T("mpi_barrier"), 11, 0, 0, 0, QSE_NULL, fnc_barrier) == QSE_NULL) return -1;
     return 0;
}
