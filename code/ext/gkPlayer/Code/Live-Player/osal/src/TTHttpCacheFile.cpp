#include "TTHttpCacheFile.h"

CTTHttpCacheFile::CTTHttpCacheFile()
: iFileHandle(NULL)
, iCachedSize(0)
, iTotalSize(0)
, iIndexEnd(0)
, iCurIndex(0)
, iBufferUnit(NULL)
, iBufferPool(NULL)
, iWriteBuffer(NULL)
, iWriteCount(0)
, iWriteIndex(0)
{	
	iCritical.Create();
    
    iBufferPool= new TTBufferUnit;
    iBufferPool->iPosition = 0;
    iBufferPool->iFlag = 0;
    iBufferPool->iSize = 0;
    iBufferPool->iBuffer = (TTUint8*)malloc(BUFFER_UNIT_SIZE);
    if(iBufferPool->iBuffer){
        iBufferPool->iTotalSize = BUFFER_UNIT_SIZE;
    } else {
        iBufferPool->iTotalSize = 0;
    }
}

TTInt CTTHttpCacheFile::Open(const TTChar* aUrl)
{
	iFileHandle = fopen(aUrl, "rb+");
	TTInt nLen = 0;
	if ((iFileHandle != NULL) && (TTKErrNone == fseek(iFileHandle, 0, SEEK_END)) && ((nLen = ftell(iFileHandle)) > 0))
	{	 
		iCachedSize = nLen;
		ResetBufferUnit();
        iBufferPool->iPosition = iCachedSize;
		return TTKErrNone;
	}
	
    return TTKErrAccessDenied;
}

TTInt CTTHttpCacheFile::Create(const TTChar* aUrl, TTInt64 aTotalSize)
{
	GKASSERT(aTotalSize > 0);
	iCritical.Lock();
	iFileHandle = fopen(aUrl, "wb+");
	if (iFileHandle == NULL)
	{
		iCritical.UnLock();
		return TTKErrPathNotFound;
	}

	iCachedSize = 0;
	iTotalSize = aTotalSize;

    ResetBufferUnit();
	/*UnInitBufferUnit();
	if(aTotalSize < BUFFER_UNIT_MAXSIZE) {
		TTInt nErr = InitBufferUnit(aTotalSize);
		if(nErr) {
			UnInitBufferUnit();
		}
	}*/
	iCritical.UnLock();
	return TTKErrNone;
}

CTTHttpCacheFile::~CTTHttpCacheFile()
{
    if(iBufferPool) {
        SAFE_FREE(iBufferPool->iBuffer);
        SAFE_DELETE(iBufferPool);
    }
	Close();
	iCritical.Destroy();
}

TTInt CTTHttpCacheFile::Read(void* aBuffer, TTInt aReadPos, TTInt aReadSize)
{
	GKASSERT((aBuffer != NULL) && (iFileHandle != 0));
	/*if(iBufferMode) {
		return ReadBuffer((TTUint8*)aBuffer, aReadPos, aReadSize);
	}*/

    if (aReadPos < 0) {
        return 0;
    }
	iCritical.Lock();
    
	TTInt nReadSize = 0;
    TTInt nSize = 0;
    if (aReadPos < iBufferPool->iPosition)
    {
        if (aReadPos + aReadSize <= iBufferPool->iPosition) {
            if (0 == fseek(iFileHandle, aReadPos, SEEK_SET))
                nReadSize = fread(aBuffer, 1, aReadSize, iFileHandle);
        }
        else{
            if (0 == fseek(iFileHandle, aReadPos, SEEK_SET))
                nReadSize = fread(aBuffer, 1, iBufferPool->iPosition - aReadPos, iFileHandle);
            
            nSize = aReadSize - nReadSize;
            
            if(nSize <= iBufferPool->iSize){
                memcpy(((unsigned char*)aBuffer + nReadSize), iBufferPool->iBuffer, nSize);
                nReadSize += nSize;
                
            }
            else{
                if (iBufferPool->iSize > 0) {
                    memcpy(((unsigned char*)aBuffer + nReadSize), iBufferPool->iBuffer, iBufferPool->iSize);
                    nReadSize += iBufferPool->iSize;
                }
            }
        }
    }
    else{
        if (aReadPos < iBufferPool->iPosition  + iBufferPool->iSize) {
            if (aReadPos + aReadSize <=  iBufferPool->iPosition  + iBufferPool->iSize) {
                nReadSize = aReadSize;
                memcpy(aBuffer, iBufferPool->iBuffer + (aReadPos-iBufferPool->iPosition), nReadSize);
            }
            else{
                nReadSize = (iBufferPool->iPosition + iBufferPool->iSize) - aReadPos;
                memcpy(aBuffer, iBufferPool->iBuffer + (aReadPos-iBufferPool->iPosition), nReadSize);
            }
        }
    }

    iCritical.UnLock();
	return nReadSize;
}

