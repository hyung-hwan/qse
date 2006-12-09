/*
 * $Id: awk_cp.h,v 1.2 2006-12-09 12:09:42 bacon Exp $
 */

#ifndef _AWK_CP_H_
#define _AWK_CP_H_

/*#import "C:\projects\ase\debug\win32\vs60\aseawk.dll" raw_interfaces_only, raw_native_types, no_namespace, named_guids*/

template <class T>
class CProxyIAwkEvents : public IConnectionPointImpl<T, &DIID_IAwkEvents, CComDynamicUnkArray>
{
public:
	LONG Fire_OpenSource(LONG mode)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[1], ret;
		
		for (i = 0; i < nconns; i++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(i);
			pT->Unlock();

			IDispatch* pDispatch = 
				reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch == NULL) continue;

			VariantClear (&ret);
			VariantClear (&args[0]);

			args[0] = mode;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x1, IID_NULL, LOCALE_USER_DEFAULT,
				DISPATCH_METHOD, &disp, &ret, NULL, NULL);
			if (FAILED(hr)) continue;

			if (ret.vt == VT_EMPTY)
			{
				/* probably, the handler has not been implemeted*/
				continue;
			}

			hr = ret.ChangeType (VT_I4);
			if (FAILED(hr))
			{
				/* TODO: set the error code properly... */
				/* invalid value returned... */
				return -1;
			}

			return ret.lVal;
		}

		return -1;
	}

	LONG Fire_CloseSource(LONG mode)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[1], ret;
		
		for (i = 0; i < nconns; i++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(i);
			pT->Unlock();

			IDispatch* pDispatch = 
				reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch == NULL) continue;


			VariantClear (&ret);
			VariantClear (&args[0]);

			args[0] = mode;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x2, IID_NULL, LOCALE_USER_DEFAULT, 
				DISPATCH_METHOD, &disp, &ret, NULL, NULL);
			if (FAILED(hr)) continue;

			if (ret.vt == VT_EMPTY)
			{
				/* probably, the handler has not been implemeted*/
				continue;
			}

			hr = ret.ChangeType (VT_I4);
			if (FAILED(hr))
			{
				/* TODO: set the error code properly... */
				/* invalid value returned... */
				return -1;
			}

			return ret.lVal;
		}

		return -1;
	}

	LONG Fire_ReadSource (IBuffer* buf)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[1], ret;
	
		for (i = 0; i < nconns; i++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(i);
			pT->Unlock();

			IDispatch* pDispatch = 
				reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch == NULL) continue;

			VariantClear (&ret);
			VariantClear (&args[0]);

			args[0] = (IUnknown*)buf;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x3, IID_NULL, LOCALE_USER_DEFAULT, 
				DISPATCH_METHOD, &disp, &ret, NULL, NULL);
			if (FAILED(hr))
			{
				continue;
			}

			if (ret.vt == VT_EMPTY)
			{
				/* probably, the handler has not been implemeted*/
				continue;
			}

			hr = ret.ChangeType (VT_I4);
			if (FAILED(hr))
			{
				/* TODO: set the error code properly... */
				/* invalid value returned... */
				return -1;
			}

			return ret.lVal;
		}

		/* no event handler attached for the source code read. */
		/* TODO: set error code ... */
		return -1;
	}

	LONG Fire_WriteSource (IBuffer* buf)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[1], ret;
	
		for (i = 0; i < nconns; i++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(i);
			pT->Unlock();

			IDispatch* pDispatch = 
				reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch == NULL) continue;

			VariantClear (&ret);
			VariantClear (&args[0]);

			args[0] = (IUnknown*)buf;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x4, IID_NULL, LOCALE_USER_DEFAULT, 
				DISPATCH_METHOD, &disp, &ret, NULL, NULL);
			if (FAILED(hr))
			{
				continue;
			}

			if (ret.vt == VT_EMPTY)
			{
				/* probably, the handler has not been implemeted*/
				continue;
			}

			hr = ret.ChangeType (VT_I4);
			if (FAILED(hr))
			{
				/* TODO: set the error code properly... */
				/* invalid value returned... */
				return -1;
			}

			return ret.lVal;
		}

		/* no event handler attached for the source code write.
		 * make the operation succeed by returning the reqested 
		 * data length. */ 
		CComBSTR bstr;
		buf->get_Value (&bstr);
		return bstr.Length();
	}
};

#endif
