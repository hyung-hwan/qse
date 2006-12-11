/*
 * $Id: AwkExtio.cpp,v 1.4 2006-12-11 14:58:25 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "AwkExtio.h"

#include <stdio.h>
/////////////////////////////////////////////////////////////////////////////
// CAwkExtio

CAwkExtio::CAwkExtio (): name (NULL)
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwkExtio::CAwkExtio %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif
}

CAwkExtio::~CAwkExtio ()
{
#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwkExtio::~CAwkExtio %p"), this);
	MessageBox (NULL, x, x, MB_OK);
#endif
	if (name != NULL) SysFreeString (name);
}

STDMETHODIMP CAwkExtio::get_Name (BSTR *pVal)
{
	if (name == NULL) *pVal = name;
	else
	{
		BSTR tmp = SysAllocStringLen (name, SysStringLen(name));
		if (tmp = NULL) return E_OUTOFMEMORY;
		*pVal = tmp;
	}

	return S_OK;
}

BOOL CAwkExtio::PutName (const TCHAR* val)
{
	if (name != NULL) SysFreeString (name);
	name = SysAllocString (val);
	return (name == NULL)? FALSE: TRUE;
}

STDMETHODIMP CAwkExtio::get_Type(int *pVal)
{
	*pVal = type;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Mode(int *pVal)
{
	*pVal = mode;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Handle (VARIANT *pVal)
{
	*pVal = handle;
	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Handle (VARIANT newVal)
{
	handle.Copy (&newVal);
	return S_OK;
}
