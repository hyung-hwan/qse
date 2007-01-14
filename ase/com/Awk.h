/*
 * $Id: Awk.h,v 1.11 2007-01-14 15:06:58 bacon Exp $
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
#include <ase/awk/val.h>

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

	struct
	{
		struct
		{
			int parse;
			int run;
		} block;
		struct
		{
			int parse;
			int run;
		} expr;
		struct
		{
			int build;
			int match;
		} rex;
	} max_depth;

	IBuffer* read_src_buf;
	IBuffer* write_src_buf;
	ase_size_t read_src_pos;
	ase_size_t read_src_len;

	IBuffer* write_extio_buf;

	BSTR entry_point;
	BOOL debug;
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
	STDMETHOD(get_Debug)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Debug)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_EntryPoint)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EntryPoint)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_MaxDepthForRexMatch)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForRexMatch)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForRexBuild)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForRexBuild)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForExprRun)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForExprRun)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForExprParse)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForExprParse)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForBlockRun)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForBlockRun)(/*[in]*/ int newVal);
	STDMETHOD(get_MaxDepthForBlockParse)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_MaxDepthForBlockParse)(/*[in]*/ int newVal);
	STDMETHOD(get_UseCrlf)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UseCrlf)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Nextofile)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Nextofile)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_StripSpaces)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_StripSpaces)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_StringIndexOne)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_StringIndexOne)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SupportBlockless)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_SupportBlockless)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SupportExtio)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_SupportExtio)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ConcatString)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ConcatString)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_IdivOperator)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_IdivOperator)(/*[in]*/ BOOL newVal);
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
	STDMETHOD(AddBuiltinFunction)(BSTR name);
	HRESULT __stdcall Parse (int* ret);
	HRESULT __stdcall Run (int* ret);
};

#endif
