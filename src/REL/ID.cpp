#include "REL/ID.h"

#include "REX/W32/KERNEL32.h"

#ifdef ENABLE_SKYRIM_VR
#	include <rapidcsv.h>
#endif

namespace REL
{
	namespace detail
	{
		bool memory_map::open(stl::zwstring a_name, std::size_t a_size)
		{
			close();

			REX::W32::ULARGE_INTEGER bytes;
			bytes.value = a_size;

			_mapping = REX::W32::OpenFileMappingW(
				REX::W32::FILE_MAP_READ | REX::W32::FILE_MAP_WRITE,
				false,
				a_name.data());
			if (!_mapping) {
				close();
				return false;
			}

			_view = REX::W32::MapViewOfFile(
				_mapping,
				REX::W32::FILE_MAP_READ | REX::W32::FILE_MAP_WRITE,
				0,
				0,
				bytes.value);
			if (!_view) {
				close();
				return false;
			}

			return true;
		}

		bool memory_map::create(stl::zwstring a_name, std::size_t a_size)
		{
			close();

			REX::W32::ULARGE_INTEGER bytes;
			bytes.value = a_size;

			_mapping = REX::W32::OpenFileMappingW(
				REX::W32::FILE_MAP_READ | REX::W32::FILE_MAP_WRITE,
				false,
				a_name.data());
			if (!_mapping) {
				_mapping = REX::W32::CreateFileMappingW(
					REX::W32::INVALID_HANDLE_VALUE,
					nullptr,
					REX::W32::PAGE_READWRITE,
					bytes.hi,
					bytes.lo,
					a_name.data());
				if (!_mapping) {
					return false;
				}
			}

			_view = REX::W32::MapViewOfFile(
				_mapping,
				REX::W32::FILE_MAP_READ | REX::W32::FILE_MAP_WRITE,
				0,
				0,
				bytes.value);
			if (!_view) {
				return false;
			}

			return true;
		}

		void memory_map::close()
		{
			if (_view) {
				REX::W32::UnmapViewOfFile(static_cast<const void*>(_view));
				_view = nullptr;
			}

			if (_mapping) {
				REX::W32::CloseHandle(_mapping);
				_mapping = nullptr;
			}
		}
	}

	IDDatabase IDDatabase::_instance;

	bool IDDatabase::load_file(stl::zwstring a_filename, Version a_version, std::uint8_t a_formatVersion, bool a_failOnError)
	{
		try {
			istream_t  in(a_filename.data(), std::ios::in | std::ios::binary);
			const auto format = in.readout<std::int32_t>();

			// AE 1.7.99 introduced format 5, so an AE runtime accepts either the
			// delta coded format 2 or the dense format 5. SE stays on format 1.
			if (format == 5 && a_formatVersion >= 2) {
				return load_dense(in, a_filename, a_version, a_failOnError);
			}
			if (format != a_formatVersion) {
				return stl::report_and_error(
					std::format(
						"Unsupported address library format: {}\n"
						"This means this script extender plugin is incompatible with the address "
						"library available for this version of the game, and thus does not "
						"support it."sv,
						format),
					a_failOnError);
			}

			header_t header;
			header.read(in);
			if (header.version() != a_version) {
				return stl::report_and_error("version mismatch"sv, a_failOnError);
			}

			auto mapname = L"CommonLibSSEOffsets-v2-"s;
			mapname += a_version.wstring();
			const auto byteSize = static_cast<std::size_t>(header.address_count()) * sizeof(mapping_t);
			if (_mmap.open(mapname, byteSize)) {
				_id2offset = { static_cast<mapping_t*>(_mmap.data()), header.address_count() };
			} else if (_mmap.create(mapname, byteSize)) {
				_id2offset = { static_cast<mapping_t*>(_mmap.data()), header.address_count() };
				unpack_file(in, header, a_failOnError);
				std::sort(_id2offset.begin(), _id2offset.end(), [](auto&& a_lhs, auto&& a_rhs) {
					return a_lhs.id < a_rhs.id;
				});
			} else {
				return stl::report_and_error("failed to create shared mapping"sv, a_failOnError);
			}
		} catch (const std::system_error&) {
			return stl::report_and_error(
				std::format(
					"Failed to locate an appropriate address library with the path: {}\n"
					"This means you are missing the address library for this specific version of "
					"the game. Please continue to the mod page for address library to download "
					"an appropriate version. If one is not available, then it is likely that "
					"address library has not yet added support for this version of the game."sv,
					stl::utf16_to_utf8(a_filename).value_or("<unknown filename>"s)),
				a_failOnError);
			return false;
		}

		return true;
	}

