#include <vxWorks.h>
#include <sysLib.h>
#include <algorithm>
#include "./vwpp.h"

using namespace vwpp::v2_7;

int ms_to_tick(int const v)
{
    if (v == WAIT_FOREVER)
	return WAIT_FOREVER;
    else
	return (std::max(v, 0) * ::sysClkRateGet() + 999) / 1000;
}

uint8_t* VME::calcBaseAddr(VME::AddressSpace const tag, uint32_t const base)
{
    char* addr;

    if (ERROR != sysBusToLocalAdrs(tag, reinterpret_cast<char*>(base),
				   &addr))
	return reinterpret_cast<uint8_t*>(addr);
    throw std::runtime_error("cannot localize address");
}
