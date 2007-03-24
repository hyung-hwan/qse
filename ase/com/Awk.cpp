/*
 * $Id: Awk.cpp,v 1.30 2007-03-24 05:18:31 bacon Exp $
 *
 * {License}
 */

#include "stdafx.h"
#include "asecom.h"
#include "Awk.h"
#include "AwkExtio.h"
#include "Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wctype.h>
#include <stdio.h>

#include <ase/cmn/str.h>
#include <ase/utl/stdio.h>
#include <ase/utl/ctype.h>

#define DBGOUT(x) do { if (debug) OutputDebugString (x); } while(0)
#define DBGOUT2(awk,x) do { if (awk->debug) OutputDebugString (x); } while(0)

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
	bfn_list (NULL),
	entry_point (NULL),
	debug (FALSE),
	use_longlong (FALSE)
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
		ASE_AWK_STRBASEONE | 
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
	while (bfn_list != NULL)
	{
		bfn_t* next = bfn_list->next;
		free (bfn_list->name.ptr);
		free (bfn_list);
		bfn_list = next;
	}

	if (entry_point != NULL) SysFreeString (entry_point);

	if (write_extio_buf != NULL) write_extio_buf->Release ();
	if (write_src_buf != NULL) write_src_buf->Release ();
	if (read_src_buf != NULL) read_src_buf->Release ();

	if (handle != NULL) 
	{
		ase_awk_close (handle);
		handle = NULL;
		DBGOUT (_T("closed awk"));
	}
}

#ifndef NDEBUG
void ase_assert_abort (void)
{
	abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	int n;
	ase_char_t buf[1024];

	va_start (ap, fmt);
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	MessageBox (NULL, buf, ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	va_end (ap);
}
#endif

static void* custom_awk_malloc (void* custom, ase_size_t n)
{
	return malloc (n);
}

static void* custom_awk_realloc (void* custom, void* ptr, ase_size_t n)
{
	return realloc (ptr, n);
}

static void custom_awk_free (void* custom, void* ptr)
{
	free (ptr);
}

ase_bool_t custom_awk_isupper (void* custom, ase_cint_t c)  
{ 
	return ase_isupper (c); 
}

ase_bool_t custom_awk_islower (void* custom, ase_cint_t c)  
{ 
	return ase_islower (c); 
}

ase_bool_t custom_awk_isalpha (void* custom, ase_cint_t c)  
{ 
	return ase_isalpha (c); 
}

ase_bool_t custom_awk_isdigit (void* custom, ase_cint_t c)  
{ 
	return ase_isdigit (c); 
}

ase_bool_t custom_awk_isxdigit (void* custom, ase_cint_t c) 
{ 
	return ase_isxdigit (c); 
}

ase_bool_t custom_awk_isalnum (void* custom, ase_cint_t c)
{ 
	return ase_isalnum (c); 
}

ase_bool_t custom_awk_isspace (void* custom, ase_cint_t c)
{ 
	return ase_isspace (c); 
}

ase_bool_t custom_awk_isprint (void* custom, ase_cint_t c)
{ 
	return ase_isprint (c); 
}

ase_bool_t custom_awk_isgraph (void* custom, ase_cint_t c)
{
	return ase_isgraph (c); 
}

ase_bool_t custom_awk_iscntrl (void* custom, ase_cint_t c)
{
	return ase_iscntrl (c);
}

ase_bool_t custom_awk_ispunct (void* custom, ase_cint_t c)
{
	return ase_ispunct (c);
}

ase_cint_t custom_awk_toupper (void* custom, ase_cint_t c)
{
	return ase_toupper (c);
}

ase_cint_t custom_awk_tolower (void* custom, ase_cint_t c)
{
	return ase_tolower (c);
}

static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}


static int custom_awk_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static void custom_awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	_vftprintf (stderr, fmt, ap);
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
			if (FAILED(hr)) 
			{
				DBGOUT2 (awk, _T("cannot create source input buffer"));
				return -1;
			}

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

		ASE_ASSERT (awk->read_src_pos < awk->read_src_len);

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
			if (FAILED(hr)) 
			{
				DBGOUT2 (awk, _T("cannot create source output buffer"));
				return -1;
			}
		}

		CBuffer* tmp = (CBuffer*)awk->write_src_buf;
		if (tmp->PutValue (data, count) == FALSE) 
		{
			DBGOUT2 (awk, _T("cannot set source output buffer"));
			return -1; 
		}

		INT n = awk->Fire_WriteSource (awk->write_src_buf);
		if (n > (INT)count) return -1; 
		return (ase_ssize_t)n;
	}

	return -1;
}

