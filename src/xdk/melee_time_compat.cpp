#include <xtl.h>

#include "melee_xdk_compat.h"

extern "C" OSTime M360_OSGetTime(void)
{
    return static_cast<OSTime>(GetTickCount());
}

extern "C" void M360_OSTicksToCalendarTime(
    OSTime ticks, OSCalendarTime* calendar)
{
    if (!calendar)
        return;

    ZeroMemory(calendar, sizeof(*calendar));
    const u64 seconds = static_cast<u64>(ticks / 1000);
    calendar->sec = static_cast<int>(seconds % 60);
    calendar->min = static_cast<int>((seconds / 60) % 60);
    calendar->hour = static_cast<int>((seconds / 3600) % 24);
}
