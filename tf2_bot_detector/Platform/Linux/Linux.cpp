#include "Platform/Platform.h"

#include <unistd.h>

#include <array>
#include <cstdlib>
#include <filesystem>

std::filesystem::path tf2_bot_detector::Platform::GetCurrentExeDir()
{
	std::array<char, 4096> buffer{};
	const auto length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
	if (length <= 0)
		throw std::runtime_error("readlink(/proc/self/exe) failed");

	buffer[length] = '\0';
	return std::filesystem::path(buffer.data()).remove_filename();
}

std::filesystem::path tf2_bot_detector::Platform::GetLegacyAppDataDir()
{
	return {};
}

std::filesystem::path tf2_bot_detector::Platform::GetRootLocalAppDataDir()
{
	if (const char* xdgData = std::getenv("XDG_DATA_HOME"))
		return xdgData;

	if (const char* home = std::getenv("HOME"))
		return std::filesystem::path(home) / ".local/share";

	return std::filesystem::temp_directory_path();
}

std::filesystem::path tf2_bot_detector::Platform::GetRootRoamingAppDataDir()
{
	if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME"))
		return xdgConfig;

	if (const char* home = std::getenv("HOME"))
		return std::filesystem::path(home) / ".config";

	return std::filesystem::temp_directory_path();
}

std::filesystem::path tf2_bot_detector::Platform::GetRootTempDataDir()
{
	return std::filesystem::temp_directory_path();
}