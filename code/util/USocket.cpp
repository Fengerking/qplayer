/*******************************************************************************
	File:		qcSocketIO.cpp
 
	Contains: The base utility for socket implement file
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/13/16		liangliang			Create file
 
 *******************************************************************************/

#ifdef __QC_OS_WIN32__

#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "qcErr.h"
#include "USocket.h"

int qcSockErrno () {
#ifdef __QC_OS_WIN32__
    int err = WSAGetLastError();
    switch (err) {
        case WSAEWOULDBLOCK:
            return EAGAIN;
        case WSAEINTR:
            return EINTR;
        case WSAEPROTONOSUPPORT:
            return EPROTONOSUPPORT;
        case WSAETIMEDOUT:
            return ETIMEDOUT;
        case WSAECONNREFUSED:
            return ECONNREFUSED;
        case WSAEINPROGRESS:
            return EINPROGRESS;
    }
    return -err;
#else
    return errno;
#endif
}

int qcSockSetNonblock (int fd, int enable)
{
#ifdef __QC_OS_WIN32__
    u_long param = enable;
    return ioctlsocket(fd, FIONBIO, &param);
#else
    if (enable)
        return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    else
        return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
#endif // __QC_OS_WIN32__
}

int qcSockSelect (int fd, int write, int read, int timeout)
{
    fd_set read_set;
    fd_set write_set;
    fd_set exception_set;
    
#if __QC_OS_WIN32__
    if (numfds >= FD_SETSIZE)
    {
        errno = EINVAL;
        return QC_ERR_FAILED;
    }
#endif /* HAVE_WINSOCK2_H */
    
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&exception_set);
    
    int n = 0;
    if (fd < 0)
        return QC_ERR_FAILED;
#if !__QC_OS_WIN32__
    if (fd >= FD_SETSIZE)
    {
        errno = EINVAL;
        return QC_ERR_FAILED;
    }
#endif /* !HAVE_WINSOCK2_H */
    
    if (write)
        FD_SET(fd, &write_set);
    if (read)
        FD_SET(fd, &read_set);
    
    FD_SET(fd, &exception_set);
    
    if (fd >= n)
        n = fd + 1;
    
    if (n == 0)
        return QC_ERR_FAILED;
    
    int ret = 0;
    if (timeout < 0)
    {
        ret = select(n, &read_set, &write_set, &exception_set, NULL);
    }
    else
    {
        struct timeval tv;
        tv.tv_sec  = timeout / 1000;
        tv.tv_usec = 1000 * (timeout % 1000);
        ret         = select(n, &read_set, &write_set, &exception_set, &tv);
    }
    
    if (ret < 0)
    {
        return QC_ERR_IO_AGAIN;
    }
    else if (ret == 0)
    {
        return QC_ERR_TIMEOUT;
    }
    else if (read && FD_ISSET(fd, &read_set))
    {
        return QC_ERR_NONE;
    }
    else if (write && FD_ISSET(fd, &write_set))
    {
        return QC_ERR_NONE;
    }
    
    return QC_ERR_IO_AGAIN;
}

int qcSockSocket (int pf, int type, int proto)
{
    int fd = socket(pf, type, proto);
    if (fd == -1) {
        return QC_ERR_FAILED;
    }
    
    return fd;
}

int qcSockConnect (int fd, const struct sockaddr * addr, socklen_t addrlen)
{
    if (qcSockSetNonblock(fd, 1) == -1) {
        return QC_ERR_FAILED;
    }
    
    int ret = connect(fd, addr, addrlen);
    
    if (ret == -1) {
        ret = qcSockErrno();
        switch (ret) {
            case EINTR:
            case EINPROGRESS:
            case EAGAIN:
                return QC_ERR_IO_AGAIN;
            default:
                return QC_ERR_FAILED;
        }
    }
    return  QC_ERR_NONE;
}

void qcSockClose(int fd)
{
    if (fd > -1) {
#ifdef __QC_OS_WIN32__
        closesocket(fd);
#else
        close(fd);
#endif
    }
}

int qcSockRead (int fd, unsigned char * pBuff, int nSize)
{
    int ret = 0;
    ret = (int)recv(fd, pBuff, nSize, 0);
    if (ret < 0) {
        ret = qcSockErrno();
        switch (ret) {
            case EINTR:
            case EINPROGRESS:
            case EAGAIN:
                return QC_ERR_IO_AGAIN;
            default:
                return QC_ERR_FAILED;
        }
    }
    else if (0 == ret)
    {
        return QC_ERR_IO_EOF;
    }
    
    return ret;
}

int qcSockWrite (int fd, unsigned char * pBuff, int nSize)
{
    int ret = 0;
    ret = (int)send(fd, pBuff, nSize, 0);
    if (ret < 0) {
        ret = qcSockErrno();
        switch (ret) {
            case EINTR:
            case EINPROGRESS:
            case EAGAIN:
                return QC_ERR_IO_AGAIN;
            default:
                return QC_ERR_FAILED;
        }
    }
    
    return ret;
}
