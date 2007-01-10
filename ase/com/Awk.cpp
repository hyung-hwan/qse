/*
 * $Id: Awk.cpp,v 1.15 2007-01-10 14:30:43 bacon Exp $
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

CAwk::CAwk (): 
	handle (NULL), 
	read_src_buf (NULL), 
	write_src_buf (NULL),
	write_extio_buf (NULL),
	entry_point (NULL)
{
	/* TODO: what is the best default option? */
	option = 
		ASE_AWK_IMPLICIT | 
		ASE_AWK_EXPLICIT | 
		ASE_AWK_UNIQUEFN | 
		ASE_AWK_IDIV |
		ASE_AWK_SHADING | 
		ASE_AWK_SHIFT | 
		ASE_AWK_EXTIO | 
		ASE_AWK_BLOCKLESS | 
		ASE_AWK_STRIDXONE | 
		ASE_AWK_STRIPSPACES | 
		ASE_AWK_NEXTOFILE |
		ASE_AWK_CRLF;
	memset (&max_depth, 0, sizeof(max_depth));

	errnum    = 0;
	errlin    = 0;
	errmsg[0] = ASE_T('\0');
}

CAwk::~CAwk ()
{
	if (entry_point != NULL) SysFreeString (entry_point);

	if (write_extio_buf != NULL) write_extio_buf->Release ();
	if (write_src_buf != NULL) write_src_buf->Release ();
	if (read_src_buf != NULL) read_src_buf->Release ();

	if (handle != NULL) 
	{
		ase_awk_close (handle);
		handle = NULL;
	}
}

static void* awk_malloc (ase_size_t n, void* custom_data)
{
	return malloc (n);
}

static void* awk_realloc (void* ptr, ase_size_t n, void* custom_data)
{
	return realloc (ptr, n);
}

static void awk_free (void* ptr, void* custom_data)
{
	free (ptr);
}

static ase_real_t awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static void awk_abort (void* custom_data)
{
	abort ();
}

static int awk_sprintf (
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

static void awk_aprintf (const ase_char_t* fmt, ...)
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

static void awk_dprintf (const ase_char_t* fmt, ...)
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
		if (awk->read_src_buf == NULL)
		{
			HRESULT hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->read_src_buf);
			if (FAILED(hr)) return -1;

			awk->read_src_pos = 0;
			awk->read_src_len = 0;
		}

		CBuffer* tmp = (CBuffer*)awk->read_src_buf;

		if (awk->read_src_pos >= awk->read_src_len)
		{
			INT n = awk->Fire_ReadSource (awk->read_src_buf);
			if (n <= 0) return (ase_ssize_t)n;

			if (SysStringLen(tmp->str) < (UINT)n) return -1;
			awk->read_src_pos = 0;
			awk->read_src_len = n;
		}

		ASE_AWK_ASSERT (awk->handle, 
			awk->read_src_pos < awk->read_src_len);

		BSTR str = tmp->str;
		INT left = awk->read_src_len - awk->read_src_pos;
		if (left > (ase_ssize_t)count)
		{
			memcpy (data,
				((TCHAR*)str)+awk->read_src_pos,
				count * ASE_SIZEOF(ase_char_t));
			awk->read_src_pos += count;
			return count;
		}
		else
		{
			memcpy (data, 
				((TCHAR*)str)+awk->read_src_pos,
				left * ASE_SIZEOF(ase_char_t));
			awk->read_src_pos = 0;
			awk->read_src_len = 0;
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

		if (awk->write_src_buf == NULL)
		{
			hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->write_src_buf);
			if (FAILED(hr)) return -1;
		}

		CBuffer* tmp = (CBuffer*)awk->write_src_buf;
		if (tmp->PutValue (data, count) == FALSE) return -1; 

		INT n = awk->Fire_WriteSource (awk->write_src_buf);
		if (n > (INT)count) return -1; 
		return (ase_ssize_t)n;
	}

	return -1;
}

