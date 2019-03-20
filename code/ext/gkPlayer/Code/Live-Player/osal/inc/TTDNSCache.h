/**
* File : TTDNSCache.h 
* Description : TTDNSCache定义文件
*/
#ifndef __TT_DNSCACHE_H__
#define __TT_DNSCACHE_H__

// INCLUDES
#include "GKTypedef.h"
#include "GKMacrodef.h"

#define IP_NOT_FOUND 0

typedef struct DNSNode
{
    TTChar*    hostName;
    TTPtr      ipAddress; //适配IPv4 & IPv6地址，指向struct sockaddr
    DNSNode* next;
    DNSNode()
    {
        hostName = NULL;
        ipAddress = 0;
        next = NULL;
    }
} DNSNode, *PDNSNode;

// CLASSES DECLEARATION
class CTTDNSCache
{
public:

	CTTDNSCache();

	~CTTDNSCache();

	/**
	* \fn                       void put(TTChar* hostName, TTPtr ipAddress, TTUint ipAddressSize);
	* \brief                    保存[域名,ip地址]到cache中去，内部会复制一份TTPtr指向内容
	* \param[in]	hostName	域名	
	* \param[in]	ipAddress	ip地址指针
	* \param[in]	ipAddressSize	ip地址大小
	*/
	void put(TTChar* hostName, TTPtr ipAddress, TTUint ipAddressSize);

	/**
	* \fn                       void del(TTChar* hostName);
	* \brief                    在cache中根据域名删除ip地址
	* \param[in]	hostName	域名	
	*/
	void del(TTChar* hostName);

	/**
	* \fn                       TTPtr get(TTChar*  hostName);
	* \brief                    在cache中根据域名查找ip地址，TTPtr由内部维护释放
	* \param[in]	hostName	域名	
	* \return					ip地址指针，指向struct addrinfo
	*/
	TTPtr get(TTChar*  hostName);

private:
	DNSNode*  iDNSList;
};


#endif
