// DeviceNotification2.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SerialUtils.h"
#include <stdio.h>
#include <tchar.h>
#include <dbt.h>
#include <devguid.h>
#include <thread>


std::list<SerialUtilsNotifier::notification_callback_function> SerialUtilsNotifier::callback_vector;
std::thread SerialUtilsNotifier::notification_thread;
bool SerialUtilsNotifier::notification_thread_running = false;
DWORD SerialUtilsNotifier::scheduler_thread_id = 0;

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


void SerialUtilsNotifier::NotifyAll(EventType_en event_type, std::string comm_port)
{
	std::list<SerialUtilsNotifier::notification_callback_function>::iterator it = callback_vector.begin();
	for (; it != callback_vector.end(); ++it)
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
		return;
	}
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(NotificationFilter);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_reserved = 0;

	NotificationFilter.dbcc_classguid = GUID_DEVCLASS_USB;
	HDEVNOTIFY hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

	scheduler_thread_id = GetCurrentThreadId();

	MSG msg;
	while (notification_thread_running)
	{
		if (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void SerialUtilsNotifier::Initialize(HINSTANCE module_instance)
{
	callback_vector.clear();
	notification_thread_running = true;
	notification_thread = std::thread(SerialUtilsNotifier::notification_thread_func, (void*)module_instance);
}

void SerialUtilsNotifier::Destroy()
{
	notification_thread_running = false;
	PostAppMessage(scheduler_thread_id, WM_CLOSE, 0, 0);
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
