/*
 * $Id: awk.c 478 2008-12-12 09:42:32Z baconevi $
 */

#include <ase/awk/awk.h>
#include <ase/cmn/sll.h>
#include <ase/cmn/mem.h>
#include <ase/cmn/chr.h>
#include <ase/cmn/opt.h>

#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>

#define ABORT(label) goto label

#if defined(_WIN32)
	#include <windows.h>
	#include <tchar.h>
	#include <process.h>
	#pragma warning (disable: 4996)
	#pragma warning (disable: 4296)

	#if defined(_MSC_VER) && defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>
	#endif
#else
	#include <unistd.h>
#endif

#define SRCIO_FILE 1
#define SRCIO_STR  2

typedef struct srcio_data_t
{
	int type; /* file or string */

	union
	{
		struct 
		{
			ase_char_t* ptr;
			ase_char_t* cur;
		} str;
		struct 
		{
			ase_sll_t* sll;
			ase_sll_node_t* cur;
			FILE* handle;
		} file;
	} data;
} srcio_data_t;

typedef struct runio_data_t
{
	ase_char_t** icf;
	ase_size_t icf_no;
} runio_data_t;

static void dprint (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

/* custom memory management function */
static void* custom_awk_malloc (void* custom, ase_size_t n)
{
#ifdef _WIN32
	return HeapAlloc ((HANDLE)custom, 0, n);
#else
	return malloc (n);
#endif
}

static void* custom_awk_realloc (void* custom, void* ptr, ase_size_t n)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	return (ptr == NULL)?
		HeapAlloc ((HANDLE)custom, 0, n):
		HeapReAlloc ((HANDLE)custom, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void custom_awk_free (void* custom, void* ptr)
{
#ifdef _WIN32
	HeapFree ((HANDLE)custom, 0, ptr);
#else
	free (ptr);
#endif
}

/* custom miscellaneous functions */
static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int custom_awk_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

ase_awk_t* app_awk = NULL;
ase_awk_run_t* app_run = NULL;

#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		ase_awk_stop (app_run);
		return TRUE;
	}

	return FALSE;
}
#else
static void stop_run (int sig)
{
	signal  (SIGINT, SIG_IGN);
	ase_awk_stop (app_run);
	signal  (SIGINT, stop_run);
}
#endif

static void on_run_start (ase_awk_run_t* run, void* custom)
{
	app_run = run;
	dprint (ASE_T("[AWK ABOUT TO START]\n"));
}

static ase_map_walk_t print_awk_value (
	ase_map_t* map, ase_map_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;
	ase_char_t* str;
	ase_size_t len;

	str = ase_awk_valtostr (run, ASE_MAP_VPTR(pair), 0, ASE_NULL, &len);
	if (str == ASE_NULL)
	{
		dprint (ASE_T("***OUT OF MEMORY***\n"));
	}
	else
	{
		dprint (ASE_T("%.*s = %.*s\n"), 
			(int)ASE_MAP_KLEN(pair), ASE_MAP_KPTR(pair), 
			(int)len, str);
		ase_awk_free (ase_awk_getrunawk(run), str);
	}

	return ASE_MAP_WALK_FORWARD;
}

static void on_run_statement (
	ase_awk_run_t* run, ase_size_t line, void* custom)
{
	/*dprint (L"running %d\n", (int)line);*/
}

static void on_run_return (
	ase_awk_run_t* run, ase_awk_val_t* ret, void* custom)
{
	ase_size_t len;
	ase_char_t* str;

	str = ase_awk_valtostr (run, ret, 0, ASE_NULL, &len);
	if (str == ASE_NULL)
	{
		dprint (ASE_T("[RETURN] - ***OUT OF MEMORY***\n"));
	}
	else
	{
		dprint (ASE_T("[RETURN] - %.*s\n"), (int)len, str);
		ase_awk_free (ase_awk_getrunawk(run), str);
	}

	dprint (ASE_T("[NAMED VARIABLES]\n"));
	ase_map_walk (ase_awk_getrunnvmap(run), print_awk_value, run);
	dprint (ASE_T("[END NAMED VARIABLES]\n"));
}

static void on_run_end (ase_awk_run_t* run, int errnum, void* data)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		dprint (ASE_T("[AWK ENDED WITH AN ERROR]\n"));
		ase_printf (ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"),
			errnum, 
			(unsigned int)ase_awk_getrunerrlin(run),
			ase_awk_getrunerrmsg(run));
	}
	else dprint (ASE_T("[AWK ENDED SUCCESSFULLY]\n"));

	app_run = NULL;
}

