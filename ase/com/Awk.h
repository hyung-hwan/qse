/*
 * $Id: Awk.h,v 1.4 2006-12-11 14:58:25 bacon Exp $
 */

#ifndef _ASE_COM_AWK_H_
#define _ASE_COM_AWK_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h" 
#include "ase.h"
#include "awk_cp.h"
#include <ase/awk/awk.h>

/////////////////////////////////////////////////////////////////////////////
// CAwk

class CAwk : 
	public IDispatchImpl<IAwk, &IID_IAwk, &LIBID_ASELib>, 
	public ISupportErrorInfo,
	/*public CComObjectRoot,*/
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAwk,&CLSID_Awk>,
	public IConnectionPointContainerImpl<CAwk>,
	public IProvideClassInfo2Impl<&CLSID_Awk, &DIID_IAwkEvents, &LIBID_ASELib>,
	public CProxyIAwkEvents< CAwk >

{
public:
	int option;
	ase_awk_t* handle;

	IBuffer* read_src_buf;
	IBuffer* write_src_buf;
	ase_size_t read_src_pos;
	ase_size_t read_src_len;

	IBuffer* write_extio_buf;
public:
	CAwk();
	~CAwk ();
	
BEGIN_COM_MAP(CAwk)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IAwk)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CAwk)
CONNECTION_POINT_ENTRY(DIID_IAwkEvents)
END_CONNECTION_POINT_MAP()

//DECLARE_NOT_AGGREGATABLE(CAwk) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_AWK)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IAwk
public:
	STDMETHOD(get_Option)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_Option)(/*[in]*/ int newVal);
	HRESULT __stdcall Parse (int* ret);
	HRESULT __stdcall Run (int* ret);
};

#endif
