#include <vxWorks.h>
#include <sysLib.h>
#include <algorithm>
#include "vwpp.h"

int vwpp::ms_to_tick(int const v)
{
    if (v == WAIT_FOREVER)
	return WAIT_FOREVER;
    else
	return (std::max(v, 0) * ::sysClkRateGet() + 999) / 1000;
}
