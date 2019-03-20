/**
* File : TTDNSCache.cpp
* Description : TTDNSCache 实现文件
*/

// INCLUDES
#include <pthread.h>
#include "TTDNSCache.h"
#include "string.h"

CTTDNSCache::CTTDNSCache()
: iDNSList(NULL)
{
}

CTTDNSCache::~CTTDNSCache()
{
	if (iDNSList != NULL)
	{
		PDNSNode pNext = iDNSList;
		PDNSNode pCurr;
		while (pNext)
		{
			pCurr = pNext;
			pNext = pNext->next;
			delete[] pCurr->hostName;
			delete pCurr;
		}
		iDNSList = NULL;
	}
}

TTPtr CTTDNSCache::get(TTChar* hostName)
{
	TTPtr   nret = IP_NOT_FOUND;
	PDNSNode ptr = iDNSList;

	while (ptr)
	{
		if (ptr->hostName && strcmp(ptr->hostName, hostName) == 0)
		{
			nret = ptr->ipAddress;
			break;
		}
		else
			ptr = ptr->next;
	}

	return nret;
}

void CTTDNSCache::put(TTChar* hostName, TTPtr ipAddress, TTUint ipAddressSize)
{
	PDNSNode ptr = iDNSList;
	PDNSNode ptrPrev = iDNSList;

	if (hostName == NULL || strlen(hostName) == 0)
		return ;

	while (ptr != NULL)
	{
		ptrPrev = ptr;
		ptr = ptr->next;
	}

	if(iDNSList == ptr)
	{
		iDNSList = new DNSNode;
		ptr = iDNSList;
	}
	else
	{
		ptrPrev->next = new DNSNode;
		ptr = ptrPrev->next;
	}

	ptr->hostName = new TTChar[strlen(hostName) + 1];
	strcpy(ptr->hostName, hostName);
    ptr->ipAddress = new TTChar[ipAddressSize];
    memcpy(ptr->ipAddress, ipAddress, ipAddressSize);
}

void CTTDNSCache::del(TTChar* hostName)
{
	PDNSNode ptr = iDNSList;
	PDNSNode ptrPrev = ptr;

	if (hostName == NULL || strlen(hostName) == 0 ||  ptr == NULL)
	{
		return ;
	}

	while (ptr != NULL)
	{ 
		if (ptr->hostName && strcmp(ptr->hostName, hostName) == 0)
		{
			//handle the first item
			if (ptr == iDNSList)
				iDNSList = ptr->next;
			else
				ptrPrev->next = ptr->next;

			delete[] ptr->hostName;
            delete[] ptr->ipAddress;
			delete ptr;
			break;
		}

		ptrPrev = ptr;
		ptr = ptr->next;
	}
}


//end of file
