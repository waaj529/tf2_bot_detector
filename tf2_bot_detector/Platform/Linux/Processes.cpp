#include "../Platform.h"
#include "Log.h"

#include <mh/coroutine/future.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

using namespace std::string_literals;
using namespace tf2_bot_detector;

namespace
{
	bool IsNumeric(std::string_view value)
	{
		return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
	}

	std::string ReadFirstLine(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		std::string line;
		std::getline(file, line);
		return line;
	}

	std::string ReadCmdline(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		std::replace(data.begin(), data.end(), '\0', ' ');
		return data;
	}

	bool CommandMatches(const std::string& comm, const std::string& cmdline, std::string_view processName)
	{
		return comm == processName || cmdline.find(processName) != std::string::npos;
	}

	std::vector<std::pair<std::filesystem::path, std::string>> EnumerateProcesses()
	{
		std::vector<std::pair<std::filesystem::path, std::string>> processes;
		for (const auto& entry : std::filesystem::directory_iterator("/proc"))
		{
			if (!entry.is_directory())
				continue;

			const auto dirName = entry.path().filename().string();
			if (!IsNumeric(dirName))
				continue;

			processes.emplace_back(entry.path(), ReadFirstLine(entry.path() / "comm"));
		}

		return processes;
	}

	bool LaunchProcessDetached(const std::filesystem::path& executable, const std::vector<std::string>& args)
	{
		std::vector<std::string> argvStorage;
		argvStorage.reserve(args.size() + 1);
		argvStorage.emplace_back(executable.string());
		for (const auto& arg : args)
			argvStorage.push_back(arg);

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
}

bool tf2_bot_detector::Processes::IsTF2Running()
{
	for (const auto& [path, comm] : EnumerateProcesses())
	{
		const auto cmdline = ReadCmdline(path / "cmdline");
		if (CommandMatches(comm, cmdline, "hl2.exe") ||
			CommandMatches(comm, cmdline, "hl2_linux") ||
			CommandMatches(comm, cmdline, "tf2_linux64"))
		{
			return true;
		}
	}

	return false;
}

mh::task<std::vector<std::string>> tf2_bot_detector::Processes::GetTF2CommandLineArgsAsync()
{
	std::vector<std::string> args;
	for (const auto& [path, comm] : EnumerateProcesses())
	{
		const auto cmdline = ReadCmdline(path / "cmdline");
		if (CommandMatches(comm, cmdline, "hl2.exe") ||
			CommandMatches(comm, cmdline, "hl2_linux") ||
			CommandMatches(comm, cmdline, "tf2_linux64"))
		{
			args.push_back(cmdline);
		}
	}

	co_return args;
}

bool tf2_bot_detector::Processes::IsSteamRunning()
{
	return IsProcessRunning("steam") || IsProcessRunning("steamwebhelper");
}

bool tf2_bot_detector::Processes::IsProcessRunning(const std::string_view& processName)
{
	for (const auto& [path, comm] : EnumerateProcesses())
	{
		const auto cmdline = ReadCmdline(path / "cmdline");
		if (CommandMatches(comm, cmdline, processName))
			return true;
	}

	return false;
}

void tf2_bot_detector::Processes::RequireTF2NotRunning()
{
	if (!IsTF2Running())
		return;

	LogError("TF2 Bot Detector must be started before Team Fortress 2.");
	std::exit(1);
}

void tf2_bot_detector::Processes::Launch(const std::filesystem::path& executable,
	const std::vector<std::string>& args, bool elevated)
{
	(void)elevated;
	if (!LaunchProcessDetached(executable, args))
		LogError(MH_SOURCE_LOCATION_CURRENT(), "Failed to launch {}", executable);
}

void tf2_bot_detector::Processes::Launch(const std::filesystem::path& executable,
	const std::string_view& args, bool elevated)
{
	auto splitArgs = Shell::SplitCommandLineArgs(args);
	if (!LaunchProcessDetached(executable, splitArgs))
		LogError(MH_SOURCE_LOCATION_CURRENT(), "Failed to launch {}", executable);
}

int tf2_bot_detector::Processes::GetCurrentProcessID()
{
	return static_cast<int>(getpid());
}

size_t tf2_bot_detector::Processes::GetCurrentRAMUsage()
{
	std::ifstream status("/proc/self/status");
	std::string line;
	while (std::getline(status, line))
	{
		if (!line.starts_with("VmRSS:"))
			continue;

		std::istringstream iss(line.substr(std::strlen("VmRSS:")));
		size_t valueKB = 0;
		iss >> valueKB;
		return valueKB * 1024;
	}

	return 0;
}