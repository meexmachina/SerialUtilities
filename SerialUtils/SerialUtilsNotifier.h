#pragma once

#include <list>
#include <thread>
#include <string>
#include <Windows.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

class SerialUtilsNotifier
{
public:
	typedef enum
	{
		DeviceConnection,
		DeviceDisconnection,
	} EventType_en;

	typedef void(*notification_callback_function)(const EventType_en event_type, const std::string&);
	typedef int notification_handle;
private:
	static std::thread notification_thread;
	static bool notification_thread_running;
	static DWORD scheduler_thread_id;
	static std::list<notification_callback_function> callback_vector;
	static void notification_thread_func(void* instance);
	static std::list<notification_callback_function>::iterator FindCallback(notification_handle handle);
	static void Initialize(HINSTANCE module_instance);
	static void Destroy();
	static void NotifyAll(EventType_en event_type, std::string comm_port);
	static LRESULT CALLBACK InternalCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:

	static notification_handle RegisterCallback(SerialUtilsNotifier::notification_callback_function cb,
		HINSTANCE module_instance = HINST_THISCOMPONENT);
	static void UnregisterCallback(notification_handle handle);
};

