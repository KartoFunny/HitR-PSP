//=============================================================================
// pddipsp.cpp — точка входа PDDI для PSP
// Использует pguDevice из pddi/gu/
//=============================================================================
#include <pddi/pddi.hpp>
#include <pddi/pddipsp.hpp>

#include <pddi/gu/gudev.hpp>

extern "C" int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev)
{
    if (versionMajor != PDDI_VERSION_MAJOR || versionMinor != PDDI_VERSION_MINOR)
        return PDDI_VERSION_ERROR;
    if (!dev) return PDDI_ERR;
    *dev = new pguDevice();
    return PDDI_OK;
}
