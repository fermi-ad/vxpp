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

char* vwpp::VME::calcBaseAddr(vwpp::VME::AddressSpace const tag,
			      uint32_t const base)
{
    char* addr;

    if (ERROR != sysBusToLocalAdrs(tag, reinterpret_cast<char*>(base), &addr))
	return addr;
    throw std::runtime_error("cannot localize address");
}
