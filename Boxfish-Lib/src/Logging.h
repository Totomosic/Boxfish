#pragma once
#include <memory>
#include <string>
#include <functional>
#include <cstdint>

#if (!defined(EMSCRIPTEN) || !defined(BOX_DIST))
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

#define BOX_API

namespace Boxfish
{

	class BOX_API Logger
	{
	private:
		static bool s_Initialized;
		static std::shared_ptr<spdlog::logger> s_Logger;

	public:
		static void Init();
		static inline spdlog::logger& GetLogger() { return *s_Logger; }
	};

}

#endif

#ifdef BOX_DIST 
#define BOX_TRACE(...)
#define BOX_INFO(...)
#define BOX_WARN(...)
#define BOX_ERROR(...)
#define BOX_FATAL(...)

#define BOX_ASSERT(arg, ...)

#define BOX_DEBUG_ONLY(x)
#else

#define BOX_TRACE(...) ::Boxfish::Logger::GetLogger().trace(__VA_ARGS__)
#define BOX_INFO(...) ::Boxfish::Logger::GetLogger().info(__VA_ARGS__)
#define BOX_WARN(...) ::Boxfish::Logger::GetLogger().warn(__VA_ARGS__)
#define BOX_ERROR(...) ::Boxfish::Logger::GetLogger().error(__VA_ARGS__)
#define BOX_FATAL(...) ::Boxfish::Logger::GetLogger().critical(__VA_ARGS__)

#ifdef BOX_PLATFORM_WINDOWS
#define BOX_ASSERT(arg, ...) { if (!(arg)) { BOX_FATAL(__VA_ARGS__); __debugbreak(); } }
#else
#define BOX_ASSERT(arg, ...) { if (!(arg)) { BOX_FATAL(__VA_ARGS__); } }
#endif

#define BOX_DEBUG_ONLY(x) x
#endif
