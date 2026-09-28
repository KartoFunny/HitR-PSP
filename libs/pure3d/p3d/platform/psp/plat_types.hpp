//=============================================================================
// plat_types.hpp — PSP-версия платформенных типов для Pure3D.
//=============================================================================

#ifndef _PLAT_TYPES_HPP
#define _PLAT_TYPES_HPP

#ifndef RAD_PSP
#error PSP platform header included in non-PSP build
#endif

typedef long long          P3D_S64;
typedef unsigned long long P3D_U64;
typedef int                P3D_S32;
typedef unsigned           P3D_U32;
typedef short              P3D_S16;
typedef unsigned short     P3D_U16;
typedef signed char        P3D_S8;
typedef unsigned char      P3D_U8;
typedef unsigned short       P3D_UNICODE;

#ifndef strcmpi
#define strcmpi strcasecmp
#endif

#endif
