/*
 * $Id: Buffer.cpp,v 1.5 2007-01-10 14:30:44 bacon Exp $
 */

#include "stdafx.h"
#include "Buffer.h"

CBuffer::CBuffer ()
{
	str = NULL;
}

CBuffer::~CBuffer ()
{
	if (str != NULL) SysFreeString (str);
}

STDMETHODIMP CBuffer::get_Value (BSTR *pVal)
{
	if (str == NULL) *pVal = NULL;
	else
	{
		BSTR tmp = SysAllocStringLen(str, SysStringLen(str));
		if (tmp == NULL) return E_OUTOFMEMORY;
		*pVal = tmp;
	}

	return S_OK;
}

STDMETHODIMP CBuffer::put_Value (BSTR newVal)
{
	if (str != NULL) SysFreeString (str);
	if (newVal == NULL) str = newVal;
	else 
	{
		str = SysAllocStringLen (newVal, SysStringLen(newVal));
		if (str == NULL) return E_OUTOFMEMORY;
	}

	return S_OK;
}

BOOL CBuffer::PutValue (const TCHAR* val, SIZE_T len)
{
	if (str != NULL) SysFreeString (str);
	str = SysAllocStringLen (val, len);
	return (str == NULL)? FALSE: TRUE;
}
