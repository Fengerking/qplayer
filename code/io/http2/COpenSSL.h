/*******************************************************************************
	File:		COpenSSL.h

	Contains:	The vo audio dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __COpenSSL_H__
#define __COpenSSL_H__

#include "openssl/ssl.h"
#include "CBaseObject.h"
#include "ULibFunc.h"

//int OPENSSL_init_ssl(uint64_t opts, const OPENSSL_INIT_SETTINGS *settings);
typedef int					(* OPENSSL_INIT_SSL) (uint64_t opts, const OPENSSL_INIT_SETTINGS *settings);
// int SSL_library_init(void );
typedef int					(* SSL_LIBRARY_INIT) (void );

//__owur const SSL_METHOD *TLS_client_method(void);
typedef const SSL_METHOD *	(* TLS_CLIENT_METHOD) (void);
//__owur SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);
typedef SSL_CTX *			(* SSL_CTX_NEW) (const SSL_METHOD *meth);
// SSL *SSL_new(SSL_CTX *ctx);
typedef SSL *				(* SSL_NEW) (SSL_CTX *ctx);
// __owur int SSL_set_fd(SSL *s, int fd);
typedef int					(* SSL_SET_FD) (SSL *s, int fd);
// __owur int SSL_connect(SSL *ssl);
typedef int					(* SSL_CONNECT) (SSL *ssl);
// __owur int SSL_read(SSL *ssl, void *buf, int num);
typedef int					(* SSL_READ) (SSL *ssl, void *buf, int num);
// __owur int SSL_write(SSL *ssl, const void *buf, int num);
typedef int					(* SSL_WRITE) (SSL *ssl, const void *buf, int num);

// int SSL_shutdown(SSL *s);
typedef int					(*SSL_SHUTDOWN) (SSL *s);
// void SSL_free(SSL *ssl);
typedef void				(* SSL_FREE) (SSL *ssl);
// void SSL_CTX_free(SSL_CTX *);
typedef void				(* SSL_CTX_FREE) (SSL_CTX *);

// __owur int SSL_get_error(const SSL *s, int ret_code);
typedef int					(* SSL_GET_ERROR) (const SSL *s, int ret_code);

typedef void                (* SSL_SET_CONNECT_STATE)(SSL *s);
typedef int                 (* SSL_DO_HANDSHAKE)(SSL *s);


class COpenSSL : public CBaseObject
{
public:
	COpenSSL(CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenSSL(void);

	virtual int		Init (void);

	virtual int		Connect (int nSocket);
    virtual int		SetConnectState(int nSocket);
    virtual int		DoHandshake();

	virtual int		Read (void * pBuff, int nSize);
	virtual int		Write (const void * pBuff, int nSize);

	virtual int		Disconnect (int nSocket);

	virtual int		Uninit (void);
    
private:
    int CheckSSLErr(int nErr, char* pFuncName);

protected:
	void *				m_hInst;
	qcLibHandle			m_hLib;

	SSL *				m_pSSL;
	SSL_CTX *			m_pSSLCtx;
	SSL_METHOD *		m_pSSLMethod;

	int					m_nSocket;
	bool				m_bConnect;

public:
	OPENSSL_INIT_SSL	m_fSSLInit;
	SSL_LIBRARY_INIT	m_fLIBInit;
	TLS_CLIENT_METHOD	m_fMethod23;
	TLS_CLIENT_METHOD	m_fMethod10;
	TLS_CLIENT_METHOD	m_fMethod30;
	SSL_CTX_NEW			m_fCtxNew;
	SSL_NEW				m_fSSLNew;
	SSL_SET_FD			m_fSetFD;
	SSL_CONNECT			m_fConnect;
	SSL_READ			m_fRead;
	SSL_WRITE			m_fWrite;
	SSL_SHUTDOWN		m_fShutDown;
	SSL_FREE			m_fSSLFree;
	SSL_CTX_FREE		m_fCtxFree;
	SSL_GET_ERROR		m_fGetError;
    SSL_SET_CONNECT_STATE m_fSetConnectState;
    SSL_DO_HANDSHAKE	m_fDoHandshake;
};

#endif // __COpenSSL_H__
