/*******************************************************************************
	File:		USocket.h
 
	Contains: The base utility for socket header file
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/13/16		liangliang			Create file
 
 *******************************************************************************/

#ifndef __USocket_H__
#define __USocket_H__

#include <errno.h>
#include <stdio.h>
#include <netdb.h>


#if __QC_OS_WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT       WSAETIMEDOUT
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED    WSAECONNREFUSED
#endif
#ifndef EINPROGRESS
#define EINPROGRESS     WSAEINPROGRESS
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#endif

#define getsockopt(a, b, c, d, e) getsockopt(a, b, c, (char*) d, e)
#define setsockopt(a, b, c, d, e) setsockopt(a, b, c, (const char*) d, e)

#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int qcSockErrno();

int qcSockSetNonblock (int fd, int enable);
int qcSockSelect (int fd, int write, int read, int timeout);

int qcSockSocket (int pf, int type, int proto);
int qcSockConnect (int fd, const struct sockaddr * addr, socklen_t addrlen);
void qcSockClose (int fd);

int qcSockRead (int fd, unsigned char * pBuff, int nSize);
int qcSockWrite (int fd, unsigned char * pBuff, int nSize);
    
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif /* __USocket_H__ */
