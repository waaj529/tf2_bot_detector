#include "Platform/Platform.h"

#include <unistd.h>

using namespace tf2_bot_detector;

bool tf2_bot_detector::Platform::IsInstalled()
{
	return false;
}

bool tf2_bot_detector::Platform::CanInstallUpdate(const BuildInfo& bi)
{
	(void)bi;
	return false;
}

mh::task<tf2_bot_detector::Platform::InstallUpdate::Result> tf2_bot_detector::Platform::BeginInstallUpdate(
	const BuildInfo& bi, const IHTTPClient& client)
{
	(void)bi;
	(void)client;
	co_return InstallUpdate::NeedsUpdateTool{ "" };
}

bool tf2_bot_detector::Platform::NeedsElevationToWrite(const std::filesystem::path& path, bool recursive)
{
	(void)recursive;
	auto target = path;
	if (target.has_filename())
		target = target.parent_path();
	if (target.empty())
		target = std::filesystem::current_path();

	return access(target.c_str(), W_OK) != 0;
}