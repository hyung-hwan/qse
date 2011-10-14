
#include <qse/fs/dir.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

int dir_main (int argc, qse_char_t* argv[])
{
	int n;
	qse_dir_t* dir;
	qse_dir_ent_t* ent;

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <directory>\n"), argv[0]);
		return -1;
	}

	dir = qse_dir_open (QSE_NULL, 0, argv[1]);
	if (dir == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: Cannot open directory\n"), argv[1]);
		return -1;
	}

	do
	{
		ent = qse_dir_read (dir);
		if (ent == QSE_NULL) break;

		if (ent->type == QSE_DIR_ENT_DIRECTORY)
			qse_printf (QSE_T("<DIR> %16lu %s\n"), (unsigned long)ent->size, ent->name);
		else
			qse_printf (QSE_T("      %16lu %s\n"), (unsigned long)ent->size, ent->name);
	}
	while (1);

	qse_dir_close (dir);
	return n;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, dir_main);
}