/* TODO: remove otab... */
static struct
{
	const ase_char_t* name;
	int opt;
} otab[] =
{
	{ ASE_T("implicit"),    ASE_AWK_IMPLICIT },
	{ ASE_T("explicit"),    ASE_AWK_EXPLICIT },
	{ ASE_T("bxor"),        ASE_AWK_BXOR },
	{ ASE_T("shift"),       ASE_AWK_SHIFT },
	{ ASE_T("idiv"),        ASE_AWK_IDIV },
	{ ASE_T("extio"),       ASE_AWK_EXTIO },
	{ ASE_T("newline"),     ASE_AWK_NEWLINE },
	{ ASE_T("baseone"),     ASE_AWK_BASEONE },
	{ ASE_T("stripspaces"), ASE_AWK_STRIPSPACES },
	{ ASE_T("nextofile"),   ASE_AWK_NEXTOFILE },
	{ ASE_T("crfl"),        ASE_AWK_CRLF },
	{ ASE_T("argstomain"),  ASE_AWK_ARGSTOMAIN },
	{ ASE_T("reset"),       ASE_AWK_RESET },
	{ ASE_T("maptovar"),    ASE_AWK_MAPTOVAR },
	{ ASE_T("pablock"),     ASE_AWK_PABLOCK }
};

static void print_usage (const ase_char_t* argv0)
{
	int j;

	ase_printf (ASE_T("Usage: %s [options] -f sourcefile [ -- ] [datafile]*\n"), argv0);
	ase_printf (ASE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), argv0);
	ase_printf (ASE_T("Where options are:\n"));
	ase_printf (ASE_T(" -f sourcefile   --file=sourcefile\n"));
	ase_printf (ASE_T(" -d deparsedfile --deparsed-file=deparsedfile\n"));
	ase_printf (ASE_T(" -F string       --field-separator=string\n"));

	ase_printf (ASE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < ASE_COUNTOF(otab); j++)
	{
		ase_printf (ASE_T("    -%-20s -no%-20s\n"), otab[j].name, otab[j].name);
	}
}

