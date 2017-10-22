#pragma once
#include "GenException.h"

class SerialUtilsException : public GenException
{
public:
	SerialUtilsException();
	virtual ~SerialUtilsException();
};

