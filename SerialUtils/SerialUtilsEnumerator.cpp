#include "stdafx.h"
#include "SerialUtilsEnumerator.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <atlbase.h>
#include <Wbemidl.h>
#include <comutil.h>
#include <winioctl.h>
#include <setupapi.h>
#include <tuple>

std::vector<std::tuple<unsigned int, std::wstring>> SerialUtilsEnumerator::ports_array;

BOOL SerialUtilsEnumerator::UpdatePortListWMI()
{
	//Initialize COM (Required by CEnumerateSerial::UsingWMI)
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to initialize COM, Error:%x\n"), hr);
		return FALSE;
	}

	//Initialize COM security (Required by CEnumerateSerial::UsingWMI)
	hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to initialize COM security, Error:%08X\n"), hr);
		CoUninitialize();
		return FALSE;
	}

	// Start of scope with smart pointers
	// The braces are intended to be so that memory leaks won't occur
	{
		//Set our output parameters to sane defaults
		ports_array.clear();
		//names_array.clear();

		//Create the WBEM locator
		ATL::CComPtr<IWbemLocator> pLocator;
		hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&pLocator));
		if (FAILED(hr))
		{
			return FALSE;
		}

		CComPtr<IWbemServices> pServices;
		hr = pLocator->ConnectServer(_bstr_t("\\\\.\\root\\cimv2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pServices);
		if (FAILED(hr))
		{
			return FALSE;
		}
		//Execute the query
		CComPtr<IEnumWbemClassObject> pClassObject;
		hr = pServices->CreateInstanceEnum(_bstr_t("Win32_SerialPort"), WBEM_FLAG_RETURN_WBEM_COMPLETE, nullptr, &pClassObject);
		if (FAILED(hr))
		{
			return FALSE;
		}

		//Now enumerate all the ports
		hr = WBEM_S_NO_ERROR;

		//The final Next will return WBEM_S_FALSE
		while (hr == WBEM_S_NO_ERROR)
		{
			ULONG uReturned = 0;
			CComPtr<IWbemClassObject> apObj[10];
			hr = pClassObject->Next(WBEM_INFINITE, 10, reinterpret_cast<IWbemClassObject**>(apObj), &uReturned);
			if (SUCCEEDED(hr))
			{
				for (ULONG n = 0; n < uReturned; n++)
				{
					ATL::CComVariant varProperty1;
					HRESULT hrGet = apObj[n]->Get(L"DeviceID", 0, &varProperty1, nullptr, nullptr);
					if (SUCCEEDED(hrGet) && (varProperty1.vt == VT_BSTR) && (wcslen(varProperty1.bstrVal) > 3))
					{
						//If it looks like "COMX" then add it to the array which will be returned
						if ((_wcsnicmp(varProperty1.bstrVal, L"COM", 3) == 0) && IsNumeric(&(varProperty1.bstrVal[3]), TRUE))
						{
							std::tuple<unsigned int, std::wstring> current;
							//Work out the port number
							int nPort = _wtoi(&(varProperty1.bstrVal[3]));
							//ports_array.push_back(nPort);

							//Also get the friendly name of the port
							ATL::CComVariant varProperty2;
							if (SUCCEEDED(apObj[n]->Get(L"Name", 0, &varProperty2, nullptr, nullptr)) && (varProperty2.vt == VT_BSTR))
							{
								std::wstring szName(varProperty2.bstrVal);
								//names_array.push_back(szName);
								;
								ports_array.push_back(std::tuple<int, std::wstring>(nPort, szName));
							}
							else
							{
								ports_array.push_back(std::tuple<int, std::wstring>(nPort, _T("")));
								//names_array.push_back(_T(""));
							}
						}
					}
				}
			}

			apObj[0].Release();
		}
	}

	// Close down COM
	CoUninitialize();
	return TRUE;
}

BOOL SerialUtilsEnumerator::UpdatePortListSetupAPI()
{
	//return QueryUsingSetupAPI(GUID_DEVINTERFACE_COMPORT, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	return QueryUsingSetupAPI(GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR, DIGCF_PRESENT);
}

BOOL SerialUtilsEnumerator::UpdatePortList()
{
	return TRUE;
}

bool SerialUtilsEnumerator::IsNumeric(LPCSTR pszString, bool bIgnoreColon)
{
	size_t nLen = strlen(pszString);
	if (nLen == 0)
	{
		return false;
	}

	//What will be the return value from this function (assume the best)
	bool bNumeric = true;

	for (size_t i = 0; i<nLen && bNumeric; i++)
	{
		bNumeric = (isdigit(static_cast<int>(pszString[i])) != 0);
		if (bIgnoreColon && (pszString[i] == ':'))
		{
			bNumeric = true;
		}
	}

	return bNumeric;
}

bool SerialUtilsEnumerator::IsNumeric(LPCWSTR pszString, bool bIgnoreColon)
{
	size_t nLen = wcslen(pszString);
	if (nLen == 0)
		return false;

	//What will be the return value from this function (assume the best)
	bool bNumeric = true;

	for (size_t i = 0; i<nLen && bNumeric; i++)
	{
		bNumeric = (iswdigit(pszString[i]) != 0);
		if (bIgnoreColon && (pszString[i] == L':'))
			bNumeric = true;
	}

	return bNumeric;
}

