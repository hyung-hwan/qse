/*
 * $Id: Awk.h,v 1.5 2007-01-03 09:51:52 bacon Exp $
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
	ase_awk_t* handle;
	int        option;
	int        errnum;
	ase_size_t errlin;
	ase_char_t errmsg[256];

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
	STDMETHOD(get_ShiftOperators)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ShiftOperators)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_VariableShading)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_VariableShading)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_UniqueFunction)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UniqueFunction)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ExplicitVariable)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ExplicitVariable)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ImplicitVariable)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ImplicitVariable)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ErrorMessage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ErrorLine)(/*[out, retval]*/ int *pVal);
	STDMETHOD(get_ErrorCode)(/*[out, retval]*/ int *pVal);
	STDMETHOD(get_Option)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_Option)(/*[in]*/ int newVal);
	HRESULT __stdcall Parse (int* ret);
	HRESULT __stdcall Run (int* ret);
};

#endif
