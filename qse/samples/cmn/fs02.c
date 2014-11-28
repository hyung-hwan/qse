#include <qse/cmn/fs.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/path.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <locale.h>


static int fs_del (qse_fs_t* fs, const qse_char_t* path)
{
	qse_printf (QSE_T("Deleting [%s]\n"), path);
/*if (qse_strcmp(path, QSE_T("b/c")) == 0) return 0;*/
	return 1;
}

static void print_usage (const qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, QSE_T("Usage: %s command filename\n"), qse_basename(argv0));
	qse_fprintf (QSE_STDERR, QSE_T("Command is one of rmfile | rmfile-r | rmdir | rmdir-r | mkdir | mkdir-p\n"));
	qse_fprintf (QSE_STDERR, QSE_T("Filename is a pattern for delXXX\n"));
}

static int fs_main (int argc, qse_char_t* argv[])
{
	qse_fs_t* fs;
	qse_fs_cbs_t cbs;
	int ret = 0;

	if (argc != 3)
	{
		print_usage (argv[0]);
		return -1;
	}
	fs = qse_fs_open (QSE_MMGR_GETDFL(), 0);

	qse_memset (&cbs, 0, QSE_SIZEOF(cbs));
	cbs.del = fs_del;
	qse_fs_setopt (fs, QSE_FS_CBS, &cbs);

	if (qse_strcmp(argv[1], QSE_T("rmfile")) == 0)
	{
		if (qse_fs_delfile (fs, argv[2], QSE_FS_DELFILEMBS_GLOB) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot delete files - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else if (qse_strcmp(argv[1], QSE_T("rmfile-r")) == 0)
	{
		if (qse_fs_delfile (fs, argv[2], QSE_FS_DELFILE_GLOB | QSE_FS_DELFILE_RECURSIVE) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot delete files - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else if (qse_strcmp (argv[1], QSE_T("rmdir")) == 0)
	{
		if (qse_fs_deldir (fs, argv[2], QSE_FS_DELDIR_GLOB) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot delete directories - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else if (qse_strcmp (argv[1], QSE_T("rmdir-r")) == 0)
	{
		if (qse_fs_deldir (fs, argv[2], QSE_FS_DELDIR_GLOB | QSE_FS_DELDIR_RECURSIVE) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot delete directories - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else if (qse_strcmp (argv[1], QSE_T("mkdir")) == 0)
	{
		if (qse_fs_mkdir (fs, argv[2], 0) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot make directory - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else if (qse_strcmp (argv[1], QSE_T("mkdir-p")) == 0)
	{
		if (qse_fs_mkdir (fs, argv[2], QSE_FS_MKDIR_PARENT) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot make directory - %d\n"), qse_fs_geterrnum(fs));
			ret = -1;
		}
	}
	else
	{
		print_usage (argv[0]);
		ret = -1;
	}

	qse_fs_close (fs);
	return ret;
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
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	qse_openstdsios ();

	x = qse_runmain (argc, argv, fs_main);

	qse_closestdsios ();

	return x;
}