HRESULT CAwk::Parse (int* ret)
{
 	if (handle == NULL)
	{
		ase_awk_sysfns_t sysfns;

		memset (&sysfns, 0, sizeof(sysfns));
		sysfns.malloc = awk_malloc;
		sysfns.realloc = awk_realloc;
		sysfns.free = awk_free;

		sysfns.is_upper  = iswupper;
		sysfns.is_lower  = iswlower;
		sysfns.is_alpha  = iswalpha;
		sysfns.is_digit  = iswdigit;
		sysfns.is_xdigit = iswxdigit;
		sysfns.is_alnum  = iswalnum;
		sysfns.is_space  = iswspace;
		sysfns.is_print  = iswprint;
		sysfns.is_graph  = iswgraph;
		sysfns.is_cntrl  = iswcntrl;
		sysfns.is_punct  = iswpunct;
		sysfns.to_upper  = towupper;
		sysfns.to_lower  = towlower;

		sysfns.memcpy = memcpy;
		sysfns.memset = memset;
		sysfns.pow = awk_pow;
		sysfns.sprintf = awk_sprintf;
		sysfns.aprintf = awk_aprintf;
		sysfns.dprintf = awk_dprintf;
		sysfns.abort = awk_abort;

		handle = ase_awk_open (&sysfns, &errnum);
		if (handle == NULL)
		{
			errlin = 0;
			ase_awk_strxcpy (
				errmsg, ASE_COUNTOF(errmsg), 
				ase_awk_geterrstr(errnum));

			*ret = -1;
			return S_OK;
		}

		ase_awk_setoption (handle, option);

		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_BLOCK_PARSE, max_depth.block.parse);
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_BLOCK_RUN, max_depth.block.run);
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_EXPR_PARSE, max_depth.expr.parse);
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_EXPR_RUN, max_depth.expr.run);
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_REX_BUILD, max_depth.rex.build);
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_REX_MATCH, max_depth.rex.match);
	}

	ase_awk_srcios_t srcios;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = this;
	
	if (ase_awk_parse (handle, &srcios) == -1)
	{
		const ase_char_t* msg;

		ase_awk_geterror (handle, &errnum, &errlin, &msg);
		ase_awk_strxcpy (errmsg, ASE_COUNTOF(errmsg), msg);

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
		IBuffer* read_buf;

		HRESULT hr = CoCreateInstance (
			CLSID_AwkExtio, NULL, CLSCTX_ALL, 
			IID_IAwkExtio, (void**)&extio);
		if (FAILED(hr)) return -1; 

		hr = CoCreateInstance (
			CLSID_Buffer, NULL, CLSCTX_ALL,
			IID_IBuffer, (void**)&read_buf);
		if (FAILED(hr)) 
		{
			extio->Release ();
			return -1;
		}

		extio2 = (CAwkExtio*)extio;
		if (extio2->PutName (epa->name) == FALSE)
		{
			read_buf->Release ();
			extio->Release ();
			return -1; 
		}
		extio2->type = epa->type & 0xFF;
		extio2->mode = epa->mode;

		read_buf->AddRef ();
		extio2->read_buf = read_buf;
		extio2->read_buf_pos = 0;
		extio2->read_buf_len = 0;

		INT n = awk->Fire_OpenExtio (extio);
		if (n >= 0)
		{
			extio->AddRef ();
			epa->handle = extio;
		}

		read_buf->Release ();
		extio->Release ();
		return n;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		IAwkExtio* extio;
		CAwkExtio* extio2;
	       
		extio = (IAwkExtio*)epa->handle;
		extio2 = (CAwkExtio*)extio;

		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);

		INT n = awk->Fire_CloseExtio (extio);
		if (n >= 0)
		{
			extio2->read_buf->Release();
			extio->Release();
			epa->handle = NULL;
		}

		return n;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		IAwkExtio* extio;
		CAwkExtio* extio2;

		extio = (IAwkExtio*)epa->handle;
		extio2 = (CAwkExtio*)extio;

		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);

		CBuffer* tmp = (CBuffer*)extio2->read_buf;
		if (extio2->read_buf_pos >= extio2->read_buf_len)
		{
			INT n = awk->Fire_ReadExtio (extio, extio2->read_buf);
			if (n <= 0) return (ase_ssize_t)n;

			if (SysStringLen(tmp->str) < (UINT)n) return -1;
			extio2->read_buf_pos = 0;
			extio2->read_buf_len = n;
		}

		ASE_AWK_ASSERT (awk->handle, 
			extio2->read_buf_pos < extio2->read_buf_len);

		BSTR str = tmp->str;
		INT left = extio2->read_buf_len - extio2->read_buf_pos;
		if (left > (ase_ssize_t)size)
		{
			memcpy (data,
				((TCHAR*)str)+extio2->read_buf_pos,
				size * ASE_SIZEOF(ase_char_t));
			extio2->read_buf_pos += size;
			return size;
		}
		else
		{
			memcpy (data, 
				((TCHAR*)str)+extio2->read_buf_pos,
				left * ASE_SIZEOF(ase_char_t));
			extio2->read_buf_pos = 0;
			extio2->read_buf_len = 0;
			return (ase_ssize_t)left;
		}
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

		CBuffer* tmp = (CBuffer*)awk->write_extio_buf;
		if (tmp->PutValue (data, size) == FALSE) return -1;

		INT n = awk->Fire_WriteExtio (extio, awk->write_extio_buf);
		if (n > (INT)size) return -1; 
		return (ase_ssize_t)n;
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);
		return awk->Fire_FlushExtio (extio);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_AWK_ASSERT (ase_awk_getrunawk(epa->run), extio != NULL);
		return awk->Fire_NextExtio (extio);
	}

	return -1;
}

