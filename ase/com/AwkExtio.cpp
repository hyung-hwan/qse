/*
 * $Id: AwkExtio.cpp,v 1.2 2006-12-09 17:36:27 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "AwkExtio.h"

#include <stdio.h>
/////////////////////////////////////////////////////////////////////////////
// CAwkExtio

CAwkExtio::CAwkExtio ()
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
}

STDMETHODIMP CAwkExtio::get_Name(BSTR *pVal)
{

	*pVal = name;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Type(int *pVal)
{
	*pVal = type;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Mode(int *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

