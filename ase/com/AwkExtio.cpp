/*
 * $Id: AwkExtio.cpp,v 1.1 2006-12-09 11:50:08 bacon Exp $
 */

#include "stdafx.h"
#include "ase.h"
#include "AwkExtio.h"

/////////////////////////////////////////////////////////////////////////////
// CAwkExtio

CAwkExtio::CAwkExtio ()
{
}

CAwkExtio::~CAwkExtio ()
{
}

STDMETHODIMP CAwkExtio::get_Name(BSTR *pVal)
{

	*pVal = name;
	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Name(BSTR newVal)
{
	name = newVal;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Type(int *pVal)
{
	*pVal = type;
	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Type(int newVal)
{
	type = newVal;
	return S_OK;
}

STDMETHODIMP CAwkExtio::get_Mode(int *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAwkExtio::put_Mode(int newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}
