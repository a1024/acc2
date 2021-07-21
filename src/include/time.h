#pragma once
#ifndef TIME_H
#define TIME_H

struct tm
{
	int tm_sec;		/* seconds after the minute - [0,59] */
	int tm_min;		/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;		/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
};
#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif

typedef __int64 __time64_t;
typedef __time64_t time_t;


#ifdef __cplusplus
extern "C"
{
#endif

errno_t			__cdecl _localtime64_s(struct tm *_Tm, const __time64_t *_Time);
__time64_t		__cdecl _time64(__time64_t *_Time);
__time64_t		__cdecl _mktime64(struct tm * _Tm);

#ifdef __cplusplus
}
#endif


static __inline time_t __cdecl time(time_t *_Time)
{
    return _time64(_Time);
}
static __inline errno_t __cdecl localtime_s(struct tm * _Tm, const time_t * _Time)
{
    return _localtime64_s(_Tm, _Time);
}
static __inline time_t __cdecl mktime(struct tm * _Tm)
{
    return _mktime64(_Tm);
}

#endif//TIME_H