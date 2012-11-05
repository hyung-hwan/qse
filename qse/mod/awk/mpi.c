#include <qse/awk/awk.h>
#include <qse/cmn/str.h>
#include <qse/cmn/main.h>

#if defined(HAVE_MPI)
#    include <mpi.h>
#else
#    error this module needs mpi
#endif

static int fnc_size (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	int rank;

	MPI_Comm_size (MPI_COMM_WORLD, &rank);
	retv = qse_awk_rtx_makeintval (rtx, rank);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_rank (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	int rank;

	MPI_Comm_rank (MPI_COMM_WORLD, &rank);
	retv = qse_awk_rtx_makeintval (rtx, rank);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_hash (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_long_t hv;

	hv = qse_awk_rtx_hashval (rtx, qse_awk_rtx_getarg (rtx, 0));

	retv = qse_awk_rtx_makeintval (rtx, hv);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_assign (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_long_t limit;
	int rx;

	rx = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &limit);
	if (rx >= 0)
	{
		qse_awk_nrflt_t nrflt;
		int size, rank;

		MPI_Comm_size (MPI_COMM_WORLD, &size);
		MPI_Comm_rank (MPI_COMM_WORLD, &rank);
		nrflt.limit = limit;
		nrflt.size = size;
		nrflt.rank = rank;
		qse_awk_rtx_setnrflt (rtx, &nrflt);
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_reduce (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_long_t opidx, lv;
	qse_flt_t rv;
	int n;

	static MPI_Op optab[] =
	{
		MPI_MIN,
		MPI_MAX,
		MPI_SUM,
		MPI_PROD,
		MPI_LAND,
		MPI_LOR
	};

	if (qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 1), &opidx) <= -1 ||
	   (opidx < 0 || opidx >= QSE_COUNTOF(optab)) || 
	   (n = qse_awk_rtx_valtonum (rtx, qse_awk_rtx_getarg (rtx, 0), &lv, &rv)) <= -1) goto softfail;

/* TODO: determine it to be MPI_LONG or MPI_INT, OR MPI_LONG_LONG_INT depending on the size of qse_long_t */
	if (n == 0) 
	{
		qse_long_t lout;
		if (MPI_Allreduce (&lv, &lout, 1, MPI_LONG_LONG_INT, optab[opidx], MPI_COMM_WORLD) != MPI_SUCCESS) goto softfail;
		retv = qse_awk_rtx_makeintval (rtx, lout);
	}
	else
	{
		qse_flt_t fout;
		if (MPI_Allreduce (&rv, &fout, 1, MPI_LONG_DOUBLE, optab[opidx], MPI_COMM_WORLD) != MPI_SUCCESS) goto softfail;
		retv = qse_awk_rtx_makefltval (rtx, fout);
	}
	
	if (retv == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, retv);

	return 0;

softfail:
	/* return without setting the return value.
	 * this intrinsic function will return a nil value when it fails */
	return 0;
}

static int fnc_barrier (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx;
	qse_awk_val_t* retv;

	rx = (MPI_Barrier (MPI_COMM_WORLD) == MPI_SUCCESS)? 0: -1;

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

/* ------------------------------------------------------------------------- */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_int_t info;
};

static fnctab_t fnctab[] =
{
	{ QSE_T("assign"),   { { 1, 1, QSE_NULL }, fnc_assign,  0 } },
	{ QSE_T("barrier"),  { { 0, 0, QSE_NULL }, fnc_barrier, 0 } },
	{ QSE_T("hash"),     { { 1, 1, QSE_NULL }, fnc_hash,    0 } },
	{ QSE_T("rank"),     { { 0, 0, QSE_NULL }, fnc_rank,    0 } },
	{ QSE_T("reduce"),   { { 2, 2, QSE_NULL }, fnc_reduce,  0 } },
	{ QSE_T("size"),     { { 0, 0, QSE_NULL }, fnc_size,    0 } }
};

static inttab_t inttab[] =
{
	{ QSE_T("REDUCE_LAND"),  { 4 } },
	{ QSE_T("REDUCE_LOR"),   { 5 } },
	{ QSE_T("REDUCE_MAX"),   { 1 } },
	{ QSE_T("REDUCE_MIN"),   { 0 } },
	{ QSE_T("REDUCE_PROD"),  { 3 } },
	{ QSE_T("REDUCE_SUM"),   { 2 } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int i;

/* TODO: binary search or something better */
	for (i = 0; i < QSE_COUNTOF(fnctab); i++)
	{
		if (qse_strcmp (fnctab[i].name, name) == 0)
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[i].info;
			return 0;
		}
	}

	for (i = 0; i < QSE_COUNTOF(inttab); i++)
	{
		if (qse_strcmp (inttab[i].name, name) == 0)
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[i].info;
			return 0;
		}
	}

	ea.ptr = name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

QSE_EXPORT int load (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	/*
	mod->ctx...
	 */

	return 0;
}

/* The MPI module is special in that it exports 2 extra symbols - 
 * mpi_init and mpi_fini. These two symbols are intended to be called
 * dynamically using dlopen() or something similar when an application
 * intending to use mpi::xxx starts up. This way, the application doesn't
 * have to be linked to any MPI libraries while this module is linked to
 * an MPI library. If this module doesn't exist, it means MPI is not availble
 * and the module wasn't built. So you can't access mpi::xxx symbols either 
 */

QSE_EXPORT int mpi_init (int argc, qse_achar_t* argv[])
{
	int rx;

	if (MPI_Init (&argc, &argv) != MPI_SUCCESS) rx = -1;
	else
	{
		MPI_Comm_set_errhandler (MPI_COMM_WORLD, MPI_ERRORS_RETURN);
		rx = 0;
	}

	return rx;
}

QSE_EXPORT void mpi_fini (void)
{
	MPI_Finalize ();
}
