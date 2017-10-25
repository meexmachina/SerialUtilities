#pragma once

#include "../Serial/SerialEx.h"
#include <string>

class SerialUtilsComm : public CSerialEx
{
protected:
	unsigned int port_number;
	unsigned int baud_rate;

public:
	SerialUtilsComm(unsigned int nPort, unsigned int nBaud);
	virtual ~SerialUtilsComm();

	void OpenPort();
};