TTInt CTTHttpCacheFile::Write(void* aBuffer, TTInt aWriteSize)
{
    GKASSERT((aBuffer != NULL) && (iFileHandle != 0) && (aWriteSize + iCachedSize <= iTotalSize));
	TTInt nWriteSize = 0;
    TTInt nSize = 0;
	/*if(iBufferMode) {
		return WriteBuffer((TTUint8*)aBuffer, aWriteSize);
	}*/

	iCritical.Lock();

    iCachedSize += aWriteSize;
    
    if (iBufferPool->iSize + aWriteSize <= iBufferPool->iTotalSize) {
        memcpy(iBufferPool->iBuffer + iBufferPool->iSize, aBuffer, aWriteSize);
        iBufferPool->iSize += aWriteSize;
        
        if (iBufferPool->iSize == iBufferPool->iTotalSize) {
            if (0 == fseek(iFileHandle, iBufferPool->iPosition, SEEK_SET))
            {
                nWriteSize = fwrite(iBufferPool->iBuffer, 1, iBufferPool->iTotalSize, iFileHandle);
                if (nWriteSize > 0)
                {
                    iBufferPool->iPosition += nWriteSize;
                }
                else{
                    aWriteSize = nWriteSize;
                }
                
                if (nWriteSize == iBufferPool->iTotalSize) {
                    iBufferPool->iSize = 0;
                }else{
                    aWriteSize -= 1;
                }
            }
        }
    }
    else{
        nSize = iBufferPool->iTotalSize - iBufferPool->iSize;
        memcpy(iBufferPool->iBuffer + iBufferPool->iSize, aBuffer, nSize);
        iBufferPool->iSize = iBufferPool->iTotalSize;
        
        if (0 == fseek(iFileHandle, iBufferPool->iPosition, SEEK_SET))
        {
            nWriteSize = fwrite(iBufferPool->iBuffer, 1, iBufferPool->iSize, iFileHandle);
            if (nWriteSize > 0)
            {
                iBufferPool->iPosition += nWriteSize;
            }
            else{
                aWriteSize = nWriteSize;
            }
            
            if (iBufferPool->iSize == nWriteSize) {
                memcpy(iBufferPool->iBuffer, (TTUint8*)aBuffer + nSize, aWriteSize - nSize);
                iBufferPool->iSize = aWriteSize - nSize;
            }else{
                aWriteSize -= 1;
            }
        }
    }
    
	iCritical.UnLock();

	return aWriteSize;
}

