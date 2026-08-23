#include "RE/M/MenuEventHandler.h"

// Every non-VR-exclusive build defines these as non-virtual RelocateVirtual wrappers
// inline in the header (the VR vtable and AE 1.7.99 shift these slots), so they must
// not be redefined here.
#ifdef EXCLUSIVE_SKYRIM_VR
namespace RE
{
	bool MenuEventHandler::ProcessKinect(KinectEvent*)
	{
		return false;
	}

	bool MenuEventHandler::ProcessThumbstick(ThumbstickEvent*)
	{
		return false;
	}

	bool MenuEventHandler::ProcessMouseMove(MouseMoveEvent*)
	{
		return false;
	}

	bool MenuEventHandler::ProcessButton(RE::ButtonEvent*)
	{
		return false;
	}
}
#endif
