/*
 * $Id: AwkExtio.cpp,v 1.6 2006-12-14 07:55:52 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "AwkExtio.h"

#include <stdio.h>
/////////////////////////////////////////////////////////////////////////////
// CAwkExtio

CAwkExtio::CAwkExtio (): name (NULL)/*, handle (NULL)*/
{
//#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwkExtio::CAwkExtio %p"), this);
	MessageBox (NULL, x, x, MB_OK);
//#endif
}

CAwkExtio::~CAwkExtio ()
{
//#ifdef _DEBUG
	TCHAR x[128];
	_sntprintf (x, 128, _T("CAwkExtio::~CAwkExtio %p"), this);
	MessageBox (NULL, x, x, MB_OK);
//#endif
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
	VariantCopy (pVal, &handle);
	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Handle (VARIANT newVal)
{
	handle.Copy (&newVal);
	return S_OK;
}