TTInt CTTHttpCacheFile::ReadBuffer(TTUint8* aBuffer, TTInt aReadPos, TTInt aReadSize)
{
	GKASSERT((aBuffer != NULL));
	TTInt nSize = 0;
	TTInt nIndex = 0;

	iCritical.Lock();
	TTInt nReadSize = 0;
	TTInt nReadPos = aReadPos;
	TTInt idx  = 0;

	for(idx = 0; idx <= iIndexEnd; idx++) {
		nIndex = idx; 
		if(aReadPos >= iBufferUnit[nIndex]->iPosition && aReadPos < iBufferUnit[nIndex]->iPosition + iBufferUnit[nIndex]->iSize) {
			nSize =  iBufferUnit[nIndex]->iPosition + iBufferUnit[nIndex]->iSize - aReadPos;
			if(nSize >= aReadSize) {
				nSize = aReadSize;
				memcpy(aBuffer, iBufferUnit[nIndex]->iBuffer + aReadPos - iBufferUnit[nIndex]->iPosition, nSize);
				nReadSize += nSize;
				iCurIndex = nIndex;
				break;
			} else {
				memcpy(aBuffer, iBufferUnit[nIndex]->iBuffer + aReadPos - iBufferUnit[nIndex]->iPosition, nSize);
				nReadSize += nSize;
				aBuffer += nSize;
				aReadSize -= nSize;
				aReadPos += nSize;
			}
		}
	}
	iCritical.UnLock();

	return nReadSize;
}	

TTInt CTTHttpCacheFile::WriteBuffer(TTUint8* aBuffer, TTInt aWriteSize)
{
	TTInt nWriteSize = 0;
	TTInt nIndex = 0;
	GKCAutoLock Lock(&iCritical);
	nIndex = iIndexEnd;	
	if(iBufferUnit[nIndex]->iSize == 0){
		iBufferUnit[nIndex]->iPosition = iCachedSize;
	}

	nWriteSize = iBufferUnit[nIndex]->iTotalSize - iBufferUnit[nIndex]->iSize;		
	if(nWriteSize >= aWriteSize){
		nWriteSize = aWriteSize;
		memcpy(iBufferUnit[nIndex]->iBuffer + iBufferUnit[nIndex]->iSize, aBuffer, nWriteSize);
		iBufferUnit[nIndex]->iSize += nWriteSize;
		iCachedSize += nWriteSize;
	} else {
		memcpy(iBufferUnit[nIndex]->iBuffer + iBufferUnit[nIndex]->iSize, aBuffer, nWriteSize);
		iBufferUnit[nIndex]->iSize += nWriteSize;
		iCachedSize += nWriteSize;

		iIndexEnd++;			
		nIndex = iIndexEnd;	
		iBufferUnit[nIndex]->iSize = 0;

		nWriteSize += WriteBuffer(aBuffer + nWriteSize, aWriteSize - nWriteSize);
	}		

	return nWriteSize;
}


TTInt CTTHttpCacheFile::WriteFile(TTInt aCount)
{
    if(iFileHandle == NULL)
		return TTKErrEof;
		
    int ret = TTKErrNone;
    iCritical.Lock();
    if (aCount == 0)
    {
        if(iBufferPool->iSize == 0){
            iCritical.UnLock();
            return TTKErrEof;
        }
        
        iCachedSize += iBufferPool->iSize;
        if (0 == fseek(iFileHandle, iBufferPool->iPosition, SEEK_SET))
        {
            int nWriteSize = fwrite(iBufferPool->iBuffer, 1, iBufferPool->iSize, iFileHandle);
            if (nWriteSize > 0)
            {
                iBufferPool->iPosition += nWriteSize;
            }
            iBufferPool->iSize = 0;
        }
        ret = TTKErrEof;
    }
    iCritical.UnLock();
	/*if(iBufferMode == 0 || iBufferUnit == NULL || iFileHandle == NULL)
		return TTKErrEof;

	TTInt nIndexEnd = 0;
	TTInt nWriteSize = 0;
	iCritical.Lock();
	if(iWriteIndex >= iIndexEnd && iCachedSize < iTotalSize) {
		iCritical.UnLock();
		return TTKErrNotReady;
	}

	if(aCount > 0) {
		if(iIndexEnd - iCurIndex < aCount) {
			iCritical.UnLock();
			return TTKErrNotReady;
		}
	}
	nIndexEnd = iIndexEnd;
	iCritical.UnLock();

	if(iWriteIndex > nIndexEnd) {
		if(iCachedSize == iTotalSize) {
			return TTKErrEof;
		} else {
			return TTKErrDiskFull;
		}
	}

	iCritical.Lock();
	if(iBufferUnit[iWriteIndex] == NULL && iBufferUnit[iWriteIndex]->iBuffer == NULL) {
		iCritical.UnLock();
		return TTKErrNotReady;
	}
	nWriteSize = iBufferUnit[iWriteIndex]->iSize;
	memcpy(iWriteBuffer, iBufferUnit[iWriteIndex]->iBuffer, nWriteSize);
	iCritical.UnLock();

	nWriteSize = fwrite(iWriteBuffer, 1, nWriteSize, iFileHandle);		
	if (nWriteSize > 0)
		iWriteCount += nWriteSize;

	if(iWriteIndex == nIndexEnd) {
		if(iWriteCount == iTotalSize) {
			iWriteIndex++;
			return TTKErrEof;
		} else {
			iWriteIndex++;
			return TTKErrDiskFull;
		}
	} else {
		iWriteIndex++;
	}*/

	return ret;
}

