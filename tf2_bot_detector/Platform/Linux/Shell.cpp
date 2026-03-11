#include "../Platform.h"
#include "Log.h"

#include <wordexp.h>

#include <cstdlib>
#include <filesystem>

using namespace tf2_bot_detector;

namespace
{
	void RunDetached(const std::string& command)
	{
		const auto fullCommand = command + " >/dev/null 2>&1 &";
		if (std::system(fullCommand.c_str()) != 0)
			LogWarning(MH_SOURCE_LOCATION_CURRENT(), "Failed to execute {}", command);
	}
}

std::vector<std::string> tf2_bot_detector::Shell::SplitCommandLineArgs(const std::string_view& cmdline)
{
	std::vector<std::string> args;
	wordexp_t expanded{};
	const std::string command(cmdline);
	if (wordexp(command.c_str(), &expanded, 0) != 0)
		return args;

	for (size_t i = 0; i < expanded.we_wordc; ++i)
		args.emplace_back(expanded.we_wordv[i]);

	wordfree(&expanded);
	return args;
}

std::filesystem::path tf2_bot_detector::Shell::BrowseForFolderDialog()
{
	return {};
}

void tf2_bot_detector::Shell::ExploreToAndSelect(std::filesystem::path path)
{
	ExploreTo(path.parent_path());
}

void tf2_bot_detector::Shell::ExploreTo(const std::filesystem::path& path)
{
	RunDetached("xdg-open \"" + path.string() + "\"");
}

void tf2_bot_detector::Shell::OpenURL(const char* url)
{
	RunDetached("xdg-open \"" + std::string(url) + "\"");
}