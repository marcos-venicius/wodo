#define CL_ARRAY_IMPLEMENTATION
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "visualizer.h"
#include "date.h"

void print_wodo_datetime(wodo_datetime_t timezoned_datetime, bool simple)
{
    wodo_datetime_t datetime = convert_to_local(timezoned_datetime);

    if (simple) {
        printf("%04d-%02d-%02d %02d:%02d",
               datetime.year,
               datetime.month,
               datetime.day,
               datetime.hour,
               datetime.minute);
    } else {
        printf("%04d-%02d-%02d %02d:%02d:%02d",
               datetime.year,
               datetime.month,
               datetime.day,
               datetime.hour,
               datetime.minute,
               datetime.second);

        if (datetime.tz_offset == 0) {
            printf("+00:00");
        } else {
            int offset = datetime.tz_offset;
            char sign = '+';

            if (offset < 0) {
                sign = '-';
                offset = -offset;
            }

            int hours = offset / 60;
            int minutes = offset % 60;

            printf("%c%02d:%02d", sign, hours, minutes);
        }
    }
}
