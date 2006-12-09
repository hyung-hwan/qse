/*
 * $Id: Buffer.h,v 1.1 2006-12-09 11:50:08 bacon Exp $
 */

#ifndef _ASE_COM_BUFFER_H_
#define _ASE_COM_BUFFER_H_

#include "resource.h"
#include "ase.h"

class ATL_NO_VTABLE CBuffer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBuffer, &CLSID_Buffer>,
	public IDispatchImpl<IBuffer, &IID_IBuffer, &LIBID_ASELib>
{
private:
	CComBSTR str;

public:
	CBuffer ();
	~CBuffer ();
	
DECLARE_REGISTRY_RESOURCEID(IDR_AWKBUFFER)

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
