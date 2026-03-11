#include "../Platform.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

using namespace tf2_bot_detector;

std::filesystem::path tf2_bot_detector::Platform::GetCurrentSteamDir()
{
	const auto valid = [](const std::filesystem::path& path)
	{
		return std::filesystem::exists(path / "steam.sh") || std::filesystem::exists(path / "Steam.sh");
	};

	if (const char* xdgData = std::getenv("XDG_DATA_HOME"))
	{
		const auto candidate = std::filesystem::path(xdgData) / "Steam";
		if (valid(candidate))
			return candidate;
	}

	const char* home = std::getenv("HOME");
	if (!home)
		return {};

	const std::filesystem::path homePath(home);
	for (const auto& candidate : {
		homePath / ".local/share/Steam",
		homePath / ".steam/steam",
		homePath / ".var/app/com.valvesoftware.Steam/data/Steam",
	})
	{
		if (valid(candidate))
			return candidate;
	}

	return {};
}

SteamID tf2_bot_detector::Platform::GetCurrentActiveSteamID()
{
	const auto steamDir = GetCurrentSteamDir();
	if (steamDir.empty())
		return {};

	const auto userdataDir = steamDir / "userdata";
	if (!std::filesystem::exists(userdataDir))
		return {};

	for (const auto& entry : std::filesystem::directory_iterator(userdataDir))
	{
		if (!entry.is_directory())
			continue;

		const auto dirname = entry.path().filename().string();
		if (dirname.empty() || !std::all_of(dirname.begin(), dirname.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
			continue;

		try
		{
			const auto accountID = static_cast<uint32_t>(std::stoull(dirname));
			return SteamID(accountID, SteamAccountType::Individual, SteamAccountUniverse::Public);
		}
		catch (const std::exception&)
		{
			continue;
		}
	}

	return {};
}