	// Format 5, used by the AE 1.7.99 address library. After the format field:
	//
	//   0x04  u32 version[4]
	//   0x14  char name[64], fixed size
	//   0x54  u32 pointer size
	//   0x58  u32 reserved
	//   0x5C  u32 count
	//   0x60  u32 rva[count], one RVA per ID, 0 marks an unused ID
	//
	// The array is indexed directly by ID, so the file is read into a private
	// buffer as is and no shared mapping is involved.
	bool IDDatabase::load_dense(istream_t& a_in, stl::zwstring a_filename, Version a_version, bool a_failOnError)
	{
		std::uint32_t rawVersion[4]{};
		a_in.readin(rawVersion);

		Version version;
		for (std::size_t i = 0; i < std::extent_v<decltype(rawVersion)>; ++i) {
			version[i] = static_cast<std::uint16_t>(rawVersion[i]);
		}
		if (version != a_version) {
			return stl::report_and_error("version mismatch"sv, a_failOnError);
		}

		a_in.ignore(64);

		const auto pointerSize = a_in.readout<std::uint32_t>();
		std::ignore = a_in.readout<std::uint32_t>();
		const auto count = a_in.readout<std::uint32_t>();

		constexpr std::uint64_t headerSize = 0x60;
		std::error_code         ec;
		const auto              fileSize = std::filesystem::file_size(a_filename.data(), ec);
		if (ec || pointerSize != 8 ||
			fileSize != headerSize + static_cast<std::uint64_t>(count) * sizeof(std::uint32_t)) {
			return stl::report_and_error(
				std::format(
					"Malformed address library file: {}"sv,
					stl::utf16_to_utf8(a_filename).value_or("<unknown filename>"s)),
				a_failOnError);
		}

		_id2rva.resize(count);
		a_in.readin_bytes(_id2rva.data(), static_cast<std::streamsize>(count * sizeof(std::uint32_t)));

		return true;
	}

#ifdef ENABLE_SKYRIM_VR
	bool IDDatabase::load_csv(stl::zwstring a_filename, Version a_version, bool a_failOnError)
	{
		auto nstring = SKSE::stl::utf16_to_utf8(a_filename).value_or(""s);
		if (!std::filesystem::exists(nstring)) {
			return stl::report_and_error(
				std::format("Required VR Address Library file {} does not exist"sv, nstring),
				a_failOnError);
		}

		rapidcsv::Document in(nstring);
		std::size_t        id, address_count;
		std::string        version, offset;
		auto               mapname = L"CommonLibSSEOffsets-v2-"s;
		mapname += a_version.wstring();
		address_count = in.GetCell<std::size_t>(0, 0);
		version = in.GetCell<std::string>(1, 0);
		_vrAddressLibraryVersion = Version(version);
		const auto byteSize = static_cast<std::size_t>(address_count * sizeof(mapping_t));
		if (!_mmap.open(mapname, byteSize) &&
			!_mmap.create(mapname, byteSize)) {
			return stl::report_and_error("failed to create shared mapping"sv, a_failOnError);
		}

		_id2offset = { static_cast<mapping_t*>(_mmap.data()), static_cast<std::size_t>(address_count) };
		if (in.GetRowCount() > address_count + 1) {
			return stl::report_and_error(
				std::format("VR Address Library {} tried to exceed {} allocated entries."sv,
					version, address_count),
				a_failOnError);
		} else if (in.GetRowCount() < address_count + 1) {
			return stl::report_and_error(
				std::format("VR Address Library {} loaded only {} entries but expected {}. Please redownload."sv,
					version, in.GetRowCount() - 1, address_count),
				a_failOnError);
		}

		std::size_t index = 1;
		for (; index < in.GetRowCount(); ++index) {
			id = in.GetCell<std::size_t>(0, index);
			offset = in.GetCell<std::string>(1, index);
			_id2offset[index - 1] = { static_cast<std::uint64_t>(id),
				static_cast<std::uint64_t>(std::stoul(offset, nullptr, 16)) };
		}

		std::sort(_id2offset.begin(), _id2offset.end(), [](auto&& a_lhs, auto&& a_rhs) {
			return a_lhs.id < a_rhs.id;
		});

		return true;
	}

	bool IDDatabase::IsVRAddressLibraryAtLeastVersion(const char* a_minimalVRAddressLibVersion, bool a_reportAndFail) const
	{
		const auto minimalVersion = REL::Version(a_minimalVRAddressLibVersion);

		if (minimalVersion <= _vrAddressLibraryVersion) {
			return true;
		}

		if (!a_reportAndFail) {
			return false;
		}

		stl::report_and_fail(
			std::format("You need version: {} of VR Address Library for SKSEVR, you have version: {}"sv,
				minimalVersion, _vrAddressLibraryVersion));
	}
#endif
}
