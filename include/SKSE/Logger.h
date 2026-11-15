#pragma once

#define SKSE_MAKE_SOURCE_LOGGER(a_func, a_type)                           \
                                                                          \
	template <class... Args>                                              \
	struct [[maybe_unused]] a_func                                        \
	{                                                                     \
		a_func() = delete;                                                \
                                                                          \
		explicit a_func(                                                  \
			spdlog::format_string_t<Args...> a_fmt,                       \
			Args&&... a_args,                                             \
			std::source_location a_loc = std::source_location::current()) \
		{                                                                 \
			spdlog::log(                                                  \
				spdlog::source_loc{                                       \
					a_loc.file_name(),                                    \
					static_cast<int>(a_loc.line()),                       \
					a_loc.function_name() },                              \
				spdlog::level::a_type,                                    \
				a_fmt,                                                    \
				std::forward<Args>(a_args)...);                           \
		}                                                                 \
	};                                                                    \
                                                                          \
	template <class... Args>                                              \
	a_func(spdlog::format_string_t<Args...>, Args&&...) -> a_func<Args...>;

namespace SKSE::log
{
	SKSE_MAKE_SOURCE_LOGGER(trace, trace);
	SKSE_MAKE_SOURCE_LOGGER(debug, debug);
	SKSE_MAKE_SOURCE_LOGGER(info, info);
	SKSE_MAKE_SOURCE_LOGGER(warn, warn);
	SKSE_MAKE_SOURCE_LOGGER(error, err);
	SKSE_MAKE_SOURCE_LOGGER(critical, critical);

	[[nodiscard]] std::optional<std::filesystem::path> log_directory();

	// A thread safe sink that truncates a_path and writes to it, creating the parent directories.
	// Use it instead of spdlog's basic_file_sink: that takes a narrow path in the ANSI code page,
	// and std::filesystem::path::string() throws for a path the code page cannot represent, such
	// as a Documents folder under a Cyrillic account name on a Western Windows. This sink opens the
	// wide path. If the file cannot be opened, the sink drops every message.
	[[nodiscard]] spdlog::sink_ptr file_sink(const std::filesystem::path& a_path);

	void add_papyrus_sink(std::regex a_filter);
	void remove_papyrus_sink();

	void init();
}

#undef SKSE_MAKE_SOURCE_LOGGER
