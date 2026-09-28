//=============================================================================
// plat_filemap.cpp — PSP: читаем файл через fopen/fread в heap-буфер.
//=============================================================================

#include <p3d/platform/psp/plat_filemap.hpp>
#include <p3d/error.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

tPspFileMap::tPspFileMap(const char* filename)
    : fh(NULL)
{
    length = 0;
    Open(filename);
    SetFilename(filename);
}

tPspFileMap::~tPspFileMap()
{
    Close();
}

void tPspFileMap::Open(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp)
        return;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    if (len <= 0)
    {
        fclose(fp);
        return;
    }
    fseek(fp, 0, SEEK_SET);

    unsigned char* memory = (unsigned char*)malloc((size_t)len);
    if (!memory)
    {
        fclose(fp);
        return;
    }

    size_t rd = fread(memory, 1, (size_t)len, fp);
    fclose(fp);
    if (rd != (size_t)len)
    {
        free(memory);
        return;
    }

    if (dataStream)
        dataStream->Release();
    dataStream = new radLoadDataStream(memory, (unsigned int)len, free);
}

void tPspFileMap::Close()
{
    // Буфер освобождается автоматически, когда tFileMem его отпускает.
    fh = NULL;
}
