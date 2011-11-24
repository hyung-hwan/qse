
#include <qse/fs/dir.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

static void list (qse_dir_t* dir, const qse_char_t* name)
{
	qse_dir_ent_t* ent;

	if (qse_dir_change (dir, name) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: Cannot change directory to %s - %s\n"), name, qse_dir_geterrmsg(dir));
		return;
	}	

	qse_printf (QSE_T("----------------------------------------------------------------\n"), dir->curdir);
	qse_printf (QSE_T("CURRENT DIRECTORY: [%s]\n"), dir->curdir);
	qse_printf (QSE_T("----------------------------------------------------------------\n"), dir->curdir);

	do
	{
		qse_btime_t bt;

		ent = qse_dir_read (dir, QSE_DIR_ENT_SIZE | QSE_DIR_ENT_TYPE | QSE_DIR_ENT_TIME);
		if (ent == QSE_NULL) 
		{
			qse_dir_errnum_t e = qse_dir_geterrnum(dir);
			if (e != QSE_DIR_ENOERR)
				qse_fprintf (QSE_STDERR, QSE_T("Error: Read error - %s\n"), qse_dir_geterrmsg(dir));
			break;
		}

		qse_localtime (ent->time.modify, &bt);
		qse_printf (QSE_T("%s %16lu %04d-%02d-%02d %02d:%02d %s\n"), 
			((ent->type == QSE_DIR_ENT_SUBDIR)? QSE_T("<D>"): QSE_T("   ")),
			(unsigned long)ent->size, 
			bt.year + QSE_BTIME_YEAR_BASE, bt.mon+1, bt.mday, bt.hour, bt.min,
			ent->name.base
		);
	}
	while (1);

}

int dir_main (int argc, qse_char_t* argv[])
{
	qse_dir_t* dir;

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <directory>\n"), argv[0]);
		return -1;
	}

	dir = qse_dir_open (QSE_NULL, 0);
	if (dir == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: Cannot open directory\n"), argv[1]);
		return -1;
	}

	list (dir, argv[1]);
	list (dir, QSE_T(".."));

	qse_dir_close (dir);
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, dir_main);
}

