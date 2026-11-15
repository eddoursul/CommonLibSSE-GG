#include "catch2/catch_all.hpp"

#include "SKSE/SKSE.h"

namespace
{
	void WriteLine(const std::filesystem::path& a_path, std::string_view a_line)
	{
		spdlog::logger logger("file_sink_test", SKSE::log::file_sink(a_path));
		logger.set_pattern("%v");
		logger.info("{}", a_line);
		logger.flush();
	}

	std::string ReadFile(const std::filesystem::path& a_path)
	{
		std::ifstream file(a_path, std::ios::binary);
		return { std::istreambuf_iterator<char>(file), {} };
	}
}

TEST_CASE("Logger/FileSinkWritesNonAnsiPath", "[unit]")
{
	// No ANSI code page contains Georgian, so path::string() throws for this directory,
	// unless the process code page is UTF-8. The sink must create the directory itself.
	const auto dir = std::filesystem::temp_directory_path() / L"CommonLibSSE ლოგი Журнал";
	const auto path = dir / L"ტესტი.log";
	std::filesystem::remove_all(dir);

	WriteLine(path, "first");
	CHECK(ReadFile(path) == "first\r\n");

	// A new sink truncates the file
	WriteLine(path, "second");
	CHECK(ReadFile(path) == "second\r\n");

	std::filesystem::remove_all(dir);
}