static int bfn_sleep (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_long_t lv;
	ase_real_t rv;
	ase_awk_val_t* r;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	n = ase_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (ase_long_t)rv;

#ifdef _WIN32
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#else
	n = sleep (lv);	
#endif

	r = ase_awk_makeintval (run, n);
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static void out_of_memory (void)
{
	ase_fprintf (ASE_STDERR, ASE_T("Error: out of memory\n"));	
}

struct argout_t
{
	void*        isp;   /* input source files or string */
	int          ist;  /* input source type */
	ase_size_t   isfl; /* the number of input source files */
	ase_char_t*  osf;  /* output source file */
	ase_char_t** icf;  /* input console files */
	ase_size_t   icfl; /* the number of input console files */
	ase_map_t*   vm;   /* global variable map */
};

static int handle_args (int argc, ase_char_t* argv[], struct argout_t* ao)
{
	static ase_opt_lng_t lng[] = 
	{
		{ ASE_T("implicit"),         0 },
		{ ASE_T("explicit"),         0 },
		{ ASE_T("bxor"),             0 },
		{ ASE_T("shift"),            0 },
		{ ASE_T("idiv"),             0 },
		{ ASE_T("extio"),            0 },
		{ ASE_T("newline"),          0 },
		{ ASE_T("baseone"),          0 },
		{ ASE_T("stripspaces"),      0 },
		{ ASE_T("nextofile"),        0 },
		{ ASE_T("crlf"),             0 },
		{ ASE_T("argstomain"),       0 },
		{ ASE_T("reset"),            0 },
		{ ASE_T("maptovar"),         0 },
		{ ASE_T("pablock"),          0 },

		{ ASE_T(":main"),            ASE_T('m') },
		{ ASE_T(":file"),            ASE_T('f') },
		{ ASE_T(":field-separator"), ASE_T('F') },
		{ ASE_T(":deparsed-file"),   ASE_T('d') },
		{ ASE_T(":assign"),          ASE_T('v') },

		{ ASE_T("help"),             ASE_T('h') }
	};

	static ase_opt_t opt = 
	{
		ASE_T("hm:f:F:d:v:"),
		lng
	};

	ase_cint_t c;

	ase_size_t isfc = 16; /* the capacity of isf */
	ase_size_t isfl = 0; /* number of input source files */

	ase_size_t icfc = 0; /* the capacity of icf */
	ase_size_t icfl = 0;  /* the number of input console files */

	ase_char_t** isf = ASE_NULL; /* input source files */
	ase_char_t* osf = ASE_NULL; /* output source file */
	ase_char_t** icf = ASE_NULL; /* input console files */

	ase_map_t* vm = ASE_NULL;  /* global variable map */

	isf = (ase_char_t**) malloc (ASE_SIZEOF(*isf) * isfc);
	if (isf == ASE_NULL)
	{
		out_of_memory ();
		ABORT (on_error);
	}

	vm = ase_map_open (ASE_NULL, 0, 30, 70); 
	if (vm == ASE_NULL)
	{
		out_of_memory ();
		ABORT (on_error);
	}
	ase_map_setcopier (vm, ASE_MAP_KEY, ASE_MAP_COPIER_INLINE);
	ase_map_setcopier (vm, ASE_MAP_VAL, ASE_MAP_COPIER_INLINE);
	ase_map_setscale (vm, ASE_MAP_KEY, ASE_SIZEOF(ase_char_t));
	ase_map_setscale (vm, ASE_MAP_VAL, ASE_SIZEOF(ase_char_t));

	while ((c = ase_getopt (argc, argv, &opt)) != ASE_CHAR_EOF)
	{
		switch (c)
		{
			case 0:
				ase_printf (ASE_T(">>> [%s] [%s]\n"), opt.lngopt, opt.arg);
				break;

			case ASE_T('h'):
				print_usage (argv[0]);
				if (isf != ASE_NULL) free (isf);
				if (vm != ASE_NULL) ase_map_close (vm);
				return 1;

			case ASE_T('f'):
			{
				if (isfl >= isfc-1) /* -1 for last ASE_NULL */
				{
					ase_char_t** tmp;
					tmp = (ase_char_t**) realloc (isf, ASE_SIZEOF(*isf)*(isfc+16));
					if (tmp == ASE_NULL)
					{
						out_of_memory ();
						ABORT (on_error);
					}

					isf = tmp;
					isfc = isfc + 16;
				}

				isf[isfl++] = opt.arg;
				break;
			}

			case ASE_T('F'):
			{
				ase_printf  (ASE_T("[field separator] = %s\n"), opt.arg);
				break;
			}

			case ASE_T('d'):
			{
				osf = opt.arg;
				break;
			}

			case ASE_T('v'):
			{
				ase_char_t* eq = ase_strchr(opt.arg, ASE_T('='));
				if (eq == ASE_NULL)
				{
					/* INVALID VALUE... */
					ABORT (on_error);
				}

				*eq = ASE_T('\0');

				if (ase_map_upsert (vm, opt.arg, ase_strlen(opt.arg)+1, eq, ase_strlen(eq)+1) == ASE_NULL)
				{
					out_of_memory ();
					ABORT (on_error);
				}
				break;
			}

			case ASE_T('?'):
			{
				if (opt.lngopt)
				{
					ase_printf (ASE_T("Error: illegal option - %s\n"), opt.lngopt);
				}
				else
				{
					ase_printf (ASE_T("Error: illegal option - %c\n"), opt.opt);
				}

				ABORT (on_error);
			}

			case ASE_T(':'):
			{
				if (opt.lngopt)
				{
					ase_printf (ASE_T("Error: bad argument for %s\n"), opt.lngopt);
				}
				else
				{
					ase_printf (ASE_T("Error: bad argument for %c\n"), opt.opt);
				}

				ABORT (on_error);
			}

			default:
				ABORT (on_error);
		}
	}

	isf[isfl] = ASE_NULL;

	if (isfl <= 0)
	{
		if (opt.ind >= argc)
		{
			/* no source code specified */
			ABORT (on_error);
		}

		/* the source code is the string, not from the file */
		ao->ist = ASE_AWK_PARSE_STRING;
		ao->isp = argv[opt.ind++];
	}
	else
	{
		ao->ist = ASE_AWK_PARSE_FILES;
		ao->isp = isf;
	}

	/* the remaining arguments are input console file names */
	icfc = (opt.ind >= argc)? 2: (argc - opt.ind + 1);
	icf = (ase_char_t**) malloc (ASE_SIZEOF(*icf)*icfc);
	if (icf == ASE_NULL)
	{
		out_of_memory ();
		ABORT (on_error);
	}

	if (opt.ind >= argc)
	{
		/* no input(console) file names are specified.
		 * the standard input becomes the input console */
		icf[icfl++] = ASE_T("");
	}
	else
	{	
		do { icf[icfl++] = argv[opt.ind++]; } while (opt.ind < argc);
	}
	icf[icfl] = ASE_NULL;

	ao->osf = osf;
	ao->icf = icf;
	ao->icfl = icfl;
	ao->vm = vm;

	return 0;

on_error:
	if (vm != ASE_NULL) ase_map_close (vm);
	if (icf != ASE_NULL) free (icf);
	if (isf != ASE_NULL) free (isf);
	return -1;
}

typedef struct extension_t
{
	ase_mmgr_t mmgr;
	ase_awk_prmfns_t prmfns;
} 
extension_t;

static void init_awk_extension (ase_awk_t* awk)
{
	extension_t* ext = (extension_t*) ase_awk_getextension(awk);

	ext->mmgr = *ase_awk_getmmgr(awk);
	ase_awk_setmmgr (awk, &ext->mmgr);
	ase_awk_setccls (awk, ASE_CCLS_GETDFL());

	ext->prmfns.pow         = custom_awk_pow;
	ext->prmfns.sprintf     = custom_awk_sprintf;
	ext->prmfns.data = ASE_NULL;

	ase_awk_setprmfns (awk, &ext->prmfns);
}

static ase_awk_t* open_awk (void)
{
	ase_awk_t* awk;
	ase_mmgr_t mmgr;

	memset (&mmgr, 0, ASE_SIZEOF(mmgr));
	mmgr.alloc   = custom_awk_malloc;
	mmgr.realloc = custom_awk_realloc;
	mmgr.free    = custom_awk_free;

#ifdef _WIN32
	mmgr.data = (void*)HeapCreate (0, 1000000, 1000000); /* TODO: get size from xxxx */
	if (mmgr.data == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an awk heap\n"));
		return ASE_NULL;
	}
#else
	mmgr.data = ASE_NULL;
#endif

	awk = ase_awk_open (&mmgr, ASE_SIZEOF(extension_t));
	if (awk == ASE_NULL)
	{
#ifdef _WIN32
		HeapDestroy ((HANDLE)mmgr.data);
#endif
		ase_printf (ASE_T("ERROR: cannot open awk\n"));
		return ASE_NULL;
	}
	
	init_awk_extension (awk);

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	/* TODO: get depth from command line */
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_PARSE | ASE_AWK_DEPTH_EXPR_PARSE, 50);
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_RUN | ASE_AWK_DEPTH_EXPR_RUN, 500);

	/*
	ase_awk_seterrstr (awk, ASE_AWK_EGBLRED, 
		ASE_T("\uC804\uC5ED\uBCC0\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	ase_awk_seterrstr (awk, ASE_AWK_EAFNRED, 
		ASE_T("\uD568\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	*/
	/*ase_awk_setkeyword (awk, ASE_T("func"), 4, ASE_T("FX"), 2);*/

	if (ase_awk_addfunc (awk, 
		ASE_T("sleep"), 5, 0,
		1, 1, ASE_NULL, bfn_sleep) == ASE_NULL)
	{
		ase_awk_close (awk);
#ifdef _WIN32
		HeapDestroy ((HANDLE)mmgr.data);
#endif
		ase_printf (ASE_T("ERROR: cannot add function 'sleep'\n"));
		return ASE_NULL;
	}

	return awk;
}