void CTTHttpCacheFile::Close()
{
	iCritical.Lock();
	if (iFileHandle != NULL)
	{
		fclose(iFileHandle);
		iFileHandle = NULL;
	}
	UnInitBufferUnit();
	iCritical.UnLock();
}

TTInt64 CTTHttpCacheFile::CachedSize()
{
	TTInt64 nCachedSize = 0;
	iCritical.Lock();
	nCachedSize = iCachedSize;
	iCritical.UnLock();
	return nCachedSize;
}

void CTTHttpCacheFile::ResetBufferUnit()
{
    if (iBufferPool) {
        iBufferPool->iPosition = 0;
        iBufferPool->iFlag = 0;
        iBufferPool->iSize = 0;
        if(iBufferPool->iBuffer)
            iBufferPool->iTotalSize = BUFFER_UNIT_SIZE;
        else
            iBufferPool->iTotalSize = 0;
    }
}

TTInt CTTHttpCacheFile::InitBufferUnit(int nSize)
{
	if(nSize >= BUFFER_UNIT_MAXSIZE)
		return TTKErrOverflow;
	
	iBufferCount = nSize/BUFFER_UNIT_SIZE + 1;

	iBufferUnit = new TTBufferUnit*[iBufferCount];

	TTInt n = 0;
	for(n = 0; n < iBufferCount; n++) {
		iBufferUnit[n] = new TTBufferUnit;

		iBufferUnit[n]->iPosition = 0;
		iBufferUnit[n]->iFlag = 0;
		iBufferUnit[n]->iSize = 0;

		iBufferUnit[n]->iBuffer = (TTUint8*)malloc(BUFFER_UNIT_SIZE);
		if(iBufferUnit[n]->iBuffer){
			iBufferUnit[n]->iTotalSize = BUFFER_UNIT_SIZE;
		} else {
			return TTKErrOverflow;
		}
	}

	iWriteBuffer = (TTUint8*)malloc(BUFFER_UNIT_SIZE);
	if(iWriteBuffer == NULL)
		return TTKErrOverflow;

	iBufferMode = 1;

	return TTKErrNone;
}

TTInt CTTHttpCacheFile::UnInitBufferUnit()
{
	TTInt n = 0;
	if(iBufferUnit != NULL) {
		for(n = 0; n < iBufferCount; n++) {
			if(iBufferUnit[n]) {
				SAFE_FREE(iBufferUnit[n]->iBuffer);
			}
			SAFE_FREE(iBufferUnit[n]);
		}
		SAFE_FREE(iBufferUnit);
	}

	SAFE_FREE(iWriteBuffer);

	iIndexEnd = 0;
	iBufferMode = 0;
	iBufferCount = 0;
	iWriteCount = 0;
	iWriteIndex = 0;
	return TTKErrNone;
}