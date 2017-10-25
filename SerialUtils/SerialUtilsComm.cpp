#include "stdafx.h"
#include "SerialUtilsComm.h"
#include "SerialUtilsException.h"

SerialUtilsComm::SerialUtilsComm(unsigned int nPort, unsigned int nBaud)
	: CSerialEx()
{
	port_number = nPort;
	baud_rate = nBaud;
}


SerialUtilsComm::~SerialUtilsComm()
{
}

void SerialUtilsComm::OpenPort()
{
	TCHAR port_address[32] = { 0 };
	wsprintf(port_address, L"\\\\.\\COM%d", port_number);

	if (Open(port_address, 10000, 10000, true) != 0)
	{
		throw SerialUtilsException();
	}
}