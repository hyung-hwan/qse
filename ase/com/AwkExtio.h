/*
 * $Id: AwkExtio.h,v 1.6 2006-12-11 08:44:52 bacon Exp $
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
	public IDispatchImpl<IAwkExtio, &IID_IAwkExtio, &LIBID_ASELib>
{
public:
	BSTR name;
	int type;
	int mode;
	CComVariant handle;

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
	STDMETHOD(get_Mode)(/*[out, retval]*/ int *pVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ int *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__AWKEXTIO_H_
