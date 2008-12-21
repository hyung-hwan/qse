/*
 * $Id: AwkExtio.cpp 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#include "stdafx.h"
#include "asecom.h"
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

STDMETHODIMP CAwkExtio::get_Type(AwkExtioType *pVal)
{
	*pVal = type;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Mode(AwkExtioMode *pVal)
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
