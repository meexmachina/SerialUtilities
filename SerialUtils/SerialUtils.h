#pragma once

#include <list>
#include <thread>
#include <string>
#include <vector>
#include <Windows.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#ifdef _DEBUG
# pragma comment(lib, "comsuppwd.lib")
#else
# pragma comment(lib, "comsuppw.lib")
#endif
# pragma comment(lib, "wbemuuid.lib")

class SerialUtilsNotifier
{
public:
	typedef enum
	{
		DeviceConnection,
		DeviceDisconnection,
	} EventType_en;

	typedef void (*notification_callback_function)(const EventType_en event_type, const std::string&);
	typedef int notification_handle;
	typedef std::vector<std::tuple<unsigned int, std::wstring>> PortList;
private:
	static std::thread notification_thread;
	static bool notification_thread_running;
	static std::list<notification_callback_function> callback_vector;
	static std::vector<std::tuple<unsigned int, std::wstring>> ports_array;
	static void notification_thread_func(void* instance);
	static std::list<notification_callback_function>::iterator FindCallback(notification_handle handle);
	static void Initialize(HINSTANCE module_instance);
	static void Destroy();
	static void NotifyAll(EventType_en event_type, std::string comm_port);
	static LRESULT CALLBACK InternalCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static HRESULT UpdatePortList();
	static bool IsNumeric(LPCSTR pszString, bool bIgnoreColon);
	static bool IsNumeric(LPCWSTR pszString, bool bIgnoreColon);
public:
	
	static notification_handle RegisterCallback(SerialUtilsNotifier::notification_callback_function cb,
												HINSTANCE module_instance = HINST_THISCOMPONENT);
	static PortList& GetFriendlyPortsList() { UpdatePortList(); return ports_array; }
	static void UnregisterCallback(notification_handle handle);
};

