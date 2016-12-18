#include <qse/si/fs.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/si/sio.h>
#include <qse/cmn/path.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/fmt.h>
#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

static void print_usage (const qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, QSE_T("Usage: %s [options] source-filename target-filename\n"), qse_basename(argv0));
	qse_fprintf (QSE_STDERR, QSE_T("Options include:\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -f            force\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -o            overwrite\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -p            preserve\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -r            recursive\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -s            symlink\n"));
	qse_fprintf (QSE_STDERR, QSE_T("  -g            glob\n"));
}

static int fs_actcb (qse_fs_t* fs, qse_fs_action_t act, const qse_char_t* srcpath, const qse_char_t* dstpath, qse_uintmax_t total, qse_uintmax_t done)
{
	switch (act)
	{
		case QSE_FS_CPFILE:
			if (total == done) qse_printf (QSE_T("Copied [%s] to [%s]\n"), srcpath, dstpath);
			break;

		case QSE_FS_RMFILE:
			qse_printf (QSE_T("Removed file [%s]\n"), srcpath);
			break;

		case QSE_FS_SYMLINK:
			qse_printf (QSE_T("Symlinked [%s] to [%s]\n"), srcpath, dstpath);
			break;

		case QSE_FS_MKDIR:
			qse_printf (QSE_T("Made directory [%s]\n"), srcpath);
			break;

		case QSE_FS_RMDIR:
			qse_printf (QSE_T("Removed directory [%s]\n"), srcpath);
			break;

		case QSE_FS_RENFILE:
			qse_printf (QSE_T("renamed  [%s] to [%s]\n"), srcpath);
			break;
	}

/*if (qse_strcmp(path, QSE_T("b/c")) == 0) return 0;*/
        return 1;
}


static int fs_main (int argc, qse_char_t* argv[])
{
	qse_fs_t* fs;
	qse_fs_cbs_t cbs;
	int ret = 0;
	qse_cint_t c;
	int cpfile_flags = 0;

	static qse_opt_t opt = 
	{
		QSE_T("foprsg"),
		QSE_NULL
	};

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('f'):
				cpfile_flags |= QSE_FS_CPFILE_FORCE;
				break;

			case QSE_T('o'):
				cpfile_flags |= QSE_FS_CPFILE_REPLACE;
				break;

			case QSE_T('p'):
				cpfile_flags |= QSE_FS_CPFILE_PRESERVE;
				break;

			case QSE_T('r'):
				cpfile_flags |= QSE_FS_CPFILE_RECURSIVE;
				break;

			case QSE_T('s'):
				cpfile_flags |= QSE_FS_CPFILE_SYMLINK;
				break;

			case QSE_T('g'):
				cpfile_flags |= QSE_FS_CPFILE_GLOB;
				break;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, QSE_T("illegal option - '%c'\n"), opt.opt);
				goto wrong_usage;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, QSE_T("bad argument for '%c'\n"), opt.opt);
				goto wrong_usage;

			default:
				goto wrong_usage;
		}
	}

	if (opt.ind  + 2 != argc) goto wrong_usage;

	fs = qse_fs_open (QSE_MMGR_GETDFL(), 0);

	qse_memset (&cbs, 0, QSE_SIZEOF(cbs));
	cbs.actcb = fs_actcb;
	qse_fs_setopt (fs, QSE_FS_CBS, &cbs);

	if (qse_fs_cpfile (fs, argv[opt.ind], argv[opt.ind + 1], cpfile_flags) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot copy file - %d\n"), qse_fs_geterrnum(fs));
		ret = -1;
	}

	qse_fs_close (fs);
	return ret;

wrong_usage:
	print_usage (argv[0]);
	return -1;
}

int main (int argc, qse_achar_t* argv[])
{
	int x;
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		/* .codepage */
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	}
#else
	setlocale (LC_ALL, "");
	/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
#endif

	qse_openstdsios ();

	x = qse_runmain (argc, argv, fs_main);

	qse_closestdsios ();

	return x;
}

