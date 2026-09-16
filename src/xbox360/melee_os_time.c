#include <dolphin/os.h>

#include <ppc/timebase.h>
#include <string.h>

/*
 * Melee's time helpers use the GameCube OS tick rate (40.5 MHz).  Convert the
 * Xenon time base instead of exposing Xbox-specific timing to game code.
 */
OSTime OSGetTime(void)
{
    return (OSTime)((mftb() * (u64)OS_TIMER_CLOCK) / (u64)PPC_TIMEBASE_FREQ);
}

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime *td)
{
    static const int days_before_month[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    u64 seconds = (u64)OSTicksToSeconds(ticks);
    u64 days = seconds / 86400;
    int year = 2000;
    int leap;
    int month;

    memset(td, 0, sizeof(*td));
    td->sec = (int)(seconds % 60);
    td->min = (int)((seconds / 60) % 60);
    td->hour = (int)((seconds / 3600) % 24);
    td->wday = (int)((days + 6) % 7); /* 2000-01-01 was Saturday. */

    for (;;) {
        u64 year_days;
        leap = ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0));
        year_days = leap ? 366 : 365;
        if (days < year_days)
            break;
        days -= year_days;
        ++year;
    }

    td->year = year;
    td->yday = (int)days;
    for (month = 11; month > 0; --month) {
        int start = days_before_month[month] + (leap && month > 1 ? 1 : 0);
        if ((int)days >= start)
            break;
    }
    td->mon = month;
    td->mday = (int)days - days_before_month[month] -
               (leap && month > 1 ? 1 : 0) + 1;
}
