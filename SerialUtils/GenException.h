#pragma once
#include <exception>
#include <ctime>
#include <string>
#include <chrono>
#include <windows.h>
#include <stdarg.h>
#include <iostream>
#include <ostream>
#include <errno.h>
#include <stdlib.h>
#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "WSock32.Lib")

#define THROW_GEN_EXCEPTION(FMT)	(throw GenException(__FUNCTION__, FMT);)

class GenException : public std::exception
{
public:
	typedef enum
	{
		Verb_None = 0x00,
		Verb_PrintOnConsole = 0x01,
		Verb_LogToFile = 0x02,
		Verb_LogToNetwork = 0x04,
	} Verbosity_en;

	typedef enum
	{
		ExceptionMessage = 0,

	} UDPPackType_en;

	typedef struct
	{
		unsigned int sync_word;
		UDPPackType_en type;
		std::time_t when;
		char what[256];
		char where[128];
	} UDPPack_st;

private:
	std::time_t when;				// Time of error
	std::string what;				// Description
	std::string where;				// In which function

	static unsigned int verbosity;
	static FILE* log_file;
	static struct sockaddr_in address;
	static int address_length;
	static int inet_sock;

public:
	GenException();
	GenException(char calling_function[256], char const* fmt, ...);
	virtual ~GenException();

	static void SetLogToConsole(bool activate);
	static void SetLogToFile(bool activate, char *filename);
	static void LogToNetwork(bool activate, char *ipaddress, unsigned short port);
};

