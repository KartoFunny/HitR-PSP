//=============================================================================
// plat_filemap.hpp — PSP: файловый ввод через newlib stdio.
//=============================================================================

#ifndef _PLAT_FILEMAP_HPP
#define _PLAT_FILEMAP_HPP

#include <p3d/file.hpp>

class tPspFileMap : public tFileMem
{
public:
    tPspFileMap(const char* filename);

    bool IsOpen(void) { return GetMemory() != NULL; }

protected:
    virtual ~tPspFileMap();

    void Open(const char* filename);
    void Close(void);

    void* fh;
};

#endif /*_PLAT_FILEMAP_HPP*/
