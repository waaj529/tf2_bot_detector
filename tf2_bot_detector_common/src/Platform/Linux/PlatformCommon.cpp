#include "Platform/PlatformCommon.h"

#include <mh/text/format.hpp>

#include <dlfcn.h>

#include <stdexcept>

void* tf2_bot_detector::Platform::GetProcAddressHelper(const char* moduleName, const char* symbolName,
	bool isCritical, const mh::source_location& location)
{
	if (!moduleName)
		throw std::invalid_argument("moduleName was nullptr");
	if (!moduleName[0])
		throw std::invalid_argument("moduleName was empty");
	if (!symbolName)
		throw std::invalid_argument("symbolName was nullptr");
	if (!symbolName[0])
		throw std::invalid_argument("symbolName was empty");

	dlerror();
	void* moduleHandle = dlopen(moduleName, RTLD_LAZY | RTLD_NOLOAD);
	if (!moduleHandle)
		moduleHandle = dlopen(moduleName, RTLD_LAZY);

	if (!moduleHandle)
		throw std::runtime_error(mh::format("Failed to dlopen({}): {}", moduleName, dlerror()));

	dlerror();
	void* address = dlsym(moduleHandle, symbolName);
	if (const char* error = dlerror(); error && isCritical)
	{
		throw std::runtime_error(mh::format("{}: Failed to find function {} in {}: {}",
			location, symbolName, moduleName, error));
	}

	return address;
}