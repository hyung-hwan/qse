/*
 * $Id: AwkExtio.cpp,v 1.8 2007-01-10 14:30:44 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "AwkExtio.h"

#include <stdio.h>
/////////////////////////////////////////////////////////////////////////////
// CAwkExtio

CAwkExtio::CAwkExtio (): name (NULL)
{
	VariantInit (&handle);
}

CAwkExtio::~CAwkExtio ()
{
	if (name != NULL) SysFreeString (name);
	VariantClear (&handle);
}

STDMETHODIMP CAwkExtio::get_Name (BSTR *pVal)
{
	if (name == NULL) *pVal = name;
	else
	{
		BSTR tmp = SysAllocStringLen (name, SysStringLen(name));
		if (tmp == NULL) return E_OUTOFMEMORY;
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
	VariantClear (pVal);
	VariantCopy (pVal, &handle);
	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Handle (VARIANT newVal)
{
	VariantClear (&handle);
	VariantCopy (&handle, &newVal);
	return S_OK;
}
