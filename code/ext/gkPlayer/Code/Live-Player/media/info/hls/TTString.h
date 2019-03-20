#ifndef _TT_STRING_H_
#define _TT_STRING_H_

#include"GKTypedef.h"

struct TTString {
    TTString();
    TTString(const char *s);
    TTString(const char *s, TTUint32 size);
    TTString(const TTString &from);
    TTString(const TTString &from, TTUint32 offset, TTUint32 n);
    ~TTString();

    TTString &operator=(const TTString &from);
    void setTo(const char *s);
    void setTo(const char *s, TTUint32 size);
    void setTo(const TTString &from, TTUint32 offset, TTUint32 n);

    TTUint32 size() const;
    const char *c_str() const;

    bool empty() const;

    void clear();
    void trim();
    void erase(TTUint32 start, TTUint32 n);

    void append(char c) { append(&c, 1); }
    void append(const char *s);
    void append(const char *s, TTUint32 size);
    void append(const TTString &from);
    void append(const TTString &from, TTUint32 offset, TTUint32 n);
    void append(int x);
    void append(unsigned x);
    void append(long x);
    void append(unsigned long x);
    void append(long long x);
    void append(unsigned long long x);
    void append(float x);
    void append(double x);
    void append(void *x);

    void insert(const TTString &from, TTUint32 insertionPos);
    void insert(const char *from, TTUint32 size, TTUint32 insertionPos);

    TTInt32 find(const char *substring, TTUint32 start = 0) const;

    TTUint32 hash() const;

    bool operator==(const TTString &other) const;
    bool operator<(const TTString &other) const;
    bool operator>(const TTString &other) const;

    int compare(const TTString &other) const;

    bool startsWith(const char *prefix) const;

    void tolower();

private:
    static const char *kEmptyString;

    char *mData;
    TTUint32 mSize;
    TTUint32 mAllocSize;

    void makeMutable();
};

#endif  // _TT_STRING_H_

	