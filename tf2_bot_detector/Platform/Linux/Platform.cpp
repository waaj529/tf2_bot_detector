#include "Platform/Platform.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

using namespace tf2_bot_detector;

const std::error_code tf2_bot_detector::Platform::ErrorCodes::PRIVILEGE_NOT_HELD(EPERM, std::generic_category());

tf2_bot_detector::Platform::OS tf2_bot_detector::Platform::GetOS()
{
	return OS::Linux;
}

tf2_bot_detector::Platform::Arch tf2_bot_detector::Platform::GetArch()
{
	static const Platform::Arch s_Arch = []
	{
		utsname info{};
		if (uname(&info) != 0)
			return sizeof(void*) >= 8 ? Arch::x64 : Arch::x86;

		return std::string_view(info.machine).find("64") != std::string_view::npos ? Arch::x64 : Arch::x86;
	}();

	return s_Arch;
}

bool tf2_bot_detector::Platform::IsDebuggerAttached()
{
	std::ifstream status("/proc/self/status");
	std::string line;
	while (std::getline(status, line))
	{
		if (!line.starts_with("TracerPid:"))
			continue;

		const auto value = std::atoi(line.c_str() + std::strlen("TracerPid:"));
		return value != 0;
	}

	return false;
}

bool tf2_bot_detector::Platform::IsPortAvailable(uint16_t port)
{
	const int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD < 0)
		return false;

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	const bool ok = bind(socketFD, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
	close(socketFD);
	return ok;
}