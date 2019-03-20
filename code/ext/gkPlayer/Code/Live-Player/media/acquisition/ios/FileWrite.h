#ifndef FileWrite_h
#define FileWrite_h

class FileWrite
{
public:
    FileWrite();
    ~FileWrite();
    
    int create(const char *fileName);
    int write(const void *buf, int len);
    void flush();
    void close();
    
private:
    FILE *fp;
};

#endif /* FileWrite_h */
