#include <stdio.h>
#include "FileWrite.h"
FileWrite::FileWrite()
:fp(NULL)
{

}

FileWrite::~FileWrite()
{
    if(fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
}

int FileWrite::create(const char *fileName)
{
    fp = fopen(fileName, "wb");
    if(fp == NULL)
        return 1;
    else
        return 0;
}

int FileWrite::write(const void *buf, int len)
{
    if(fp == NULL)
        return 0;
    
    return fwrite(buf, 1, len, fp);
    
}

void FileWrite::flush()
{
    if(fp == NULL)
        return;
    
    fflush(fp);
    
}
void FileWrite::close()
{
    if(fp == NULL)
        return;
    
    fclose(fp);
}
