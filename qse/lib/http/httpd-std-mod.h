

static int mod_open (qse_httpd_t* httpd, qse_httpd_mod_t* mod)
{
#if defined(USE_LTDL)
	void* h;

	h = lt_dlopenext (mod->fullname);
	if (h == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	mod->handle = h;
	return 0;


#elif defined(_WIN32)

	HMODULE h;

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));

	h = LoadLibrary (mod->fullname);
	if (h == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
		return -1;
	}

	mod->handle = h;
	return 0;

#elif defined(__OS2__)

	HMODULE h;
	char errbuf[CCHMAXPATH];
	APIRET rc;

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));

	/* DosLoadModule() seems to have severe limitation on 
	 * the file name it can load (max-8-letters.xxx) */
	rc = DosLoadModule (errbuf, QSE_COUNTOF(errbuf) - 1, modpath, &h);
	if (rc != NO_ERROR) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		return -1;
	}

	if (h == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOENT); /* is this error code ok? */
		return -1;
	}

	mod->handle = h;
	return 0;


#elif defined(__DOS__)

	/* the DOS code here is not generic enough. it's for a specific
	 * dos-extender only. the best is to enable QSE_ENABLE_STATIC_MODULE
	 * when building for DOS. */
	void* h;

	h = LoadModule (modpath);
	if (h == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	mod->handle = h;
	return 0;

#else
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#endif
}

static void mod_close (qse_httpd_t* httpd, qse_httpd_mod_t* mod)
{
#if defined(USE_LTDL)
	lt_dlclose (mod->handle);
#elif defined(_WIN32)
	FreeLibrary ((HMODULE)mod->handle);
#elif defined(__OS2__)
	DosFreeModule ((HMODULE)mod->handle);
#elif defined(__DOS__)
	FreeModule (mod->handle);
#else
	/* nothing to do */
#endif
}

static void* mod_symbol (qse_httpd_t* httpd, qse_httpd_mod_t* handle, const qse_char_t* name)
{
	void* s;
	qse_mchar_t* mname;

#if defined(QSE_CHAR_IS_MCHAR)
	mname = name;
#else
	mname = qse_wcstombsdup (name, QSE_NULL, httpd->mmgr);
	if (!mname)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return QSE_NULL;
	}
#endif

#if defined(USE_LTDL)
	s = lt_dlsym (mod->handle, mname);
	if (s == QSE_NULL) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
#elif defined(_WIN32)
	s = GetProcAddress ((HMODULE)mod->handle, mname);
	if (s == QSE_NULL) qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
#elif defined(__OS2__)
	{
		APIRET rc;
		rc = DosQueryProcAddr ((HMODULE)mod->handle, 0, mname, (PFN*)&s);
		if (rc != NO_ERROR) 
		{
			qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
			s = QSE_NULL;
		}
	}
#elif defined(__DOS__)
	s = GetProcAddress (mod->handle, mname);
	if (s == QSE_NULL) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
#else
	s = QSE_NULL;
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
#endif

#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
#else
	QSE_MMGR_FREE (httpd->mmgr, mname);
#endif

	return s;
}

