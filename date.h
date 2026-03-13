#ifndef _WODO_DATE_H_
#define _WODO_DATE_H_

#include <stdbool.h>
#include "./systemtypes.h"

bool validate_datetime(wodo_datetime_t datetime);
wodo_datetime_t convert_to_local(wodo_datetime_t datetime);

#endif // !_WODO_DATE_H_
