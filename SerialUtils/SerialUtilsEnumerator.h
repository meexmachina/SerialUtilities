#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include <setupapi.h>
#include <atlbase.h>

#ifdef _DEBUG
# pragma comment(lib, "comsuppwd.lib")
#else
# pragma comment(lib, "comsuppw.lib")
#endif
# pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "setupapi.lib")


class SerialUtilsEnumerator
{
public:
	typedef std::vector<std::tuple<unsigned int, std::wstring>> PortList;

private:
	static PortList ports_array;

	static bool UpdatePortListWMI(PortList& ports_array_local);
	static bool UpdatePortListSetupAPI(PortList& ports_array_local);
	static bool UpdatePortList();
	static bool IsNumeric(LPCSTR pszString, bool bIgnoreColon);
	static bool IsNumeric(LPCWSTR pszString, bool bIgnoreColon);

	static bool QueryUsingSetupAPI(const GUID& guid, DWORD dwFlags, PortList& ports_array_local);
	static bool QueryDeviceDescription(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& devInfo, ATL::CHeapPtr<BYTE>& byFriendlyName);
	static bool QueryRegistryPortName(ATL::CRegKey& deviceKey, int& nPort);
	static bool RegQueryValueString(ATL::CRegKey& key, LPCTSTR lpValueName, LPTSTR& pszValue);
	
public:
	static PortList& GetFriendlyPortsList() { UpdatePortList(); return ports_array; }
};

