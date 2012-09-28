
static int qse_ripuri (qse_httpd_t* httpd, const qse_char_t* uri, int flags)
{
	const qse_char_t* ptr, * colon, * at;
	qse_size_t len;
	qse_uri_t xuri;

	QSE_MEMSET (xuri, 0, QSE_SIZEOF(xuri));

	/* scheme */
	xuri.scheme.ptr = ptr;
	while (*uri != QSE_T(':')) 
	{
		if (*uri == QSE_T('\0')) return -1;
		uri++;
	}
	xuri.scheme.len = uri - xuri.scheme.ptr;

	uri++; /* skip : */ 
	if (*uri != QSE_T('/')) return -1;
	uri++; /* skip / */
	if (*uri != QSE_T('/')) return -1;
	uri++; /* skip / */

	/* username, password, host, port */
	for (colon = QSE_NULL, at = QSE_NULL, ptr = uri; ; uri++)
	{
		if (at == QSE_NULL)
		{
			if (colon == QSE_NULL && *uri == QSE_T(':')) colon = uri;
			else if (*uri == QSE_T('@')) 
			{
				if (colon)
				{
					xuri.user.ptr = ptr;
					xuri.user.len = colon - ptr;
					xuri.pass.ptr = colon + 1;
					xuri.pass.len = uri - colon - 1;

					colon = QSE_NULL;
				}
				else
				{
					xuri.user.ptr = ptr;
					xuri.user.len = uri - ptr;
				}
			}
			else if (*uri == QSE_T('/') || *uri == QSE_T('\0')) 
			{
				xuri.host = xuri.user;
				xuir.port = xuri.pass;

				xuri.user.ptr = QSE_NULL;
				xuri.user.len = 0;
				xuri.pass.ptr = QSE_NULL;
				xuri.pass.len = 0;
				break;
			}
		}
		else
		{
			if (colon == QSE_NULL && *uri == QSE_T(':')) colon = uri;
			else if (*uri == QSE_T('/') || *uri == QSE_T('\0')) 
			{
				if (colon)
				{
					xuri.host.ptr = ptr;
					xuri.host.len = colon - ptr;
					xuri.port.ptr = colon + 1;
					xuri.port.len = uri - colon - 1;
				}
				else
				{
					xuri.host.ptr = ptr;
					xuri.host.len = uri - ptr;
				}
			}
		}
	}

	if (uri == QSE_T('/'))
	{
		xuri.path.ptr = uri;
		while (*uri != QSE_T('\0') && *uri != QSE_T('?')) uri++;
		xuri.path.len = uri - xuri.path.ptr;

		if (uri == QSE('#')) 
		{
			xuri.query.ptr = ++uri;
			while (*uri != QSE_T('\0') && *uri != QSE_T('#')) uri++;
			xuri.query.len = uri - xuri.query.ptr;

			if (uri == QSE_T('#'))
			{
				xuri.query.ptr = ++uri;
				while (*uri != QSE_T('\0')) uri++;
				xuri.fragment.len = uri - xuri.fragment.ptr;
			}
		}
	}

	*xuri = scheme;
	return 0;
}