HRESULT CAwk::Run (int* ret)
{
	const ase_char_t* entry = NULL;

	if (handle == NULL)
	{
		errnum = ASE_AWK_EINTERN;
		errlin = 0;
		_tcscpy (errmsg, _T("parse not called yet"));

		*ret = -1;
		return S_OK;
	}

	ase_awk_runios_t runios;
	runios.pipe = NULL;
	runios.coproc = NULL;
	runios.file = __process_extio;
	runios.console = __process_extio;
	runios.custom_data = this;

	if (entry_point != NULL && 
	    SysStringLen(entry_point) > 0) entry = entry_point;

	if (ase_awk_run (handle, entry, &runios, NULL, NULL, this) == -1)
	{
		const ase_char_t* msg;

		ase_awk_geterror (handle, &errnum, &errlin, &msg);
		ase_awk_strxcpy (errmsg, ASE_COUNTOF(errmsg), msg);

		*ret = -1;
		return S_OK;
	}

	*ret = 0;
	return S_OK;
}

STDMETHODIMP CAwk::get_ErrorCode(int *pVal)
{
	*pVal = errnum;
	return S_OK;
}

STDMETHODIMP CAwk::get_ErrorLine(int *pVal)
{
	*pVal = errlin;
	return S_OK;
}

STDMETHODIMP CAwk::get_ErrorMessage(BSTR *pVal)
{
	BSTR tmp = SysAllocStringLen (errmsg, _tcslen(errmsg));
	if (tmp == NULL) return E_OUTOFMEMORY;
	*pVal = tmp;
	return S_OK;
}