static int __handle_bfn (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	CAwk* awk = (CAwk*)ase_awk_getruncustomdata (run);
	long nargs = (long)ase_awk_getnargs (run);
	ase_awk_val_t* v;

	SAFEARRAY* aa;
	SAFEARRAYBOUND bound[1];

	bound[0].lLbound = 0;
	bound[0].cElements = nargs;

	aa = SafeArrayCreate (VT_VARIANT, ASE_COUNTOF(bound), bound);
	if (aa == NULL)
	{
		ase_char_t buf[128];
		_sntprintf (buf, ASE_COUNTOF(buf), 
			_T("out of memory in creating argument array for '%.*s'"),
			fnl, fnm);
		//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
		return -1;
	}

	for (long i = 0; i < nargs; i++)
	{
		VARIANT arg;

		VariantInit (&arg);

		v = ase_awk_getarg (run, i);

		if (v->type == ASE_AWK_VAL_INT)
		{
			if (awk->use_longlong)
			{
				arg.vt = VT_I8;
				arg.llVal = ((ase_awk_val_int_t*)v)->val;
			}
			else
			{
				arg.vt = VT_I4;
				arg.lVal = (LONG)((ase_awk_val_int_t*)v)->val;
			}
		}
		else if (v->type == ASE_AWK_VAL_REAL)
		{
			arg.vt = VT_R8;
			arg.dblVal = ((ase_awk_val_real_t*)v)->val;
		}
		else if (v->type == ASE_AWK_VAL_STR)
		{
			BSTR tmp = SysAllocStringLen (
				((ase_awk_val_str_t*)v)->buf,
				((ase_awk_val_str_t*)v)->len);

			if (arg.bstrVal == NULL)
			{
				VariantClear (&arg);
				SafeArrayDestroy (aa);

				ase_char_t buf[128];
				_sntprintf (buf, ASE_COUNTOF(buf), 
					_T("out of memory in handling '%.*s'"),
					fnl, fnm);
				//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
				return -1;
			}

			arg.vt = VT_BSTR;
			arg.bstrVal = tmp;
		}
		else if (v->type == ASE_AWK_VAL_NIL)
		{
			arg.vt = VT_NULL;
		}

		HRESULT hr = SafeArrayPutElement (aa, &i, &arg);
		if (hr != S_OK)
		{
			VariantClear (&arg);
			SafeArrayDestroy (aa);			

			ase_char_t buf[128];
			_sntprintf (buf, ASE_COUNTOF(buf), 
				_T("out of memory in handling '%.*s'"),
				fnl, fnm);
			//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
			return -1;
		}

		VariantClear (&arg);
	}

	BSTR name = SysAllocStringLen (fnm, fnl);
	if (name == NULL) 
	{
		SafeArrayDestroy (aa);

		ase_char_t buf[128];
		_sntprintf (buf, ASE_COUNTOF(buf), 
			_T("out of memory in handling '%.*s'"),
			fnl, fnm);
		//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
					
		return -1;
	}

	ase_awk_val_t* ret;
	int n = awk->Fire_HandleBuiltinFunction (run, name, aa, &ret);
	if (n == 1)
	{
		ase_char_t buf[128];
		_sntprintf (buf, ASE_COUNTOF(buf), 
			_T("out of memory in handling '%.*s'"),
			fnl, fnm);
		//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
		return -1;
	}
	else if (n == 2)
	{
		ase_char_t buf[128];
		_sntprintf (buf, ASE_COUNTOF(buf), 
			_T("no handler for '%.*s'"),
			fnl, fnm);
		//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
		return -1;
	}
	else if (n == 3)
	{
		ase_char_t buf[128];
		_sntprintf (buf, ASE_COUNTOF(buf), 
			_T("return value not supported for '%.*s'"),
			fnl, fnm);
		//TODO: ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, buf);
		return -1;
	}

	/* name and aa are destroyed in HandleBuiltinFunction */
	//SafeArrayDestroy (aa);
	//SysFreeString (name);

	ase_awk_setretval (run, ret);
	return 0;
}

