/*
 * $Id: Awk.cpp,v 1.3 2006-12-10 05:59:52 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "Awk.h"
#include "AwkExtio.h"
#include "Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wctype.h>
#include <stdio.h>

STDMETHODIMP CAwk::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IAwk,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (/*Inline*/IsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CAwk::CAwk (): handle(NULL), 
	read_source_buf(NULL), write_source_buf(NULL),
	write_extio_buf(NULL)
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwk::CAwk %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif
}

CAwk::~CAwk ()
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwk::~CAwk %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif

	if (write_extio_buf != NULL)
	{
		write_extio_buf->Release ();
	}

	if (write_source_buf != NULL)
	{
		write_source_buf->Release ();
	}

	if (read_source_buf != NULL)
	{
		read_source_buf->Release ();
	}

	if (handle != NULL) 
	{
		ase_awk_close (handle);
		handle = NULL;
	}
}

static void* __awk_malloc (ase_size_t n, void* custom_data)
{
	return malloc (n);
}

static void* __awk_realloc (void* ptr, ase_size_t n, void* custom_data)
{
	return realloc (ptr, n);
}

static void __awk_free (void* ptr, void* custom_data)
{
	free (ptr);
}

static ase_real_t __awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int __awk_sprintf (
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, len, fmt, ap);
	if (n < 0 || (ase_size_t)n >= len)
	{
		if (len > 0) buf[len-1] = ASE_T('\0');
		n = -1;
	}
#elif defined(__MSDOS__)
	/* TODO: check buffer overflow */
	n = vsprintf (buf, fmt, ap);
#else
	n = xp_vsprintf (buf, len, fmt, ap);
#endif
	va_end (ap);
	return n;
}

static void __awk_aprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	#if defined(_MSC_VER) && (_MSC_VER<1400)
	MessageBox (NULL, buf, 
		ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	#else
	MessageBox (NULL, buf, 
		ASE_T("\uB2DD\uAE30\uB9AC \uC870\uB610"), MB_OK|MB_ICONERROR);
	#endif
#elif defined(__MSDOS__)
	vprintf (fmt, ap);
#else
	xp_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void __awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

#if defined(_WIN32)
	_vftprintf (stderr, fmt, ap);
#elif defined(__MSDOS__)
	vfprintf (stderr, fmt, ap);
#else
	xp_vfprintf (stderr, fmt, ap);
#endif

	va_end (ap);
}

static ase_ssize_t __read_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	CAwk* awk = (CAwk*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return (ase_ssize_t)awk->Fire_OpenSource (0);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return (ase_ssize_t)awk->Fire_CloseSource (0);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		if (awk->read_source_buf == NULL)
		{
			HRESULT hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->read_source_buf);
			if (FAILED(hr))
			{	
MessageBox (NULL, _T("COCREATEINSTANCE FAILED"), _T("FUCK"), MB_OK);
				return -1;
			}

			awk->read_source_pos = 0;
			awk->read_source_len = 0;
		}

		CBuffer* tmp = (CBuffer*)awk->read_source_buf;

		if (awk->read_source_pos >= awk->read_source_len)
		{
			INT n = awk->Fire_ReadSource (awk->read_source_buf);
			if (n <= 0) return (ase_ssize_t)n;
			//if (n > (INT)tmp->str.Length()) return -1;
			if (n > (INT)SysStringLen(tmp->str)) return -1;

			awk->read_source_pos = 0;
			awk->read_source_len = n;
		}

		BSTR str = tmp->str;
		if (str == NULL) 
		{
			/* it can't be emptry here ... */
			return -1;
		}

		ASE_AWK_ASSERT (awk->handle, 
			awk->read_source_pos < awk->read_source_len);


		INT left = awk->read_source_len - awk->read_source_pos;
		if (left > (ase_ssize_t)count)
		{
			memcpy (data,
				((TCHAR*)str)+awk->read_source_pos,
				count * ASE_SIZEOF(ase_char_t));
			awk->read_source_pos += count;
			return count;
		}
		else
		{
			memcpy (data, 
				((TCHAR*)str)+awk->read_source_pos,
				left * ASE_SIZEOF(ase_char_t));
			awk->read_source_pos = 0;
			awk->read_source_len = 0;
			return (ase_ssize_t)left;
		}
	}

	return -1;
}

static ase_ssize_t __write_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t count)
{
	CAwk* awk = (CAwk*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		return (ase_ssize_t)awk->Fire_OpenSource (1);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return (ase_ssize_t)awk->Fire_CloseSource (1);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		HRESULT hr;

		if (awk->write_source_buf == NULL)
		{
			hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->write_source_buf);
			if (FAILED(hr))
			{	
MessageBox (NULL, _T("COCREATEINSTANCE FAILED"), _T("FUCK"), MB_OK);
				return -1;
			}
		}

		BSTR bstr = SysAllocStringLen (data, count);
		if (bstr == NULL) return -1; /* TODO: bettter error handling */

		hr = awk->write_source_buf->put_Value (bstr);
SysFreeString (bstr);
		if (FAILED(hr))
		{
//SysFreeString (bstr);
			return -1;
		}

		INT n = awk->Fire_WriteSource (awk->write_source_buf);
		if (n > (INT)count) return -1; 
		return (ase_ssize_t)n;
	}

	return -1;
}

