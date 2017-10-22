// DeviceNotification2.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SerialUtils.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <dbt.h>
#include <devguid.h>
#include <list>
#include <thread>
#include <atlbase.h>
#include <vector>
#include <atlstr.h>
#include <Wbemidl.h>
#include <comutil.h>
#include <tuple>

std::list<SerialUtilsNotifier::notification_callback_function> SerialUtilsNotifier::callback_vector;
std::thread SerialUtilsNotifier::notification_thread;
std::vector<std::tuple<unsigned int, std::wstring>> SerialUtilsNotifier::ports_array;
bool SerialUtilsNotifier::notification_thread_running = false;

LRESULT CALLBACK SerialUtilsNotifier::InternalCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HDR ph = (PDEV_BROADCAST_HDR)lParam;
	_DEV_BROADCAST_USERDEFINED* pud = (_DEV_BROADCAST_USERDEFINED*)lParam;
	switch (message)
	{
	case WM_DEVICECHANGE:

		switch (wParam)
		{
		case DBT_CONFIGCHANGECANCELED:
			//printf("A request to change the current configuration (dock or undock) has been canceled.\n");
			break;

		case DBT_CONFIGCHANGED: 
			//printf("The current configuration has changed, due to a dock or undock.\n");
			break;

		case DBT_CUSTOMEVENT:
			//printf("A custom event has occurred (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			break;

		case DBT_DEVICEARRIVAL: 
			//printf("A device or piece of media has been inserted and is now available (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			// Check what was inserted
			if (ph->dbch_devicetype == DBT_DEVTYP_PORT)
			{
				PDEV_BROADCAST_PORT port_info = (PDEV_BROADCAST_PORT)lParam;
				std::wstring str = port_info->dbcp_name;
				//wprintf(L"	DeviceName = %s\n", port_info->dbcp_name);
				//UpdatePortList();
				NotifyAll(SerialUtilsNotifier::EventType_en::DeviceConnection, std::string(str.begin(), str.end()));
			}
			break;

		case DBT_DEVICEQUERYREMOVE: 
			//printf("Permission is requested to remove a device or piece of media. Any application can deny this request and cancel the removal (Devtype = %d Size = %d).\n", 
			//	ph->dbch_devicetype, ph->dbch_size);
			break;

		case DBT_DEVICEQUERYREMOVEFAILED: 
			//printf("A request to remove a device or piece of media has been canceled (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			break;

		case DBT_DEVICEREMOVECOMPLETE: 
			//printf("A device or piece of media has been removed (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			// Check what was removed
			if (ph->dbch_devicetype == DBT_DEVTYP_PORT)
			{
				PDEV_BROADCAST_PORT port_info = (PDEV_BROADCAST_PORT)lParam;
				std::wstring str = port_info->dbcp_name;
				//wprintf(L"	DeviceName = %s\n", port_info->dbcp_name);
				//UpdatePortList();
				NotifyAll(SerialUtilsNotifier::EventType_en::DeviceDisconnection, std::string(str.begin(), str.end()));
			}
			break;

		case DBT_DEVICEREMOVEPENDING: 
			//printf("A device or piece of media is about to be removed. Cannot be denied (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			break;

		case DBT_DEVICETYPESPECIFIC: 
			//printf("A device-specific event has occurred (Devtype = %d Size = %d).\n", ph->dbch_devicetype, ph->dbch_size);
			break;

		case DBT_DEVNODES_CHANGED: 
			//printf("A device has been added to or removed from the system.\n");
			break;

		case DBT_QUERYCHANGECONFIG: 
			//printf("Permission is requested to change the current configuration (dock or undock).\n");
			break;

		case DBT_USERDEFINED: 
			//printf("The meaning of this message is user-defined (Devtype = %d Size = %d Name = %s).\n", 
			//	pud->dbud_dbh.dbch_devicetype, pud->dbud_dbh.dbch_size, pud->dbud_szName);
			break;
		default: break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HRESULT SerialUtilsNotifier::UpdatePortList()
{
	//Initialize COM (Required by CEnumerateSerial::UsingWMI)
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to initialize COM, Error:%x\n"), hr);
		return -1;
	}

	//Initialize COM security (Required by CEnumerateSerial::UsingWMI)
	hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to initialize COM security, Error:%08X\n"), hr);
		CoUninitialize();
		return -1;
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
			return hr;

		CComPtr<IWbemServices> pServices;
		hr = pLocator->ConnectServer(_bstr_t("\\\\.\\root\\cimv2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pServices);
		if (FAILED(hr))
			return hr;

		//Execute the query
		CComPtr<IEnumWbemClassObject> pClassObject;
		hr = pServices->CreateInstanceEnum(_bstr_t("Win32_SerialPort"), WBEM_FLAG_RETURN_WBEM_COMPLETE, nullptr, &pClassObject);
		if (FAILED(hr))
			return hr;

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
								ports_array.push_back({ nPort, szName });
							}
							else
							{
								ports_array.push_back({ nPort, _T("") });
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
	return S_OK;
}

bool SerialUtilsNotifier::IsNumeric(LPCSTR pszString, bool bIgnoreColon)
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

bool SerialUtilsNotifier::IsNumeric(LPCWSTR pszString, bool bIgnoreColon)
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

void SerialUtilsNotifier::NotifyAll(EventType_en event_type, std::string comm_port)
{
	std::list<SerialUtilsNotifier::notification_callback_function>::iterator it = callback_vector.begin();
	for (;it != callback_vector.end();++it)
	{
		if (*it) (*it)(event_type, comm_port);
	}
}

void SerialUtilsNotifier::notification_thread_func(void* instance)
{
	HINSTANCE hInstance = (HINSTANCE)instance;

	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = InternalCallback;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"SerialUtils";
	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowW(L"SerialUtils", L"SerialUtilsNotifier", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		//return FALSE;
	}
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(NotificationFilter);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_reserved = 0;

	NotificationFilter.dbcc_classguid = GUID_DEVCLASS_USB;
	HDEVNOTIFY hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

	MSG msg;
	while (notification_thread_running)
	{
		if (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			//Sleep(100);
		}
	}
}

void SerialUtilsNotifier::Initialize(HINSTANCE module_instance)
{
	callback_vector.clear();
	UpdatePortList();
	notification_thread_running = true;
	notification_thread = std::thread(SerialUtilsNotifier::notification_thread_func, (void*)module_instance);
}

void SerialUtilsNotifier::Destroy()
{
	notification_thread_running = false;
	if (notification_thread.joinable())
		notification_thread.join();
	callback_vector.clear();
}

SerialUtilsNotifier::notification_handle SerialUtilsNotifier::RegisterCallback(SerialUtilsNotifier::notification_callback_function cb,
	HINSTANCE module_instance)
{
	int size_of_vec = callback_vector.size();

	if (size_of_vec == 0)
	{
		Initialize(module_instance);
	}

	callback_vector.push_back(cb);
	return size_of_vec;
}

std::list<SerialUtilsNotifier::notification_callback_function>::iterator 
			SerialUtilsNotifier::FindCallback(SerialUtilsNotifier::notification_handle handle)
{
	std::list<SerialUtilsNotifier::notification_callback_function>::iterator it = callback_vector.begin();
	for (int c = 0;
		it != callback_vector.end();
		++it, c++)
	{
		if (handle == c) break;
	}
	return it;
}

void SerialUtilsNotifier::UnregisterCallback(SerialUtilsNotifier::notification_handle handle)
{
	if (callback_vector.size() == 0) return;
	if (callback_vector.size() == 1)
	{
		Destroy();
		return;
	}
	callback_vector.remove(*(FindCallback(handle)));
}
