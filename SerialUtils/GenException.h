#pragma once
#include "vcruntime_exception.h"
#include <exception>
#include <ctime>
#include <string>

class GenException : public std::exception
{
public:
	typedef enum
	{
		None,
		PrintOnConsole = 0x01,
		LogToFile = 0x02,
		LogToNetwork = 0x04,
	} Verbosity_en;

private:
	std::time_t when;
	std::string what;
	std::string where;
	std::string how;
public:
	GenException();
	virtual ~GenException();
};