static void close_awk (ase_awk_t* awk)
{
	extension_t* ext = (extension_t*)ase_awk_getextension(awk);
	
#ifdef _WIN32
	HANDLE heap = (HANDLE)ext->mmgr.data;
#endif

	ase_awk_close (awk);

#ifdef _WIN32
	HeapDestroy (heap);
#endif
}

static int awk_main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;

	ase_awk_srcios_t srcios;
	int i, file_count = 0;
	const ase_char_t* mfn = ASE_NULL;
	int mode = 0;
	int runarg_count = 0;
	ase_awk_runarg_t runarg[128];
	int deparse = 0;
	struct argout_t ao;

	ase_memset (&ao, 0, ASE_SIZEOF(ao));

	i = handle_args (argc, argv, &ao);
	if (i == -1)
	{
		print_usage (argv[0]);
		return -1;
	}
	if (i == 1) return 0;

	runarg[runarg_count].ptr = NULL;
	runarg[runarg_count].len = 0;

	awk = open_awk ();
	if (awk == ASE_NULL) return -1;

	app_awk = awk;

	if (ase_awk_parsesimple (awk, ao.isp, ao.ist, ao.osf) == -1)
	{
		ase_printf (
			ASE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk)
		);

		close_awk (awk);
		return -1;
	}


#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	signal (SIGINT, stop_run);
#endif

	if (ase_awk_runsimple (awk, ao.icf) == -1)
	{
		ase_printf (
			ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"),
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk),
			ase_awk_geterrmsg(awk)
		);

		close_awk (awk);
		return -1;
	}

	close_awk (awk);

	if (ao.ist == ASE_AWK_PARSE_FILES && ao.isp != ASE_NULL) free (ao.isp);
	if (ao.osf != ASE_NULL) free (ao.osf);
	if (ao.icf != ASE_NULL) free (ao.icf);
	if (ao.vm != ASE_NULL) ase_map_close (ao.vm);

	return 0;
}

int ase_main (int argc, ase_achar_t* argv[])
{
	int n;

#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	n = ase_runmain (argc, argv, awk_main);

#if defined(_WIN32) && defined(_DEBUG)
	/*#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif*/
#endif

	return n;
}
