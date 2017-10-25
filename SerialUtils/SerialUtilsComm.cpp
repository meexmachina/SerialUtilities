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
	int err;
	wsprintf(port_address, L"\\\\.\\COM%d", port_number);

	err = Open(port_address, 10000, 10000, true);
	if (err != 0)
	{
		THROW_GEN_EXCEPTION("Port opening failed with error %d", err);
	}
}