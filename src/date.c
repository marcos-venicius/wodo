#include <time.h>
#include "date.h"

static const int MAX_TIMEZONE_OFFSET = 14 * 60;

static int is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int y, int m) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    if (m == 2 && is_leap(y))
        return 29;

    return days[m-1];
}

bool validate_datetime(wodo_datetime_t datetime) {
    if (datetime.month < 1 || datetime.month > 12)
        return false;

    if (datetime.day < 1 || datetime.day > days_in_month(datetime.year, datetime.month))
        return false;

    if (datetime.hour < 0 || datetime.hour > 23)
        return false;

    if (datetime.minute < 0 || datetime.minute > 59)
        return false;

    if (datetime.second < 0 || datetime.second > 59)
        return false;

    /* ISO timezone limits are -14:00 to +14:00 */
    if (datetime.tz_offset < -MAX_TIMEZONE_OFFSET || datetime.tz_offset > MAX_TIMEZONE_OFFSET)
        return false;

    return true;
}

/* days from civil algorithm (Howard Hinnant) */
static long long days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d - 1;
    unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;
    return era * 146097LL + (long long)doe - 719468LL;
}

static time_t datetime_to_timestamp(wodo_datetime_t dt)
{
    long long days = days_from_civil(dt.year, dt.month, dt.day);

    long long seconds =
        days * 86400LL +
        dt.hour * 3600 +
        dt.minute * 60 +
        dt.second;

    /* remove source timezone offset */
    seconds -= dt.tz_offset * 60;

    return (time_t)seconds;
}

wodo_datetime_t convert_to_local(wodo_datetime_t dt)
{
    time_t t = datetime_to_timestamp(dt);

    struct tm local;
    localtime_r(&t, &local);

    struct tm gmt;
    gmtime_r(&t, &gmt);

    int tz_offset =
        (int)difftime(mktime(&local), mktime(&gmt)) / 60;

    return (wodo_datetime_t){
        .year = local.tm_year + 1900,
        .month = local.tm_mon + 1,
        .day = local.tm_mday,
        .hour = local.tm_hour,
        .minute = local.tm_min,
        .second = local.tm_sec,
        .tz_offset = tz_offset
    };
}