STDMETHODIMP CAwk::get_ImplicitVariable(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_IMPLICIT) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_ImplicitVariable(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_IMPLICIT;
	else option = option & ~ASE_AWK_IMPLICIT;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_ExplicitVariable(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_EXPLICIT) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_ExplicitVariable(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_EXPLICIT;
	else option = option & ~ASE_AWK_EXPLICIT;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_UniqueFunction(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_UNIQUEFN) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_UniqueFunction(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_UNIQUEFN;
	else option = option & ~ASE_AWK_UNIQUEFN;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_VariableShading(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_SHADING) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_VariableShading(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_SHADING;
	else option = option & ~ASE_AWK_SHADING;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_ShiftOperators(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_SHIFT) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_ShiftOperators(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_SHIFT;
	else option = option & ~ASE_AWK_SHIFT;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_IdivOperator(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_IDIV) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_IdivOperator(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_IDIV;
	else option = option & ~ASE_AWK_IDIV;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_ConcatString(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_STRCONCAT) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_ConcatString(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_STRCONCAT;
	else option = option & ~ASE_AWK_STRCONCAT;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_SupportExtio(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_EXTIO) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_SupportExtio(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_EXTIO;
	else option = option & ~ASE_AWK_EXTIO;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_SupportBlockless(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_BLOCKLESS) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_SupportBlockless(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_BLOCKLESS;
	else option = option & ~ASE_AWK_BLOCKLESS;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_StringIndexOne(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_STRIDXONE) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_StringIndexOne(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_STRIDXONE;
	else option = option & ~ASE_AWK_STRIDXONE;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_StripSpaces(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_STRIPSPACES) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_StripSpaces(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_STRIPSPACES;
	else option = option & ~ASE_AWK_STRIPSPACES;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_Nextofile(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_NEXTOFILE) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_Nextofile(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_NEXTOFILE;
	else option = option & ~ASE_AWK_NEXTOFILE;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_UseCrlf(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_CRLF) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_UseCrlf(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_CRLF;
	else option = option & ~ASE_AWK_CRLF;
	if (handle != NULL) ase_awk_setoption (handle, option);
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForBlockParse(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.block.parse = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_BLOCK_PARSE);
	}
	*pVal = max_depth.block.parse;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForBlockParse(int newVal)
{
	max_depth.block.parse = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_BLOCK_PARSE, max_depth.block.parse);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForBlockRun(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.block.run = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_BLOCK_RUN);
	}
	*pVal = max_depth.block.run;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForBlockRun(int newVal)
{
	max_depth.block.run = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_BLOCK_RUN, max_depth.block.run);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForExprParse(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.expr.parse = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_EXPR_PARSE);
	}
	*pVal = max_depth.expr.parse;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForExprParse(int newVal)
{
	max_depth.expr.parse = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_EXPR_PARSE, max_depth.expr.parse);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForExprRun(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.expr.run = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_EXPR_RUN);
	}
	*pVal = max_depth.expr.run;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForExprRun(int newVal)
{
	max_depth.expr.run = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_EXPR_RUN, max_depth.expr.run);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForRexBuild(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.rex.build = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_REX_BUILD);
	}
	*pVal = max_depth.rex.build;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForRexBuild(int newVal)
{
	max_depth.rex.build = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_REX_BUILD, max_depth.rex.build);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_MaxDepthForRexMatch(int *pVal)
{
	if (handle != NULL) 
	{
		max_depth.rex.match = 
			ase_awk_getmaxdepth (handle, ASE_AWK_DEPTH_REX_MATCH);
	}
	*pVal = max_depth.rex.match;
	return S_OK;
}

STDMETHODIMP CAwk::put_MaxDepthForRexMatch(int newVal)
{
	max_depth.rex.match = newVal;
	if (handle != NULL) 
	{
		ase_awk_setmaxdepth (handle, 
			ASE_AWK_DEPTH_REX_MATCH, max_depth.rex.match);
	}
	return S_OK;
}

STDMETHODIMP CAwk::get_EntryPoint(BSTR *pVal)
{
	if (entry_point == NULL) *pVal = NULL;
	else
	{
		BSTR tmp = SysAllocStringLen (
			entry_point, SysStringLen(entry_point));
		if (tmp == NULL) return E_OUTOFMEMORY;
		*pVal = tmp;
	}

	return S_OK;
}

STDMETHODIMP CAwk::put_EntryPoint(BSTR newVal)
{
	if (entry_point != NULL) SysFreeString (entry_point);
	if (newVal == NULL) entry_point = newVal;
	else 
	{
		entry_point = SysAllocStringLen (newVal, SysStringLen(newVal));
		if (entry_point == NULL) return E_OUTOFMEMORY;
	}

	return S_OK;
}
