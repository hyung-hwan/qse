/*
 * $Id: AwkExtio.h,v 1.3 2007/04/30 06:04:43 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_COM_AWKEXTIO_H_
#define _ASE_COM_AWKEXTIO_H_

#include "resource.h" 
#include <ase/awk/awk.h>

/////////////////////////////////////////////////////////////////////////////
// CAwkExtio
class ATL_NO_VTABLE CAwkExtio : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAwkExtio, &CLSID_AwkExtio>,
	public IDispatchImpl<IAwkExtio, &IID_IAwkExtio, &LIBID_ASECOM>
{
public:
	BSTR name;
	AwkExtioType type;
	AwkExtioMode mode;
	VARIANT handle;

	IBuffer* read_buf;
	ase_size_t read_buf_pos;
	ase_size_t read_buf_len;

	BOOL PutName (const TCHAR* val);

	CAwkExtio ();
	~CAwkExtio ();

DECLARE_REGISTRY_RESOURCEID(IDR_AWKEXTIO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAwkExtio)
	COM_INTERFACE_ENTRY(IAwkExtio)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IAwkExtio
public:
	STDMETHOD(get_Handle)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Handle)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Mode)(/*[out, retval]*/ AwkExtioMode *pVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ AwkExtioType *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__AWKEXTIO_H_
