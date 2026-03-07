#include "./date.h"

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
