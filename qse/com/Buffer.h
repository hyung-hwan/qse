/*
 * $Id: Buffer.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_COM_BUFFER_H_
#define _ASE_COM_BUFFER_H_

#include "resource.h"
#include "asecom.h"

class ATL_NO_VTABLE CBuffer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBuffer, &CLSID_Buffer>,
	public IDispatchImpl<IBuffer, &IID_IBuffer, &LIBID_ASECOM>
{
public:
	BSTR str;
	BOOL PutValue (const TCHAR* val, SIZE_T len);

public:
	CBuffer ();
	~CBuffer ();
	
DECLARE_REGISTRY_RESOURCEID(IDR_BUFFER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBuffer)
	COM_INTERFACE_ENTRY(IBuffer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
	STDMETHOD(get_Value)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Value)(/*[in]*/ BSTR newVal);
};

#endif
