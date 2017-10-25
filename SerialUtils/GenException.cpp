#include "stdafx.h"
#include "GenException.h"

unsigned int GenException::verbosity = GenException::Verbosity_en::Verb_None;
FILE* GenException::log_file = NULL;
int GenException::address_length = sizeof(GenException::address_length);
struct sockaddr_in GenException::address = { 0 };
int GenException::inet_sock = INVALID_SOCKET;

namespace Color {
	enum Code {
		FG_RED = 31,
		FG_GREEN = 32,
		FG_BLUE = 34,
		FG_DEFAULT = 39,
		BG_RED = 41,
		BG_GREEN = 42,
		BG_BLUE = 44,
		BG_DEFAULT = 49
	};
	class Modifier {
		Code code;
	public:
		Modifier(Code pCode) : code(pCode) {}
		friend std::ostream&
			operator<<(std::ostream& os, const Modifier& mod) {
			return os << "\033[" << mod.code << "m";
		}
	};
}

GenException::GenException() 
{
}

GenException::~GenException() 
{
}

GenException::GenException(char calling_function[256], char const* fmt, ...)
{
	when = time(0);
	char text[128];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf_s(text, sizeof text, fmt, ap);
	va_end(ap);
	what.append(text);
	where.append(calling_function);

	if (verbosity & Verbosity_en::Verb_PrintOnConsole || verbosity & Verbosity_en::Verb_LogToFile)
	{
		char date_string[64] = { 0 };
		ctime_s(date_string, 64, &when);

		if (verbosity & Verbosity_en::Verb_PrintOnConsole)
		{
			Color::Modifier red(Color::FG_RED);
			Color::Modifier def(Color::FG_DEFAULT);

			std::cout << red << date_string << ": ";
			std::cout << "@ " << where << def << " - ";
			std::cout << what << std::endl;
		}
		else
		{
			char file_text[512] = { 0 };
			if (log_file != NULL) fprintf_s(log_file, "%s: @ %s - %s\n", date_string, where.c_str(), what.c_str());
		}
	}
	if (verbosity & Verbosity_en::Verb_LogToNetwork)
	{
		UDPPack_st pack = { 0 };
		pack.sync_word = 0xdeadbeef;
		pack.type = UDPPackType_en::ExceptionMessage;
		strcpy_s(pack.what, sizeof(pack.what), what.c_str());
		strcpy_s(pack.where, sizeof(pack.where), where.c_str());
		pack.when = when;

		sendto(inet_sock, (char*)&pack, sizeof(UDPPack_st), 0, (struct sockaddr *)&address, address_length);
	}
}

void GenException::SetLogToConsole(bool activate)
{
	if (activate)
	{
		verbosity |= Verbosity_en::Verb_PrintOnConsole;
	}
	else
	{
		verbosity &= ~Verbosity_en::Verb_PrintOnConsole;
	}
}

void GenException::SetLogToFile(bool activate, char *filename)
{
	if (activate)
	{
		errno_t err = fopen_s(&log_file, filename, "a");
		if (err == EINVAL)
		{
			log_file = NULL;
			return;
		}
		verbosity |= Verbosity_en::Verb_LogToFile;
	}
	else
	{
		verbosity &= ~Verbosity_en::Verb_LogToFile;
		if (log_file != NULL) fclose(log_file);
		log_file = NULL;
	}
}

void GenException::LogToNetwork(bool activate, char *ipaddress, unsigned short port)
{
	if (activate)
	{
#ifdef _MSC_VER
		WORD wVersionRequested = MAKEWORD(1, 1);
		WSADATA wsaData;
		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
			return;
		}
#endif
		memset((char *)&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		struct hostent *pHost = gethostbyname(ipaddress);
		memcpy(&address.sin_addr.s_addr, pHost->h_addr, pHost->h_length);

		if ((inet_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			return;
		}

		verbosity |= Verbosity_en::Verb_LogToNetwork;
	}
	else
	{
		verbosity &= ~Verbosity_en::Verb_LogToNetwork;
		closesocket(inet_sock);
		WSACleanup();
	}
}