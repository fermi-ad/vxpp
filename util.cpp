// $Id: util.cpp,v 1.1 2008/12/19 21:17:42 neswold Exp $

#include <vxWorks.h>
#include <sysLib.h>
#include <algorithm>
#include "vwpp.h"

int vwpp::ms_to_tick(int const v)
{
    if (v == WAIT_FOREVER)
	return WAIT_FOREVER;
    else {
	int const rate = sysClkRateGet();

	return std::max((rate * v + 500) / 1000, 1);
    }
}
