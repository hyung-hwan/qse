/* 
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fs.h"

int qse_fs_sysmkdir (qse_fs_t* fs, const qse_fs_char_t* fspath)
{

#if defined(_WIN32)

	if (CreateDirectory (fspath, QSE_NULL) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

#elif defined(__OS2__)

	APIRET rc;

	rc = DosMkDir (fspath, QSE_NULL);
	if (rc != NO_ERROR)
	{
		FILESTATUS3L ffb;
		if (DosQueryPathInfo (fspath, FIL_STANDARDL, &ffb, QSE_SIZEOF(ffb)) == NO_ERROR)
		{
			if (ffb.attrFile & FILE_DIRECTORY) 
			{
				fs->errnum = QSE_FS_EEXIST;
				return -1;
			}
		}

		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

#elif defined(__DOS__)

	if (mkdir (fspath) <= -1)
	{
		struct stat st;
		if (errno != EEXIST && stat (fspath, &st) >= 0)
		{
			if (S_ISDIR(st.st_mode)) 
			{
				fs->errnum = QSE_FS_EEXIST;
				return -1;
			}
		}

		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#else

	if (QSE_MKDIR (fspath, 0777) <= -1) /* TODO: proper mode?? */
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#endif

	return 0;
}

/* --------------------------------------------------------------------- */

static int make_directory_chain (qse_fs_t* fs, qse_fs_char_t* fspath)
{
	qse_fs_char_t* core, * p, c;
	int ret = 0;

	core = get_fspath_core (fspath);
	canon_fspath (core, core, 0);

	if (*core == QSE_FS_T('\0'))
	{
		fs->errnum = QSE_FS_EINVAL;
		return -1;
	}

	p = core;
	if (IS_FSPATHSEP(*p)) p++;
	for (; *p; p++)
	{
		if (IS_FSPATHSEP(*p))
		{
		#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
			/* exclude the separator from the path name */
			c = *p;
			*p = QSE_FS_T('\0');
		#else
			/* include the separater in the path name */
			c = *(p + 1);
			*(p + 1) = QSE_FS_T('\0');
		#endif
			ret = qse_fs_sysmkdir (fs, fspath);
			if (ret <= -1 && fs->errnum != QSE_FS_EEXIST) 
			{
				return -1;
				goto done; /* abort */
			}
		#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
			*p = c;
		#else
			*(p + 1) = c;
		#endif
		}
	}

	if (!IS_FSPATHSEP(*(p - 1))) ret = qse_fs_sysmkdir (fs, fspath);

done:
	return ret;
}

int qse_fs_mkdirmbs (qse_fs_t* fs, const qse_mchar_t* path, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	if (*path == QSE_MT('\0')) 
	{
		fs->errnum = QSE_FS_EINVAL;
		return -1;
	}

	if (flags & QSE_FS_MKDIRMBS_PARENT)
	{
		/* make_directory_chain changes the input path.
		 * ensure to create a modifiable string for it. */
		fspath = qse_fs_dupfspathformbs (fs, path);
		if (!fspath) return -1;

		ret = make_directory_chain (fs, fspath);
	}
	else
	{
		fspath = (qse_fs_char_t*)qse_fs_makefspathformbs (fs, path);
		if (!fspath) return -1;

		ret = qse_fs_sysmkdir (fs, fspath);
	}

	qse_fs_freefspathformbs (fs, path, fspath);

	return ret;
}

int qse_fs_mkdirwcs (qse_fs_t* fs, const qse_wchar_t* path, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	if (*path == QSE_WT('\0')) 
	{
		fs->errnum = QSE_FS_EINVAL;
		return -1;
	}

	if (flags & QSE_FS_MKDIRWCS_PARENT)
	{
		/* make_directory_chain changes the input path.
		 * ensure to create a modifiable string for it. */
		fspath = qse_fs_dupfspathforwcs (fs, path);
		if (!fspath) return -1;

		ret = make_directory_chain (fs, fspath);
	}
	else
	{
		fspath = (qse_fs_char_t*)qse_fs_makefspathforwcs (fs, path);
		if (!fspath) return -1;

		ret = qse_fs_sysmkdir (fs, fspath);
	}

	qse_fs_freefspathforwcs (fs, path, fspath);

	return ret;
}


/* --------------------------------------------------------------------- */
/*
mknodmbs
mkfifombs
mknodwcs
mknodwcs
*/
