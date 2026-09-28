//=============================================================================
// pddipsp.hpp
//
// PSP-платформенный заголовок PDDI. Скопирован с pddilinux.hpp —
// он максимально generic, без платформенной специфики.
//=============================================================================

#ifndef _PDDIPSP_HPP
#define _PDDIPSP_HPP

#include <pddi/pddienum.hpp>

// machine dependent types
typedef long long      PDDI_S64;
typedef long long      PDDI_U64;
typedef int            PDDI_S32;
typedef unsigned       PDDI_U32;
typedef char           PDDI_S8;
typedef unsigned char  PDDI_U8;
typedef short          PDDI_S16;
typedef unsigned short PDDI_U16;

class pddiDevice;

// prototype for the initialization function through implicit linking
extern "C" int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev);

// prototype for the initialization function through LoadLibrary
typedef int (*PDDICREATEPROC)(int, int, pddiDevice**);

// PSP version of PDDI uses fully virtualized interfaces
#define PDDI_INTERFACE virtual
#define PDDI_PURE = 0
#define PDDI_VIRTUAL

class pddiDisplayInit
{
public:
    pddiDisplayInit()
    {
        xsize = 480;
        ysize = 272;
        bpp = 32;
    }

    int xsize;         // x resolution (fullscreen only)
    int ysize;         // y resolution (fullscreen only)
    int bpp;           // bits per pixel
};

#include <pddi/pddipc.hpp>

#endif /* _PDDIPSP_HPP */
