#pragma once

#include "RE/B/BSIntrusiveRefCounted.h"
#include "REL/Relocation.h"
#include "SKSE/Version.h"

namespace RE
{
	class ButtonEvent;
	class InputEvent;
	class KinectEvent;
	class MouseMoveEvent;
#ifdef ENABLE_SKYRIM_AE
	class MotionGestureEvent;
	class SixaxisEvent;
#endif
	class ThumbstickEvent;
	class VrWandTouchpadPositionEvent;
	class VrWandTouchpadSwipeEvent;

	class MenuEventHandler : public BSIntrusiveRefCounted
	{
	public:
		inline static constexpr auto RTTI = RTTI_MenuEventHandler;
		inline static constexpr auto VTABLE = VTABLE_MenuEventHandler;

		MenuEventHandler() = default;
		virtual ~MenuEventHandler() = default;  // 00

		virtual bool CanProcess(InputEvent* a_event) = 0;  // 01

		// VR's vtable inserts three wand-touchpad input virtuals after CanProcess, shifting
		// ProcessKinect/Thumbstick/MouseMove/Button from slots 02-05 down to 05-08. The
		// MenuControls dispatcher routes INPUT_EVENT_TYPE::kVrTouchpadSwipe (7) -> slot 02 and
		// kVrTouchpadPosition (6) -> slot 03; slot 04 is not routed by MenuControls.
		//
		// AE 1.7.99 inserts its own pair after CanProcess (ProcessMotionGesture at 02,
		// ProcessSixaxis at 03), shifting the flat slots 02-05 to 04-07. A compile-time
		// override in a flat build would therefore bind to the wrong slot on 1.7.99, so
		// every non-VR-exclusive build dispatches these through runtime slot indices.
#if defined(EXCLUSIVE_SKYRIM_VR)
		virtual bool ProcessVrWandTouchpadSwipe(VrWandTouchpadSwipeEvent* a_event);        // VR 02 - { return false; }
		virtual bool ProcessVrWandTouchpadPosition(VrWandTouchpadPositionEvent* a_event);  // VR 03 - { return false; }
		virtual bool Unk_04(void* a_event);                                                // VR 04 - { return false; }
		virtual bool ProcessKinect(KinectEvent* a_event);                                  // VR 05 - { return false; }
		virtual bool ProcessThumbstick(ThumbstickEvent* a_event);                          // VR 06 - { return false; }
		virtual bool ProcessMouseMove(MouseMoveEvent* a_event);                            // VR 07 - { return false; }
		virtual bool ProcessButton(ButtonEvent* a_event);                                  // VR 08 - { return false; }
#else
#	ifdef ENABLE_SKYRIM_AE
#		define AE1799_SLOT_SHIFT(idx) (REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99) ? (idx) + 2 : (idx))
#	else
#		define AE1799_SLOT_SHIFT(idx) (idx)
#	endif
		bool ProcessKinect(KinectEvent* a_event)
		{
			return REL::RelocateVirtual<decltype(&MenuEventHandler::ProcessKinect)>(AE1799_SLOT_SHIFT(0x02), 0x05, this, a_event);
		}
		bool ProcessThumbstick(ThumbstickEvent* a_event)
		{
			return REL::RelocateVirtual<decltype(&MenuEventHandler::ProcessThumbstick)>(AE1799_SLOT_SHIFT(0x03), 0x06, this, a_event);
		}
		bool ProcessMouseMove(MouseMoveEvent* a_event)
		{
			return REL::RelocateVirtual<decltype(&MenuEventHandler::ProcessMouseMove)>(AE1799_SLOT_SHIFT(0x04), 0x07, this, a_event);
		}
		bool ProcessButton(ButtonEvent* a_event)
		{
			return REL::RelocateVirtual<decltype(&MenuEventHandler::ProcessButton)>(AE1799_SLOT_SHIFT(0x05), 0x08, this, a_event);
		}

#	ifdef ENABLE_SKYRIM_AE
		// AE 1.7.99-only slots. No-op below 1.7.99, where the slots do not exist.
		bool ProcessMotionGesture(MotionGestureEvent* a_event)
		{
			if (!REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99)) {
				return false;
			}
			return REL::RelocateVirtual<bool(MenuEventHandler*, MotionGestureEvent*)>(0x02, 0x02, this, a_event);
		}
		bool ProcessSixaxis(SixaxisEvent* a_event)
		{
			if (!REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99)) {
				return false;
			}
			return REL::RelocateVirtual<bool(MenuEventHandler*, SixaxisEvent*)>(0x03, 0x03, this, a_event);
		}
#	endif
#	undef AE1799_SLOT_SHIFT

		// VR-only slots (no flat counterpart to relocate to): no-op on flat, dispatch to the real
		// VR vtable slot only when actually running as VR. The repeated index passed as the SE/AE
		// slot is a dead placeholder, not a real flat-layout value -- RelocateVirtual is unreachable
		// on flat because of the IsVR() guard above it.
		bool ProcessVrWandTouchpadSwipe(VrWandTouchpadSwipeEvent* a_event)
		{
			if SKYRIM_REL_VR_CONSTEXPR (!REL::Module::IsVR()) {
				return false;
			} else {
				return REL::RelocateVirtual<bool(MenuEventHandler*, VrWandTouchpadSwipeEvent*)>(0x02, 0x02, this, a_event);
			}
		}
		bool ProcessVrWandTouchpadPosition(VrWandTouchpadPositionEvent* a_event)
		{
			if SKYRIM_REL_VR_CONSTEXPR (!REL::Module::IsVR()) {
				return false;
			} else {
				return REL::RelocateVirtual<bool(MenuEventHandler*, VrWandTouchpadPositionEvent*)>(0x03, 0x03, this, a_event);
			}
		}
		bool Unk_04(void* a_event)
		{
			if SKYRIM_REL_VR_CONSTEXPR (!REL::Module::IsVR()) {
				return false;
			} else {
				return REL::RelocateVirtual<bool(MenuEventHandler*, void*)>(0x04, 0x04, this, a_event);
			}
		}
#endif

		// members
		bool          registered;  // 0C
		std::uint8_t  unk0D;       // 0D
		std::uint16_t pad0E;       // 0E
	};
	static_assert(sizeof(MenuEventHandler) == 0x10);
}