BOOL SerialUtilsEnumerator::QueryRegistryPortName(ATL::CRegKey& deviceKey, int& nPort)
{
	//What will be the return value from the method (assume the worst)
	BOOL bAdded = FALSE;

	//Read in the name of the port
	LPTSTR pszPortName = nullptr;
	if (RegQueryValueString(deviceKey, _T("PortName"), pszPortName))
	{
		//If it looks like "COMX" then
		//add it to the array which will be returned
		size_t nLen = _tcslen(pszPortName);
		if (nLen > 3)
		{
			if ((_tcsnicmp(pszPortName, _T("COM"), 3) == 0) && IsNumeric((pszPortName + 3), FALSE))
			{
				//Work out the port number
				nPort = _ttoi(pszPortName + 3);

				bAdded = TRUE;
			}
		}
		LocalFree(pszPortName);
	}

	return bAdded;
}

BOOL SerialUtilsEnumerator::RegQueryValueString(ATL::CRegKey& key, LPCTSTR lpValueName, LPTSTR& pszValue)
{
	//Initialize the output parameter
	pszValue = nullptr;

	//First query for the size of the registry value 
	ULONG nChars = 0;
	LSTATUS nStatus = key.QueryStringValue(lpValueName, nullptr, &nChars);
	if (nStatus != ERROR_SUCCESS)
	{
		SetLastError(nStatus);
		return FALSE;
	}

	//Allocate enough bytes for the return value
	DWORD dwAllocatedSize = ((nChars + 1) * sizeof(TCHAR)); //+1 is to allow us to null terminate the data if required
	pszValue = reinterpret_cast<LPTSTR>(LocalAlloc(LMEM_FIXED, dwAllocatedSize));
	if (pszValue == nullptr)
		return FALSE;

	//We will use RegQueryValueEx directly here because ATL::CRegKey::QueryStringValue does not handle non-null terminated data
	DWORD dwType = 0;
	ULONG nBytes = dwAllocatedSize;
	pszValue[0] = _T('\0');
	nStatus = RegQueryValueEx(key, lpValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(pszValue), &nBytes);
	if (nStatus != ERROR_SUCCESS)
	{
		LocalFree(pszValue);
		pszValue = nullptr;
		SetLastError(nStatus);
		return FALSE;
	}
	if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
	{
		LocalFree(pszValue);
		pszValue = nullptr;
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
	if ((nBytes % sizeof(TCHAR)) != 0)
	{
		LocalFree(pszValue);
		pszValue = nullptr;
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
	if (pszValue[(nBytes / sizeof(TCHAR)) - 1] != _T('\0'))
	{
		//Forcibly null terminate the data ourselves
		pszValue[(nBytes / sizeof(TCHAR))] = _T('\0');
	}

	return TRUE;
}

BOOL SerialUtilsEnumerator::QueryUsingSetupAPI(const GUID& guid, DWORD dwFlags)
{
	//Set our output parameters to sane defaults
	ports_array.clear();

	//Create a "device information set" for the specified GUID
	HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, nullptr, nullptr, dwFlags);
	if (hDevInfoSet == INVALID_HANDLE_VALUE)
		return FALSE;

	//Finally do the enumeration
	BOOL bMoreItems = TRUE;
	int nIndex = 0;
	SP_DEVINFO_DATA devInfo;
	while (bMoreItems)
	{
		//Enumerate the current device
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
		bMoreItems = SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo);
		if (bMoreItems)
		{
			//Did we find a serial port for this device
			BOOL bAdded = FALSE;

			//Get the registry key which stores the ports settings
			ATL::CRegKey deviceKey;
			int nPort = 0;
			deviceKey.Attach(SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
			if (deviceKey != INVALID_HANDLE_VALUE)
			{
				if (QueryRegistryPortName(deviceKey, nPort))
				{
					bAdded = TRUE;
				}
			}

			//If the port was a serial port, then also try to get its friendly name
			if (bAdded)
			{
				ATL::CHeapPtr<BYTE> byFriendlyName;
				if (QueryDeviceDescription(hDevInfoSet, devInfo, byFriendlyName))
				{
					std::wstring szName(reinterpret_cast<LPCTSTR>(byFriendlyName.m_pData));
					ports_array.push_back(std::tuple<int, std::wstring>(nPort, szName));
				}
				else
				{
					ports_array.push_back(std::tuple<int, std::wstring>(nPort, _T("")));
				}
			}
		}

		++nIndex;
	}

	//Free up the "device information set" now that we are finished with it
	SetupDiDestroyDeviceInfoList(hDevInfoSet);

	//Return the success indicator
	return TRUE;
}

BOOL SerialUtilsEnumerator::QueryDeviceDescription(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& devInfo, ATL::CHeapPtr<BYTE>& byFriendlyName)
{
	DWORD dwType = 0;
	DWORD dwSize = 0;
	//Query initially to get the buffer size required
	if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, nullptr, 0, &dwSize))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return FALSE;
	}

#pragma warning(suppress: 6102)
	if (!byFriendlyName.Allocate(dwSize))
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

	return SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, byFriendlyName.m_pData, dwSize, &dwSize) && (dwType == REG_SZ);
}