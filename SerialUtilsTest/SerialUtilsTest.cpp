// SerialUtilsTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../SerialUtils/SerialUtils.h"
#include <Windows.h>
#include <iostream>

void Callback(const SerialUtilsNotifier::EventType_en e, const std::string& comm_port)
{
	if (e == SerialUtilsNotifier::EventType_en::DeviceConnection)
	{
		std::cout << "Event: " << "Connected" << "  Port: " << comm_port << std::endl;
	}
	else
	{
		std::cout << "Event: " << "Disconnected" << "  Port: " << comm_port << std::endl;
	}
}

int main()
{
	SerialUtilsNotifier::notification_handle handle = SerialUtilsNotifier::RegisterCallback(Callback);
	SerialUtilsNotifier::PortList list = SerialUtilsNotifier::GetFriendlyPortsList();

	getchar();

	SerialUtilsNotifier::UnregisterCallback(handle);
    return 0;
}

