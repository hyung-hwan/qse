/*
 * $Id: awk_cp.h,v 1.9 2007-02-03 10:52:12 bacon Exp $
 *
 * {License}
 */

#ifndef _AWK_CP_H_
#define _AWK_CP_H_

template <class T>
class CProxyIAwkEvents: 
	public IConnectionPointImpl<T, &DIID_IAwkEvents, CComDynamicUnkArray>
{
public:
	INT Fire_OpenSource(INT mode)
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

	INT Fire_CloseSource(INT mode)
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

	INT Fire_ReadSource (IBuffer* buf)
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

	INT Fire_WriteSource (IBuffer* buf)
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

	INT Fire_OpenExtio (IAwkExtio* extio)
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

			args[0] = (IUnknown*)extio;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x5, IID_NULL, LOCALE_USER_DEFAULT,
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

	INT Fire_CloseExtio (IAwkExtio* extio)
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

			args[0] = (IUnknown*)extio;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x6, IID_NULL, LOCALE_USER_DEFAULT, 
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

	INT Fire_ReadExtio (IAwkExtio* extio, IBuffer* buf)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[2], ret;
	
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
			VariantClear (&args[1]);

			args[1] = (IUnknown*)extio;
			args[0] = (IUnknown*)buf;

			DISPPARAMS disp = { args, NULL, 2, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x7, IID_NULL, LOCALE_USER_DEFAULT, 
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

		return -1;
	}

	INT Fire_WriteExtio (IAwkExtio* extio, IBuffer* buf)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant args[2], ret;
	
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
			VariantClear (&args[1]);

			args[1] = (IUnknown*)extio;
			args[0] = (IUnknown*)buf;

			DISPPARAMS disp = { args, NULL, 2, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x8, IID_NULL, LOCALE_USER_DEFAULT, 
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

		return -1;
	}

	
	INT Fire_FlushExtio (IAwkExtio* extio)
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

			args[0] = (IUnknown*)extio;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0x9, IID_NULL, LOCALE_USER_DEFAULT, 
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

	INT Fire_NextExtio (IAwkExtio* extio)
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

			args[0] = (IUnknown*)extio;

			DISPPARAMS disp = { args, NULL, 1, 0 };
			HRESULT hr = pDispatch->Invoke (
				0xA, IID_NULL, LOCALE_USER_DEFAULT, 
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

	int Fire_HandleBuiltinFunction (
		ase_awk_run_t* run, BSTR name, SAFEARRAY* argarray, ase_awk_val_t** retv)
	{
		T* pT = static_cast<T*>(this);
		int i, nconns = m_vec.GetSize();
		CComVariant ret;
		VARIANT args[2];
		ase_awk_val_t* v;

		VariantInit (&args[0]);
		VariantInit (&args[1]);

		args[1].vt = VT_BSTR;
		args[1].bstrVal = name;

		args[0].vt = VT_ARRAY | VT_VARIANT;
		args[0].parray = argarray;
		
		for (i = 0; i < nconns; i++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(i);
			pT->Unlock();

			IDispatch* pDispatch = 
				reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch == NULL) continue;

			VariantClear (&ret);

			DISPPARAMS disp = { args, NULL, 2, 0 };
			HRESULT hr = pDispatch->Invoke (
				0xB, IID_NULL, LOCALE_USER_DEFAULT, 
				DISPATCH_METHOD, &disp, &ret, NULL, NULL);
			if (FAILED(hr)) continue;

			if (ret.vt == VT_EMPTY)
			{
				/* probably, the handler has not been implemeted*/
				continue;
			}

			VariantClear (&args[1]);
			VariantClear (&args[0]);

			if (ret.vt == VT_I1)
				v = ase_awk_makeintval (run, ret.cVal);
			else if (ret.vt == VT_I2)
				v = ase_awk_makeintval (run, ret.iVal);
			else if (ret.vt == VT_I4)
				v = ase_awk_makeintval (run, ret.lVal);
			else if (ret.vt == VT_I8)
				v = ase_awk_makeintval (run, ret.llVal);
			else if (ret.vt == VT_UI1)
				v = ase_awk_makeintval (run, ret.bVal);
			else if (ret.vt == VT_UI2)
				v = ase_awk_makeintval (run, ret.uiVal);
			else if (ret.vt == VT_UI4)
				v = ase_awk_makeintval (run, ret.ulVal);
			else if (ret.vt == VT_UI8)
				v = ase_awk_makeintval (run, ret.ullVal);
			else if (ret.vt == VT_INT)
				v = ase_awk_makeintval (run, ret.intVal);
			else if (ret.vt == VT_UINT)
				v = ase_awk_makeintval (run, ret.uintVal);
			else if (ret.vt == VT_BOOL)
				v = ase_awk_makeintval (run, ((ret.boolVal == 0)? 0: 1));
			else if (ret.vt == VT_R4)
				v = ase_awk_makerealval (run, ret.fltVal);
			else if (ret.vt == VT_R8)
				v = ase_awk_makerealval (run, ret.dblVal);
			else if (ret.vt == VT_BSTR)
				v = ase_awk_makestrval (run, ret.bstrVal, SysStringLen(ret.bstrVal));
			else if (ret.vt == VT_NULL)

				v = ase_awk_val_nil;
			else return 3; /* wrong return value */
			
			if (v == ASE_NULL) return 1; /* out of memory */

			*retv = v;
			return 0; /* success */
		}

		VariantClear (&args[1]);
		VariantClear (&args[0]);
		return 2; /* no proper handler */
	}


};

#endif
