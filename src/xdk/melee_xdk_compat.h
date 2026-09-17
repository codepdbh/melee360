#ifndef MELEE360_XDK_LBTIME_COMPAT_H
#define MELEE360_XDK_LBTIME_COMPAT_H

/* Keep lbtime.c untouched while replacing its Dolphin OS dependency. */
#define MELEE_LB_LBTIME_H

typedef unsigned int u32;
typedef unsigned __int64 u64;
typedef __int64 OSTime;

typedef struct OSCalendarTime {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int msec;
    int usec;
} OSCalendarTime;

#define U8_MAX 255u

#ifdef __cplusplus
extern "C" {
#endif

OSTime M360_OSGetTime(void);
void M360_OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* calendar);

#ifdef __cplusplus
}
#endif

#define OSGetTime M360_OSGetTime
#define OSTicksToSeconds(ticks) ((u64) (ticks) / 1000u)
#define OSSecondsToTicks(seconds) ((OSTime) ((u64) (seconds) * 1000u))
#define OSTicksToCalendarTime(ticks, calendar) \
    M360_OSTicksToCalendarTime((ticks), (calendar))

#endif