HRESULT CAwk::Parse (int* ret)
{
 	if (handle == NULL)
	{
		ase_awk_syscas_t syscas;

		memset (&syscas, 0, sizeof(syscas));
		syscas.malloc = __awk_malloc;
		syscas.realloc = __awk_realloc;
		syscas.free = __awk_free;

		syscas.is_upper  = iswupper;
		syscas.is_lower  = iswlower;
		syscas.is_alpha  = iswalpha;
		syscas.is_digit  = iswdigit;
		syscas.is_xdigit = iswxdigit;
		syscas.is_alnum  = iswalnum;
		syscas.is_space  = iswspace;
		syscas.is_print  = iswprint;
		syscas.is_graph  = iswgraph;
		syscas.is_cntrl  = iswcntrl;
		syscas.is_punct  = iswpunct;
		syscas.to_upper  = towupper;
		syscas.to_lower  = towlower;

		syscas.memcpy = memcpy;
		syscas.memset = memset;
		syscas.pow = __awk_pow;
		syscas.sprintf = __awk_sprintf;
		syscas.aprintf = __awk_aprintf;
		syscas.dprintf = __awk_dprintf;
		syscas.abort = abort;

		handle = ase_awk_open (&syscas);
		if (handle == NULL)
		{
			*ret = -1;
			return S_OK;
		}

		int opt = /*ASE_AWK_IMPLICIT | 
		      ASE_AWK_EXPLICIT | 
			  ASE_AWK_UNIQUEAFN | 
		      ASE_AWK_HASHSIGN | 
		      ASE_AWK_IDIV |
		      ASE_AWK_SHADING | 
		      ASE_AWK_SHIFT | */
		      ASE_AWK_EXTIO /*| 
		      ASE_AWK_BLOCKLESS | 
		      ASE_AWK_STRINDEXONE | 
		      ASE_AWK_STRIPSPACES | 
		      ASE_AWK_NEXTOFILE*/;

		ase_awk_setopt (handle, opt);
	}

	ase_awk_srcios_t srcios;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = this;
	
	if (ase_awk_parse (handle, &srcios) == -1)
	{
		*ret = -1;
		return S_OK;
	}

	*ret = 0;
	return S_OK;
}

static ase_ssize_t __process_extio (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	CAwk* awk = (CAwk*)epa->custom_data;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		IAwkExtio* extio;
		CAwkExtio* extio2;

		HRESULT hr = CoCreateInstance (
			CLSID_AwkExtio, NULL, CLSCTX_ALL, 
			IID_IAwkExtio, (void**)&extio);
		if (FAILED(hr)) return -1; /* TODO: better error handling.. */

		BSTR bstr = SysAllocString (epa->name);
		if (bstr == NULL) 
		{
			extio->Release ();
			return -1; /* TODO: better error handling */
		}

		extio2 = (CAwkExtio*)extio;
		extio2->name.Empty ();
		extio2->name.Attach (bstr);
		extio2->type = epa->type & 0xFF;
		extio2->mode = epa->mode;
		VariantClear (&extio2->handle);

		INT n = awk->Fire_OpenExtio (extio);
		if (n >= 0)
		{
			extio->AddRef ();
			epa->handle = extio;
		}

		extio->Release ();
		return n;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);

		INT n = awk->Fire_CloseExtio (extio);
		if (n >= 0)
		{
			extio->Release();
			epa->handle = NULL;
		}
		return n;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		/*
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);

		if (awk->write_extio_buf == NULL)
		{
			HRESULT hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->write_extio_buf);
			if (FAILED(hr)) return -1;
		}

		INT n = awk->Fire_ReadExtio (extio, awk->write_extio_buf);
		if (n > (INT)size) return -1; 
		return (ase_ssize_t)n;
		*/
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		HRESULT hr;
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);

		if (awk->write_extio_buf == NULL)
		{
			hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->write_extio_buf);
			if (FAILED(hr)) return -1;
		}

		BSTR bstr = SysAllocStringLen (data, size);
		if (bstr == NULL) return -1; /* TODO: error code */

		hr = awk->write_extio_buf->put_Value (bstr);
SysFreeString (bstr);
		if (FAILED(hr))
		{
//			SysFreeString (bstr);
			return -1;
		}

		INT n = awk->Fire_WriteExtio (extio, awk->write_extio_buf);
		if (n > (INT)size) return -1; 
		return (ase_ssize_t)n;
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		return 1;
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
	}

	return -1;
}

HRESULT CAwk::Run (int* ret)
{
	if (handle == NULL)
	{
		/* TODO: better error handling... */
		/* call parse first... */
		*ret = -1;
		return S_OK;
	}

	ase_awk_runios_t runios;
	runios.pipe = NULL;
	runios.coproc = NULL;
	runios.file = NULL;
	runios.console = __process_extio;
	runios.custom_data = this;

	if (ase_awk_run (handle, NULL, &runios, NULL, NULL, this) == -1)
	{
		int err = ase_awk_geterrnum (handle);
MessageBox (NULL, ase_awk_geterrstr(err), ase_awk_geterrstr(err), MB_OK);
		*ret = -1;
		return S_OK;
	}

	*ret = 0;
	return S_OK;
}
