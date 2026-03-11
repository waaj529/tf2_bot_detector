#include "../Platform.h"

#include <vdf_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>

namespace
{
	std::optional<tf2_bot_detector::SteamID> TryReadMostRecentSteamID(const std::filesystem::path& steamDir)
	{
		const auto loginUsersPath = steamDir / "config" / "loginusers.vdf";
		if (!std::filesystem::exists(loginUsersPath))
			return std::nullopt;

		std::ifstream file(loginUsersPath);
		if (!file.good())
			return std::nullopt;

		auto root = tyti::vdf::read(file);

		std::shared_ptr<tyti::vdf::object> users;
		if (auto it = root.childs.find("users"); it != root.childs.end())
			users = it->second;
		else if (auto it = root.childs.find("Users"); it != root.childs.end())
			users = it->second;

		if (!users)
			return std::nullopt;

		for (const auto& [steamIDStr, userInfo] : users->childs)
		{
			if (!userInfo)
				continue;

			const auto mostRecent = userInfo->attribs.find("MostRecent");
			if (mostRecent == userInfo->attribs.end() || mostRecent->second != "1")
				continue;

			try
			{
				return tf2_bot_detector::SteamID(std::stoull(steamIDStr));
			}
			catch (const std::exception&)
			{
				continue;
			}
		}

		return std::nullopt;
	}
}

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

	if (const auto mostRecent = TryReadMostRecentSteamID(steamDir); mostRecent.has_value())
		return *mostRecent;

	const auto userdataDir = steamDir / "userdata";
	if (!std::filesystem::exists(userdataDir))
		return {};

	std::optional<std::pair<std::filesystem::file_time_type, SteamID>> fallbackResult;

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
			const auto lastWriteTime = std::filesystem::last_write_time(entry.path());
			auto steamID = SteamID(accountID, SteamAccountType::Individual, SteamAccountUniverse::Public);
			if (!fallbackResult || lastWriteTime > fallbackResult->first)
				fallbackResult = std::make_pair(lastWriteTime, steamID);
		}
		catch (const std::exception&)
		{
			continue;
		}
	}

	if (fallbackResult)
		return fallbackResult->second;

	return {};
}