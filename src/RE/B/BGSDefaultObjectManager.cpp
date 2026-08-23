#include "RE/B/BGSDefaultObjectManager.h"

#include "SKSE/Version.h"

using namespace REL;

namespace RE
{
	namespace
	{
		constexpr auto kInvalid = (std::numeric_limits<std::size_t>::max)();

		inline std::size_t MapIndex(std::underlying_type_t<DefaultObjectID> a_idx) noexcept
		{
			if (a_idx <= std::to_underlying(DefaultObjectID::kKeywordActivatorFurnitureNoPlayer)) {
				return a_idx;
			}
			std::size_t result;
			if SKYRIM_REL_CONSTEXPR (Module::IsVR()) {
				result = (0xFFFF0000 & a_idx) >> 16;
			} else {
				result = 0x0000FFFF & a_idx;
				// AE 1.7.99 inserts objects at index 188 and 264-268, shifting the
				// pre-1.7.99 indices past those points.
				if (Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99)) {
					if (result >= 263) {
						result += 6;
					} else if (result >= 188) {
						result += 1;
					}
				}
			}
			return result ? result : kInvalid;
		}
	}

	TESForm** BGSDefaultObjectManager::GetObject(DefaultObjectID a_object) noexcept
	{
		auto idx = MapIndex(std::to_underlying(a_object));
		if (idx == kInvalid) {
			return nullptr;
		}
		assert(idx < ObjectCount());
		return IsObjectInitialized(idx) ?
		           &(&RelocateMember<TESForm*>(this, 0x20, 0x20))[idx] :
		           nullptr;
	}

	std::size_t BGSDefaultObjectManager::ObjectCount() noexcept
	{
		// The initialized flags sit directly after the TESForm* array at 0x20, so the
		// flag offset is always 0x20 + count * 8. Verified against the binaries:
		// SE 1.5.97 = 364 (0xB80), AE 1.6.1170 = 366 (0xB90), AE 1.7.99 = 372 (0xBC0),
		// VR = 369 (0xBA8). Note SE and AE differ -- AE appends two objects.
		if SKYRIM_REL_CONSTEXPR (Module::IsVR()) {
			return 369;
		}
		if (Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99)) {
			return 372;
		}
		return Module::IsAE() ? 366 : 364;
	}

	bool BGSDefaultObjectManager::IsObjectInitialized(DefaultObjectID a_object) const noexcept
	{
		const auto idx = MapIndex(std::to_underlying(a_object));
		return idx != kInvalid && IsObjectInitialized(idx);
	}

	bool BGSDefaultObjectManager::IsObjectInitialized(std::size_t a_idx) const noexcept
	{
		const auto count = ObjectCount();
		if (a_idx >= count) {
			return false;
		}
		const auto flags = reinterpret_cast<const bool*>(reinterpret_cast<std::uintptr_t>(this) + 0x20 + count * sizeof(TESForm*));
		return flags[a_idx];
	}

	TESForm** BGSDefaultObjectManager::GetAe1799Object(Ae1799Object a_object) noexcept
	{
		if (!Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99)) {
			return nullptr;
		}
		const auto idx = std::to_underlying(a_object);
		return IsObjectInitialized(idx) ?
		           &(&RelocateMember<TESForm*>(this, 0x20, 0x20))[idx] :
		           nullptr;
	}

	bool BGSDefaultObjectManager::SupportsVR(DefaultObjectID a_object) noexcept
	{
		auto idx = std::to_underlying(a_object);
		return idx <= std::to_underlying(DefaultObjectID::kKeywordActivatorFurnitureNoPlayer) || idx & 0xFFFF0000;
	}

	bool BGSDefaultObjectManager::SupportsSE(DefaultObjectID a_object) noexcept
	{
		return (std::to_underlying(a_object) & 0x0000FFFF) || a_object != DefaultObjectID::kWerewolfSpell;
	}

	bool BGSDefaultObjectManager::SupportsCurrentRuntime(DefaultObjectID a_object) noexcept
	{
		return MapIndex(std::to_underlying(a_object)) != kInvalid;
	}
}
