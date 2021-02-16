#include <vxWorks.h>
#include <sysLib.h>
#include <algorithm>
#include "./vwpp.h"

int vwpp::v3_0::ms_to_tick(int const v)
{
    if (v == (int) WAIT_FOREVER)
	return (int) WAIT_FOREVER;
    else
	return (std::max(v, 0) * ::sysClkRateGet() + 999) / 1000;
}

uint8_t* vwpp::v3_0::VME::calcBaseAddr(VME::AddressSpace const tag, uint32_t const base)
{
    char* addr;

    if (ERROR != sysBusToLocalAdrs(tag, reinterpret_cast<char*>(base),
				   &addr))
	return reinterpret_cast<uint8_t*>(addr);
    throw std::runtime_error("cannot localize address");
}
