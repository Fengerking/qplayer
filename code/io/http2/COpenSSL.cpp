/*******************************************************************************
	File:		COpenSSL.cpp

	Contains:	base object implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "COpenSSL.h"
#include "ULogFunc.h"
#include "USystemFunc.h"

#define QC_OPENSSL

COpenSSL::COpenSSL(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_hLib (NULL)
	, m_pSSL (NULL)
	, m_pSSLCtx (NULL)
	, m_pSSLMethod (NULL)
	, m_nSocket (0)
	, m_bConnect (false)
	, m_fSSLInit (NULL)
	, m_fLIBInit (NULL)
	, m_fMethod23 (NULL)
	, m_fMethod30 (NULL)
	, m_fMethod10 (NULL)
	, m_fCtxNew (NULL)
	, m_fSSLNew (NULL)
	, m_fSetFD (NULL)
	, m_fConnect (NULL)
	, m_fRead (NULL)
	, m_fWrite (NULL)
	, m_fShutDown (NULL)
	, m_fSSLFree (NULL)
	, m_fCtxFree (NULL)
	, m_fGetError (NULL)
	, m_fSetConnectState(NULL)
	, m_fDoHandshake(NULL)
{
	SetObjectName ("COpenSSL");

#ifdef __QC_OS_WIN32__
#ifdef QC_OPENSSL
	m_hLib = (qcLibHandle)qcLibLoad ("qcOpenSSL", 0);
#else
	m_hLib = (qcLibHandle)qcLibLoad ("libssl-1_1", 0);
#endif // QC_OPENSSL
	gqc_android_devces_ver = 8;
#elif defined __QC_OS_NDK__
	if (gqc_android_devces_ver >= 6)
		m_hLib = (qcLibHandle)qcLibLoad ("qcOpenSSL", 0);
	else
		m_hLib = (qcLibHandle)qcLibLoad ("ssl", 1);	
#elif defined __QC_OS_IOS__
    m_hLib = (qcLibHandle)qcLibLoad ("ssl", 0);
#endif // __QC_OS_WIN32__
	if (m_hLib != NULL)
	{
#ifdef __QC_OS_WIN32__
#ifdef QC_OPENSSL
		m_fSSLInit = (OPENSSL_INIT_SSL)qcLibGetAddr(m_hLib, "qcOPENSSL_init_ssl", 0);
		m_fMethod23	= (TLS_CLIENT_METHOD)qcLibGetAddr (m_hLib, "qcTLS_method", 0);		
#else
		m_fSSLInit = (OPENSSL_INIT_SSL)qcLibGetAddr(m_hLib, "OPENSSL_init_ssl", 0);
		m_fMethod23 = (TLS_CLIENT_METHOD)qcLibGetAddr(m_hLib, "TLS_method", 0);
#endif // QC_OPENSSL
#elif defined __QC_OS_NDK__
	if (gqc_android_devces_ver >= 6)
	{
		m_fSSLInit = (OPENSSL_INIT_SSL)qcLibGetAddr(m_hLib, "qcOPENSSL_init_ssl", 0);
		m_fMethod23	= (TLS_CLIENT_METHOD)qcLibGetAddr (m_hLib, "qcTLS_method", 0);	
	}
	else
	{
		m_fLIBInit	= (SSL_LIBRARY_INIT)qcLibGetAddr (m_hLib, "SSL_library_init", 0);
		m_fMethod23	= (TLS_CLIENT_METHOD)qcLibGetAddr (m_hLib, "SSLv23_method", 0);	
	}
#endif // __QC_OS_WIN32__

#ifdef __QC_OS_IOS__
        m_fSSLInit	= OPENSSL_init_ssl;
        m_fMethod23	= SSLv23_method;
        m_fCtxNew	= SSL_CTX_new;
        m_fSSLNew	= SSL_new;
        m_fSetFD	= SSL_set_fd;
        m_fConnect	= SSL_connect;
        m_fRead		= SSL_read;
        m_fWrite	= SSL_write;
        m_fShutDown	= SSL_shutdown;
        m_fSSLFree	= SSL_free;
        m_fCtxFree	= SSL_CTX_free;
        m_fGetError	= SSL_get_error;
        m_fSetConnectState = SSL_set_connect_state;
        m_fDoHandshake = SSL_do_handshake;
#else
#ifdef QC_OPENSSL
	if (gqc_android_devces_ver >= 6)
	{
		m_fCtxNew = (SSL_CTX_NEW)qcLibGetAddr(m_hLib, "qcSSL_CTX_new", 0);
		m_fSSLNew	= (SSL_NEW)qcLibGetAddr (m_hLib, "qcSSL_new", 0);
		m_fSetFD	= (SSL_SET_FD)qcLibGetAddr (m_hLib, "qcSSL_set_fd", 0);
		m_fConnect = (SSL_CONNECT)qcLibGetAddr(m_hLib, "qcSSL_connect", 0);
		m_fRead = (SSL_READ)qcLibGetAddr(m_hLib, "qcSSL_read", 0);
		m_fWrite = (SSL_WRITE)qcLibGetAddr(m_hLib, "qcSSL_write", 0);
		m_fShutDown = (SSL_SHUTDOWN)qcLibGetAddr(m_hLib, "qcSSL_shutdown", 0);
		m_fSSLFree = (SSL_FREE)qcLibGetAddr(m_hLib, "qcSSL_free", 0);
		m_fCtxFree = (SSL_CTX_FREE)qcLibGetAddr(m_hLib, "qcSSL_CTX_free", 0);
		m_fGetError = (SSL_GET_ERROR)qcLibGetAddr(m_hLib, "qcSSL_get_error", 0);
        m_fSetConnectState = (SSL_SET_CONNECT_STATE)qcLibGetAddr(m_hLib, "qcSSL_set_connect_state", 0);
        m_fDoHandshake = (SSL_DO_HANDSHAKE)qcLibGetAddr(m_hLib, "qcSSL_do_handshake", 0);
	}
	else
	{
		m_fCtxNew	= (SSL_CTX_NEW)qcLibGetAddr (m_hLib, "SSL_CTX_new", 0);
		m_fSSLNew = (SSL_NEW)qcLibGetAddr(m_hLib, "SSL_new", 0);
		m_fSetFD = (SSL_SET_FD)qcLibGetAddr(m_hLib, "SSL_set_fd", 0);
		m_fConnect = (SSL_CONNECT)qcLibGetAddr(m_hLib, "SSL_connect", 0);
		m_fRead = (SSL_READ)qcLibGetAddr(m_hLib, "SSL_read", 0);
		m_fWrite = (SSL_WRITE)qcLibGetAddr(m_hLib, "SSL_write", 0);
		m_fShutDown = (SSL_SHUTDOWN)qcLibGetAddr(m_hLib, "SSL_shutdown", 0);
		m_fSSLFree = (SSL_FREE)qcLibGetAddr(m_hLib, "SSL_free", 0);
		m_fCtxFree = (SSL_CTX_FREE)qcLibGetAddr(m_hLib, "SSL_CTX_free", 0);
		m_fGetError = (SSL_GET_ERROR)qcLibGetAddr(m_hLib, "SSL_get_error", 0);
        m_fSetConnectState = (SSL_SET_CONNECT_STATE)qcLibGetAddr(m_hLib, "SSL_set_connect_state", 0);
        m_fDoHandshake = (SSL_DO_HANDSHAKE)qcLibGetAddr(m_hLib, "SSL_do_handshake", 0);
	}
#else
		m_fCtxNew	= (SSL_CTX_NEW)qcLibGetAddr (m_hLib, "SSL_CTX_new", 0);
		m_fSSLNew = (SSL_NEW)qcLibGetAddr(m_hLib, "SSL_new", 0);
		m_fSetFD = (SSL_SET_FD)qcLibGetAddr(m_hLib, "SSL_set_fd", 0);
		m_fConnect = (SSL_CONNECT)qcLibGetAddr(m_hLib, "SSL_connect", 0);
		m_fRead = (SSL_READ)qcLibGetAddr(m_hLib, "SSL_read", 0);
		m_fWrite = (SSL_WRITE)qcLibGetAddr(m_hLib, "SSL_write", 0);
		m_fShutDown = (SSL_SHUTDOWN)qcLibGetAddr(m_hLib, "SSL_shutdown", 0);
		m_fSSLFree = (SSL_FREE)qcLibGetAddr(m_hLib, "SSL_free", 0);
		m_fCtxFree = (SSL_CTX_FREE)qcLibGetAddr(m_hLib, "SSL_CTX_free", 0);
		m_fGetError = (SSL_GET_ERROR)qcLibGetAddr(m_hLib, "SSL_get_error", 0);
        m_fSetConnectState = (SSL_SET_CONNECT_STATE)qcLibGetAddr(m_hLib, "SSL_set_connect_state", 0);
        m_fDoHandshake = (SSL_DO_HANDSHAKE)qcLibGetAddr(m_hLib, "SSL_do_handshake", 0);
#endif // QC_OPENSSL
#endif

#ifdef __QC_OS_WIN32__
#ifndef QC_OPENSSL
		SSLeay_add_ssl_algorithms();
		SSL_load_error_strings();
#endif // QC_OPENSSL
#elif defined __QC_OS_NDK__
//		m_fLIBInit ();
#elif defined __QC_OS_IOS__
        //OPENSSL_init_ssl(0, NULL);
        m_fSSLInit (0, NULL);
#endif // __QC_OS_WIN32__
	}
}

COpenSSL::~COpenSSL(void)
{
	Uninit ();
	if (m_hLib != NULL)
	{
		if (gqc_android_devces_ver >= 6)	
			qcLibFree (m_hLib, 0);
		m_hLib = NULL;
	}
}

int COpenSSL::Init (void)
{
	Uninit ();

	if (m_fMethod23 == NULL)
		return QC_ERR_FAILED;

	m_pSSLMethod = (SSL_METHOD *)m_fMethod23 ();
	if (m_pSSLMethod == NULL)
		return QC_ERR_FAILED;

	m_pSSLCtx = m_fCtxNew (m_pSSLMethod);
	if (m_pSSLCtx == NULL)
		return QC_ERR_FAILED;

	m_pSSL = m_fSSLNew (m_pSSLCtx);
	if (m_pSSL == NULL)
		return QC_ERR_FAILED;

	return QC_ERR_NONE;
}

int COpenSSL::Connect (int nSocket)
{
    QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	if (m_pSSL == NULL)
		Init ();

	if (m_fSetFD == NULL || m_pSSL == NULL)
		return QC_ERR_STATUS;

	m_nSocket = nSocket;
	int nRet = m_fSetFD (m_pSSL, nSocket);
	nRet = m_fConnect (m_pSSL);
	if (nRet == -1)
	{
		int nErr = m_fGetError (m_pSSL, nRet);
        CheckSSLErr(nErr, (char*)"SSL_connect");
		return QC_ERR_FAILED;
    }
    m_bConnect = true;
	return QC_ERR_NONE;
}

int COpenSSL::SetConnectState(int nSocket)
{
    QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
    if (m_pSSL == NULL)
        Init ();
    
    if (m_fSetFD == NULL || m_pSSL == NULL || m_fSetConnectState == NULL)
        return QC_ERR_STATUS;
    
    m_nSocket = nSocket;
    int nRet = m_fSetFD (m_pSSL, nSocket);
    m_fSetConnectState (m_pSSL);
    return nRet;
}

int COpenSSL::DoHandshake()
{
    int nRet = QC_ERR_NONE;
    QCLOG_CHECK_FUNC(&nRet, m_pBaseInst, 0);
    if(!m_fDoHandshake || !m_pSSL || !m_fGetError)
        return QC_ERR_STATUS;
    
    nRet = m_fDoHandshake(m_pSSL);
    if (nRet != 1)
    {
        int err = m_fGetError (m_pSSL, nRet);
        CheckSSLErr(err, (char*)"DoHandshake");
        nRet = err;
        return nRet;
    }
    
    nRet = QC_ERR_NONE;
    m_bConnect = true;
    return nRet;
}

int COpenSSL::Read (void * pBuff, int nSize)
{
	if (m_fRead == NULL || m_pSSL == NULL || !m_bConnect)
		return QC_ERR_STATUS;
	int nRC = m_fRead (m_pSSL, pBuff, nSize);
    if (nRC < 0)
    {
        int nErr = m_fGetError (m_pSSL, nRC);
        CheckSSLErr(nErr, (char*)"SSL_read");
    }
	return nRC;
}

int COpenSSL::Write (const void * pBuff, int nSize)
{
	if (m_fWrite == NULL || m_pSSL == NULL || !m_bConnect)
		return QC_ERR_STATUS;
	int nRC = m_fWrite (m_pSSL, pBuff, nSize);
    if (nRC < 0)
    {
        int nErr = m_fGetError (m_pSSL, nRC);
        CheckSSLErr(nErr, (char*)"SSL_write");
    }
	return nRC;
}


int COpenSSL::Disconnect (int nSocket)
{
	if (m_bConnect)
	{
		m_fShutDown (m_pSSL);
		// Close Socket?
	}

	m_nSocket = 0;
	m_bConnect = false;

	Uninit ();

	return QC_ERR_NONE;
}

int COpenSSL::Uninit (void)
{
	if (m_nSocket != 0)
		Disconnect (m_nSocket);
	if (m_pSSL != NULL)
	{
		m_fSSLFree (m_pSSL);
		m_pSSL = NULL;
	}
	if (m_pSSLCtx != NULL)
	{
		m_fCtxFree (m_pSSLCtx);
		m_pSSLCtx = NULL;
	}
	return QC_ERR_NONE;
}

#ifndef __QC_OS_IOS__
int OPENSSL_init_ssl(uint64_t opts, const OPENSSL_INIT_SETTINGS *settings)
{
	QCLOGT ("COpenSSL", "OPENSSL OPENSSL_init_ssl ");
	int nRC = 0;
//	if (g_qcOpenSSL != NULL && g_qcOpenSSL->m_fSSLInit != NULL)
//		nRC = g_qcOpenSSL->m_fSSLInit (opts, settings);
	return nRC;
}
#endif

int COpenSSL::CheckSSLErr(int nErr, char* pFuncName)
{
    switch(nErr)
    {
        case SSL_ERROR_WANT_READ :
            QCLOGE( "%s failed ,SSL_ERROR_WANT_READ ", pFuncName );
            break;
        case SSL_ERROR_WANT_WRITE:
            QCLOGE( "%s failed ,SSL_ERROR_WANT_WRITE ", pFuncName );
            break;
        case SSL_ERROR_NONE:
            QCLOGE( "%s failed ,SSL_ERROR_NONE ", pFuncName );
            break;
        case SSL_ERROR_ZERO_RETURN:
            QCLOGE( "%s failed ,SSL_ERROR_ZERO_RETURN ", pFuncName );
            break;
        case SSL_ERROR_WANT_CONNECT:
            QCLOGE( "%s failed ,SSL_ERROR_WANT_CONNECT ", pFuncName );
            break;
        case SSL_ERROR_WANT_ACCEPT:
            QCLOGE( "%s failed ,SSL_ERROR_WANT_ACCEPT ", pFuncName );
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            QCLOGE( "%s failed ,SSL_ERROR_WANT_X509_LOOKUP ", pFuncName );
            break;
        case SSL_ERROR_SYSCALL:
            QCLOGE( "%s failed ,SSL_ERROR_SYSCALL ", pFuncName );
            break;
        case SSL_ERROR_SSL:
            QCLOGE( "%s failed ,SSL_ERROR_SSL ", pFuncName );
            break;
        default:
            QCLOGE( "%s failed ,SSL_ERROR_unknown %d", pFuncName, nErr );
            break;
    }
    return nErr;
}
