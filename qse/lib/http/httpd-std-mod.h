/* Override the global definition QSE_ENABLE_STATIC_MODULE
 * for httpd on platforms with mature dynamic loading support. 
 */
#if defined(QSE_ENABLE_STATIC_MODULE) && \
    (defined(USE_LTDL) || defined(_WIN32) || defined(__OS2__))
#	undef QSE_ENABLE_STATIC_MODULE
#endif

static void* mod_open (qse_httpd_t* httpd, const qse_char_t* sysname)
{
#if defined(QSE_ENABLE_STATIC_MODULE)

	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return QSE_NULL;

#elif defined(USE_LTDL)
	void* h;
	qse_mchar_t* modpath;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = sysname;
	#else
	modpath = qse_wcstombsdup (sysname, QSE_NULL, httpd->mmgr);
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return QSE_NULL;
	}
	#endif

	h = lt_dlopenext (modpath);
	if (!h) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));

	#if defined(QSE_CHAR_IS_MCHAR)
	/* do nothing */
	#else
	QSE_MMGR_FREE (httpd->mmgr, modpath);
	#endif

	return h;

#elif defined(_WIN32)

	HMODULE h;

	h = LoadLibrary (sysname);
	if (!h) qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#elif defined(__OS2__)

	HMODULE h;
	qse_mchar_t* modpath;
	char errbuf[CCHMAXPATH];
	APIRET rc;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = sysname;
	#else
	modpath = qse_wcstombsdup (sysname, QSE_NULL, httpd->mmgr);
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return QSE_NULL;
	}
	#endif

	/* DosLoadModule() seems to have severe limitation on 
	 * the file name it can load (max-8-letters.xxx) */
	rc = DosLoadModule (errbuf, QSE_COUNTOF(errbuf) - 1, modpath, &h);
	if (rc != NO_ERROR) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		h = QSE_NULL;
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* do nothing */
	#else
	QSE_MMGR_FREE (httpd->mmgr, modpath);
	#endif

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#elif defined(__DOS__)

	/* the DOS code here is not generic enough. it's for a specific
	 * dos-extender only. the best is to enable QSE_ENABLE_STATIC_MODULE
	 * when building for DOS. */
	void* h;
	qse_mchar_t* modpath;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = sysname;
	#else
	modpath = qse_wcstombsdup (sysname, QSE_NULL, httpd->mmgr);
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return QSE_NULL;
	}
	#endif

	h = LoadModule (modpath);
	if (!h) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));

	#if defined(QSE_CHAR_IS_MCHAR)
	/* do nothing */
	#else
	QSE_MMGR_FREE (httpd->mmgr, modpath);
	#endif

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#else
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return QSE_NULL;
#endif
}

static void mod_close (qse_httpd_t* httpd, void* handle)
{
#if defined(QSE_ENABLE_STATIC_MODULE)
	/* this won't be called at all when modules are linked into
	 * the main library. */
#elif defined(USE_LTDL)
	lt_dlclose (handle);
#elif defined(_WIN32)
	FreeLibrary ((HMODULE)handle);
#elif defined(__OS2__)
	DosFreeModule ((HMODULE)handle);
#elif defined(__DOS__)
	FreeModule (handle);
#else
	/* nothing to do */
#endif
}

static void* mod_symbol (qse_httpd_t* httpd, void* handle, const qse_char_t* name)
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


#if defined(QSE_ENABLE_STATIC_MODULE)
	/* this won't be called at all when modules are linked into
	 * the main library. */
	s = QSE_NULL;

#elif defined(USE_LTDL)
	s = lt_dlsym (handle, mname);
	if (s == QSE_NULL) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
#elif defined(_WIN32)
	s = GetProcAddress ((HMODULE)handle, mname);
	if (s == QSE_NULL) qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
#elif defined(__OS2__)
	{
		APIRET rc;
		rc = DosQueryProcAddr ((HMODULE)handle, 0, mname, (PFN*)&s);
		if (rc != NO_ERROR) 
		{
			qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
			s = QSE_NULL;
		}
	}
#elif defined(__DOS__)
	s = GetProcAddress (handle, mname);
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