HRESULT CAwk::Parse (int* ret)
{
 	if (handle == NULL)
	{
		ase_awk_prmfns_t prmfns;

		memset (&prmfns, 0, sizeof(prmfns));

		prmfns.mmgr.malloc = custom_awk_malloc;
		prmfns.mmgr.realloc = custom_awk_realloc;
		prmfns.mmgr.free = custom_awk_free;

		prmfns.ccls.is_upper  = custom_awk_isupper;
		prmfns.ccls.is_lower  = custom_awk_islower;
		prmfns.ccls.is_alpha  = custom_awk_isalpha;
		prmfns.ccls.is_digit  = custom_awk_isdigit;
		prmfns.ccls.is_xdigit = custom_awk_isxdigit;
		prmfns.ccls.is_alnum  = custom_awk_isalnum;
		prmfns.ccls.is_space  = custom_awk_isspace;
		prmfns.ccls.is_print  = custom_awk_isprint;
		prmfns.ccls.is_graph  = custom_awk_isgraph;
		prmfns.ccls.is_cntrl  = custom_awk_iscntrl;
		prmfns.ccls.is_punct  = custom_awk_ispunct;
		prmfns.ccls.to_upper  = custom_awk_toupper;
		prmfns.ccls.to_lower  = custom_awk_tolower;

		prmfns.misc.pow     = custom_awk_pow;
		prmfns.misc.sprintf = custom_awk_sprintf;
		prmfns.misc.dprintf = custom_awk_dprintf;

		handle = ase_awk_open (&prmfns, NULL);
		if (handle == NULL)
		{
			errlin = 0;
			ase_strxcpy (
				errmsg, ASE_COUNTOF(errmsg), 
				ase_awk_geterrstr(NULL, ASE_AWK_ENOMEM));

			*ret = -1;

			DBGOUT (_T("cannot open awk"));
			return S_OK;
		}
		else DBGOUT (_T("opened awk successfully"));

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

	ase_awk_clrbfn (handle);

	for (bfn_t* bfn = bfn_list; bfn != NULL; bfn = bfn->next)
	{
		if (ase_awk_addbfn (
			handle, bfn->name.ptr, bfn->name.len, 0, 
			bfn->min_args, bfn->max_args, NULL, 
			__handle_bfn) == NULL)
		{
			const ase_char_t* msg;

			DBGOUT (_T("cannot add the builtin function"));

			ase_awk_geterror (handle, &errnum, &errlin, &msg);
			ase_strxcpy (errmsg, ASE_COUNTOF(errmsg), msg);

			*ret = -1;
			return S_OK;
		}
	}

	ase_awk_srcios_t srcios;

	srcios.in = __read_source;
	srcios.out = __write_source;
	srcios.custom_data = this;
	
	if (ase_awk_parse (handle, &srcios) == -1)
	{
		const ase_char_t* msg;

		ase_awk_geterror (handle, &errnum, &errlin, &msg);
		ase_strxcpy (errmsg, ASE_COUNTOF(errmsg), msg);

		DBGOUT (_T("cannot parse the source code"));

		*ret = -1;
		return S_OK;
	}
	else DBGOUT (_T("parsed the source code successfully"));

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
		if (FAILED(hr)) 
		{
			DBGOUT2 (awk, _T("cannot create extio"));
			return -1; 
		}

		hr = CoCreateInstance (
			CLSID_Buffer, NULL, CLSCTX_ALL,
			IID_IBuffer, (void**)&read_buf);
		if (FAILED(hr)) 
		{
			DBGOUT2 (awk, _T("cannot create extio input buffer"));
			extio->Release ();
			return -1;
		}

		extio2 = (CAwkExtio*)extio;
		if (extio2->PutName (epa->name) == FALSE)
		{
			read_buf->Release ();
			extio->Release ();
			DBGOUT2 (awk, _T("cannot set the name of the extio input buffer"));
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

		ASE_ASSERT (extio != NULL);

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

		ASE_ASSERT (extio != NULL);

		CBuffer* tmp = (CBuffer*)extio2->read_buf;
		if (extio2->read_buf_pos >= extio2->read_buf_len)
		{
			INT n = awk->Fire_ReadExtio (extio, extio2->read_buf);
			if (n <= 0) return (ase_ssize_t)n;

			if (SysStringLen(tmp->str) < (UINT)n) return -1;
			extio2->read_buf_pos = 0;
			extio2->read_buf_len = n;
		}

		ASE_ASSERT (extio2->read_buf_pos < extio2->read_buf_len);

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
		ASE_ASSERT (extio != NULL);

		if (awk->write_extio_buf == NULL)
		{
			hr = CoCreateInstance (
				CLSID_Buffer, NULL, CLSCTX_ALL, 
				IID_IBuffer, (void**)&awk->write_extio_buf);
			if (FAILED(hr)) 
			{
				DBGOUT2 (awk, _T("cannot create extio output buffer"));
				return -1;
			}
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
		ASE_ASSERT (extio != NULL);
		return awk->Fire_FlushExtio (extio);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		IAwkExtio* extio = (IAwkExtio*)epa->handle;
		ASE_ASSERT (extio != NULL);
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
		ase_strxcpy (errmsg, ASE_COUNTOF(errmsg), msg);

		DBGOUT (_T("cannot run the program"));
		*ret = -1;
		return S_OK;
	}
	else DBGOUT (_T("run the program successfully"));

	*ret = 0;
	return S_OK;
}

STDMETHODIMP CAwk::AddBuiltinFunction (
	BSTR name, int min_args, int max_args, int* ret)
{
	bfn_t* bfn;
	size_t name_len = SysStringLen(name);

	for (bfn = bfn_list; bfn != NULL; bfn = bfn->next)
	{
		if (ase_strxncmp (
			bfn->name.ptr, bfn->name.len,
			name, name_len) == 0)
		{
			errnum = ASE_AWK_EEXIST;
			errlin = 0;
			_sntprintf (
				errmsg, ASE_COUNTOF(errmsg), 
				_T("'%.*s' added already"), 
				bfn->name.len, bfn->name.ptr);

			*ret = -1;
			return S_OK;
		}
	}

	bfn = (bfn_t*)malloc (sizeof(bfn_t));
	if (bfn == NULL) 
	{
		errnum = ASE_AWK_ENOMEM;
		errlin = 0;
		ase_strxcpy (
			errmsg, ASE_COUNTOF(errmsg), 
			ase_awk_geterrstr(NULL, errnum));

		*ret = -1;
		return S_OK;
	}

	bfn->name.len = name_len;
	bfn->name.ptr = (TCHAR*)malloc (bfn->name.len * sizeof(TCHAR));
	if (bfn->name.ptr == NULL) 
	{
		free (bfn);

		errnum = ASE_AWK_ENOMEM;
		errlin = 0;
		ase_strxcpy (
			errmsg, ASE_COUNTOF(errmsg), 
			ase_awk_geterrstr(NULL, errnum));

		*ret = -1;
		return S_OK;
	}
	memcpy (bfn->name.ptr, name, sizeof(TCHAR) * bfn->name.len);

	bfn->min_args = min_args;
	bfn->max_args = max_args;
	bfn->next = bfn_list;
	bfn_list = bfn;

	*ret = 0;
	return S_OK;
}

STDMETHODIMP CAwk::DeleteBuiltinFunction (BSTR name, int* ret)
{
	size_t name_len = SysStringLen(name);
	bfn_t* bfn, * next, * prev = NULL;

	for (bfn = bfn_list; bfn != NULL; bfn = next)
	{
		next = bfn->next;

		if (ase_strxncmp (
			bfn->name.ptr, bfn->name.len,
			name, name_len) == 0)
		{
			free (bfn->name.ptr);
			free (bfn);

			if (prev == NULL) bfn_list = next;
			else prev->next = next;

			*ret = 0;
			return S_OK;
		}

		prev = bfn;
	}

	errnum = ASE_AWK_ENOENT;
	errlin = 0;
	ase_strxcpy (
		errmsg, ASE_COUNTOF(errmsg), 
		ase_awk_geterrstr(NULL, errnum));

	*ret = -1;
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

STDMETHODIMP CAwk::get_StringBaseOne(BOOL *pVal)
{
	if (handle != NULL) option = ase_awk_getoption (handle);
	*pVal = (option & ASE_AWK_STRBASEONE) == 1;
	return S_OK;
}

STDMETHODIMP CAwk::put_StringBaseOne(BOOL newVal)
{
	if (newVal) option = option | ASE_AWK_STRBASEONE;
	else option = option & ~ASE_AWK_STRBASEONE;
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

STDMETHODIMP CAwk::get_Debug(BOOL *pVal)
{
	*pVal = debug;
	return S_OK;
}

STDMETHODIMP CAwk::put_Debug(BOOL newVal)
{
	debug = newVal;
	return S_OK;
}

STDMETHODIMP CAwk::get_UseLongLong(BOOL *pVal)
{
	*pVal = use_longlong;
	return S_OK;
}

STDMETHODIMP CAwk::put_UseLongLong(BOOL newVal)
{
	use_longlong = newVal;
	return S_OK;
}
