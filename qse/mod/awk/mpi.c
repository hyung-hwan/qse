#include <qse/awk/std.h>
#include <qse/cmn/str.h>

#include <mpi.h>

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

		nrflt.limit = limit;
//		nrflt.size = rxtn->size;
//		nrflt.rank = rxtn->rank;
		qse_awk_rtx_setnrflt (rtx, &nrflt);
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

#if 0
static int fnc_reduce (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
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
#endif

static int fnc_barrier (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx;
	qse_awk_val_t* retv;
//	rxtn_t* rxtn;

//	rxtn = (rxtn_t*) qse_awk_rtx_getxtnstd (rtx);

//	x = (MPI_Barrier (rxtn->comm) == MPI_SUCCESS)? 0: -1;
	rx = (MPI_Barrier (MPI_COMM_WORLD) == MPI_SUCCESS)? 0: -1;

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_init (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx;
	qse_awk_val_t* retv;

     /* I didn't manage to find a good way to change the
      * default error handler to MPI_ERRORS_RETURN. 
      * so MPI_Init() will simply abort the program if it fails */
//	if (MPI_Init (&argc, &argv) != MPI_SUCCESS) rx = -1;
//	else
//	{
		MPI_Comm_set_errhandler (MPI_COMM_WORLD, MPI_ERRORS_RETURN);
		rx = 0;
//	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_fini (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx;
	qse_awk_val_t* retv;

	MPI_Finalize ();
	rx = 0;

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
	{ QSE_T("assign"),   { { 1, 1 }, fnc_assign  } },
	{ QSE_T("barrier"),  { { 0, 0 }, fnc_barrier } },
	{ QSE_T("fini"),     { { 0, 0 }, fnc_fini    } },
	{ QSE_T("hash"),     { { 1, 1 }, fnc_hash    } },
	{ QSE_T("init"),     { { 0, 0 }, fnc_init    } }
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

#if 0
	for (i = 0; i < QSE_COUNTOF(inttab); i++)
	{
		if (qse_strcmp (inttab[i].name, name) == 0)
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[i].info;
			return 0;
		}
	}
#endif

	ea.ptr = name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	return 0;
}

void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int load (qse_awk_mod_t* mod, qse_awk_t* awk)
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

