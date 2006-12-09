/*
 * $Id: Buffer.cpp,v 1.1 2006-12-09 11:50:08 bacon Exp $
 */

#include "stdafx.h"
#include "Buffer.h"

CBuffer::CBuffer ()
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CBuffer::~CBuffer %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif
}

CBuffer::~CBuffer ()
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CBuffer::~CBuffer %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif
}

STDMETHODIMP CBuffer::get_Value(BSTR *pVal)
{
	*pVal = str;
	return S_OK;
}

STDMETHODIMP CBuffer::put_Value(BSTR newVal)
{
	str = newVal;
	return S_OK;
}
