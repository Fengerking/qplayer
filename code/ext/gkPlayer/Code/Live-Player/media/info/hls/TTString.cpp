#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include "TTString.h"

const char *TTString::kEmptyString = "";

TTString::TTString()
    : mData((char *)kEmptyString),
      mSize(0),
      mAllocSize(1) {
}

TTString::TTString(const char *s)
    : mData(NULL),
      mSize(0),
      mAllocSize(1) {
    setTo(s);
}

TTString::TTString(const char *s, TTUint32 size)
    : mData(NULL),
      mSize(0),
      mAllocSize(1) {
    setTo(s, size);
}

TTString::TTString(const TTString &from)
    : mData(NULL),
      mSize(0),
      mAllocSize(1) {
    setTo(from, 0, from.size());
}

TTString::TTString(const TTString &from, TTUint32 offset, TTUint32 n)
    : mData(NULL),
      mSize(0),
      mAllocSize(1) {
    setTo(from, offset, n);
}

TTString::~TTString() {
    clear();
}

TTString &TTString::operator=(const TTString &from) {
    if (&from != this) {
        setTo(from, 0, from.size());
    }

    return *this;
}

TTUint32 TTString::size() const {
    return mSize;
}

const char *TTString::c_str() const {
    return mData;
}

bool TTString::empty() const {
    return mSize == 0;
}

void TTString::setTo(const char *s) {
    setTo(s, strlen(s));
}

void TTString::setTo(const char *s, TTUint32 size) {
    clear();
    append(s, size);
}

void TTString::setTo(const TTString &from, TTUint32 offset, TTUint32 n) {
    //assert(&from != this);

    clear();
    setTo(from.mData + offset, n);
}

void TTString::clear() {
    if (mData && mData != kEmptyString) {
        free(mData);
        mData = NULL;
    }

    mData = (char *)kEmptyString;
    mSize = 0;
    mAllocSize = 1;
}

TTUint32 TTString::hash() const {
    size_t x = 0;
    for (size_t i = 0; i < mSize; ++i) {
        x = (x * 31) + mData[i];
    }

    return x;
}

bool TTString::operator==(const TTString &other) const {
    return mSize == other.mSize && !memcmp(mData, other.mData, mSize);
}

void TTString::trim() {
    makeMutable();

    size_t i = 0;
    while (i < mSize && isspace(mData[i])) {
        ++i;
    }

    size_t j = mSize;
    while (j > i && isspace(mData[j - 1])) {
        --j;
    }

    memmove(mData, &mData[i], j - i);
    mSize = j - i;
    mData[mSize] = '\0';
}

void TTString::erase(TTUint32 start, TTUint32 n) {
    //assert(start, mSize);
    //assert(start + n, mSize);

    makeMutable();

    memmove(&mData[start], &mData[start + n], mSize - start - n);
    mSize -= n;
    mData[mSize] = '\0';
}

void TTString::makeMutable() {
    if (mData == kEmptyString) {
        mData = strdup(kEmptyString);
    }
}

void TTString::append(const char *s) {
    append(s, strlen(s));
}

void TTString::append(const char *s, TTUint32 size) {
    makeMutable();

    if (mSize + size + 1 > mAllocSize) {
        mAllocSize = (mAllocSize + size + 31) & -32;
		char *aData = NULL;
		if(mData == NULL) {
			aData = (char *)malloc(mAllocSize);
		} else {
			aData = (char *)realloc(mData, mAllocSize);
		}

		if(aData != NULL) {
			mData = aData;
		}else {
			return;
		}
        //assert(mData != NULL);
    }

    memcpy(&mData[mSize], s, size);
    mSize += size;
    mData[mSize] = '\0';
}

void TTString::append(const TTString &from) {
    append(from.c_str(), from.size());
}

void TTString::append(const TTString &from, TTUint32 offset, TTUint32 n) {
    append(from.c_str() + offset, n);
}

void TTString::append(int x) {
    char s[16];
    sprintf(s, "%d", x);

    append(s);
}

void TTString::append(unsigned x) {
    char s[16];
    sprintf(s, "%u", x);

    append(s);
}

void TTString::append(long x) {
    char s[16];
    sprintf(s, "%ld", x);

    append(s);
}

void TTString::append(unsigned long x) {
    char s[16];
    sprintf(s, "%lu", x);

    append(s);
}

void TTString::append(long long x) {
    char s[32];
    sprintf(s, "%lld", x);

    append(s);
}

void TTString::append(unsigned long long x) {
    char s[32];
    sprintf(s, "%llu", x);

    append(s);
}

void TTString::append(float x) {
    char s[16];
    sprintf(s, "%f", x);

    append(s);
}

void TTString::append(double x) {
    char s[16];
    sprintf(s, "%f", x);

    append(s);
}

void TTString::append(void *x) {
    char s[16];
    sprintf(s, "%p", x);

    append(s);
}

TTInt32 TTString::find(const char *substring, TTUint32 start) const {
    //assert(start<size());

    const char *match = strstr(mData + start, substring);

    if (match == NULL) {
        return -1;
    }

    return match - mData;
}

void TTString::insert(const TTString &from, TTUint32 insertionPos) {
    insert(from.c_str(), from.size(), insertionPos);
}

void TTString::insert(const char *from, TTUint32 size, TTUint32 insertionPos) {
    //assert(insertionPos, 0u);
    //assert(insertionPos, mSize);

    makeMutable();

    if (mSize + size + 1 > mAllocSize) {
        mAllocSize = (mAllocSize + size + 31) & -32;
		char *aData = NULL;
		if(mData == NULL) {
			aData = (char *)malloc(mAllocSize);
		} else {
			aData = (char *)realloc(mData, mAllocSize);
		}

		if(aData != NULL) {
			mData = aData;
		}else {
			return;
		}
        //assert(mData != NULL);
    }

    memmove(&mData[insertionPos + size],
            &mData[insertionPos], mSize - insertionPos + 1);

    memcpy(&mData[insertionPos], from, size);

    mSize += size;
}

bool TTString::operator<(const TTString &other) const {
    return compare(other) < 0;
}

bool TTString::operator>(const TTString &other) const {
    return compare(other) > 0;
}

int TTString::compare(const TTString &other) const {
    return strcmp(mData, other.mData);
}

void TTString::tolower() {
    makeMutable();

    for (size_t i = 0; i < mSize; ++i) {
        mData[i] = ::tolower(mData[i]);
    }
}

bool TTString::startsWith(const char *prefix) const {
    return !strncmp(mData, prefix, strlen(prefix));
}

