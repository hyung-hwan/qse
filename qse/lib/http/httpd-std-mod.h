#if !defined(QSE_HTTPD_DEFAULT_MODPREFIX)
#	if defined(_WIN32)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "qsehttpd-"
#	elif defined(__OS2__)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "htd-"
#	elif defined(__DOS__)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "htd-"
#	else
#		define QSE_HTTPD_DEFAULT_MODPREFIX "libqsehttpd-"
#	endif
#endif

#if !defined(QSE_HTTPD_DEFAULT_MODPOSTFIX)
#	define QSE_HTTPD_DEFAULT_MODPOSTFIX ""
#endif


static int mod_open (qse_httpd_t* httpd, qse_httpd_mod_t* mod)
{
#if defined(USE_LTDL)
	void* h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = mod->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, httpd->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, httpd->mmgr);
	#endif
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM, QSE_NULL);
		return -1;
	}

	h = lt_dlopenext (modpath);

	QSE_MMGR_FREE (httpd->mmgr, modpath);

	if (h == QSE_NULL) return -1;
	mod->handle = h;
	return 0;


#elif defined(_WIN32)

	HMODULE h;
	qse_char_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	modpath = qse_stradup (tmp, QSE_NULL, httpd->mmgr);
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM, QSE_NULL);
		return -1;
	}

	h = LoadLibrary (modpath);

	QSE_MMGR_FREE (httpd->mmgr, modpath);
	
	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));

	if (h == QSE_NULL) return -1;
	mod->handle = h;
	return 0;

#elif defined(__OS2__)

	HMODULE h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;
	char errbuf[CCHMAXPATH];
	APIRET rc;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, httpd->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, httpd->mmgr);
	#endif
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM, QSE_NULL);
		return -1;
	}

	/* DosLoadModule() seems to have severe limitation on 
	 * the file name it can load (max-8-letters.xxx) */
	rc = DosLoadModule (errbuf, QSE_COUNTOF(errbuf) - 1, modpath, &h);
	if (rc != NO_ERROR) h = QSE_NULL;

	QSE_MMGR_FREE (httpd->mmgr, modpath);

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));

	if (h == QSE_NULL) return -1;
	mod->handle = h;
	return 0;


#elif defined(__DOS__)

	/* the DOS code here is not generic enough. it's for a specific
	 * dos-extender only. the best is to enable QSE_ENABLE_STATIC_MODULE
	 * when building for DOS. */
	void* h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, httpd->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, httpd->mmgr);
	#endif
	if (!modpath)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM, QSE_NULL);
		return 01;
	}

	h = LoadModule (modpath);

	QSE_MMGR_FREE (httpd->mmgr, modpath);
	
	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));

	if (h == QSE_NULL) return -1;
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
#elif defined(_WIN32)
	s = GetProcAddress ((HMODULE)mod->handle, mname);
#elif defined(__OS2__)
	if (DosQueryProcAddr ((HMODULE)mod->handle, 0, mname, (PFN*)&s) != NO_ERROR) s = QSE_NULL;
#elif defined(__DOS__)
	s = GetProcAddress (mod->handle, mname);
#else
	s = QSE_NULL;
#endif

#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
#else
	QSE_MMGR_FREE (httpd->mmgr, mname);
#endif

	return s;
}

