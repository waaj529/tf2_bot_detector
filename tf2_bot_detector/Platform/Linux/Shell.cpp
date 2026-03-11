#include "../Platform.h"
#include "Log.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <wordexp.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

using namespace tf2_bot_detector;

namespace
{
	bool RunDetached(const std::vector<std::string>& command)
	{
		if (command.empty())
			return false;

		std::vector<std::string> argvStorage = command;
		std::vector<char*> argv;
		argv.reserve(argvStorage.size() + 1);
		for (auto& arg : argvStorage)
			argv.push_back(arg.data());
		argv.push_back(nullptr);

		const auto pid = fork();
		if (pid < 0)
			return false;

		if (pid == 0)
		{
			const auto grandChild = fork();
			if (grandChild == 0)
			{
				execvp(argv.front(), argv.data());
				_exit(127);
			}

			_exit(grandChild < 0 ? 127 : 0);
		}

		int status = 0;
		(void)waitpid(pid, &status, 0);

		return true;
	}

	std::filesystem::path RunCaptureCommand(const char* command)
	{
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command, "r"), &pclose);
		if (!pipe)
			return {};

		std::string output;
		char buffer[512]{};
		while (fgets(buffer, sizeof(buffer), pipe.get()))
			output += buffer;

		while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
			output.pop_back();

		if (output.empty())
			return {};

		return std::filesystem::path(output);
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
	if (const auto result = RunCaptureCommand("zenity --file-selection --directory 2>/dev/null"); !result.empty())
		return result;

	if (const auto result = RunCaptureCommand("kdialog --getexistingdirectory \"$HOME\" 2>/dev/null"); !result.empty())
		return result;

	return {};
}

void tf2_bot_detector::Shell::ExploreToAndSelect(std::filesystem::path path)
{
	ExploreTo(path.parent_path());
}

void tf2_bot_detector::Shell::ExploreTo(const std::filesystem::path& path)
{
	if (!RunDetached({ "xdg-open", path.string() }))
		LogWarning(MH_SOURCE_LOCATION_CURRENT(), "Failed to execute xdg-open for {}", path);
}

void tf2_bot_detector::Shell::OpenURL(const char* url)
{
	if (!url || !*url)
		return;

	if (!RunDetached({ "xdg-open", url }))
		LogWarning(MH_SOURCE_LOCATION_CURRENT(), "Failed to execute xdg-open for URL");
}