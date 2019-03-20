/*******************************************************************************
	File:		qcPlayerEng.cpp

	Contains:	qc media engine  implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <windows.h>
#endif // __QC_OS_WIN32__

#include "qcErr.h"
#include "qcType.h"
#include "openssl/ssl.h"

#ifdef __QC_OS_WIN32__
HINSTANCE	qc_hInst = NULL;
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	qc_hInst = (HINSTANCE) hModule;
    return TRUE;
}
#else
void *	g_hInst = NULL;
#endif // __QC_OS_WIN32__

DLLEXPORT_C void qcOpenSSL_add_all_algorithms(void)
{
	return (void)OpenSSL_add_all_algorithms();
}

DLLEXPORT_C void qcSSL_load_error_strings(void)
{
	return (void)SSL_load_error_strings();
}

DLLEXPORT_C int qcOPENSSL_init_ssl(uint64_t opts, const OPENSSL_INIT_SETTINGS *settings)
{
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	int nRC = OPENSSL_init_ssl(opts, settings);
	return nRC;
}

DLLEXPORT_C const SSL_METHOD * qcTLS_method(void)
{
	return TLS_method();
}

DLLEXPORT_C SSL_CTX * qcSSL_CTX_new(const SSL_METHOD *meth)
{
	return SSL_CTX_new(meth);
}

DLLEXPORT_C SSL * qcSSL_new(SSL_CTX *ctx)
{
	return SSL_new(ctx);
}

DLLEXPORT_C int qcSSL_set_fd(SSL * s, int fd)
{
	return SSL_set_fd(s, fd);
}

DLLEXPORT_C int qcSSL_connect(SSL * ssl)
{
	return SSL_connect(ssl);
}

DLLEXPORT_C int qcSSL_read(SSL *ssl, void *buf, int num)
{
	return SSL_read(ssl, buf, num);
}

DLLEXPORT_C int qcSSL_write(SSL *ssl, const void *buf, int num)
{
	return SSL_write(ssl, buf, num);
}
DLLEXPORT_C int qcSSL_shutdown(SSL * s)
{
	return SSL_shutdown(s);
}

DLLEXPORT_C void qcSSL_free(SSL * ssl)
{
	SSL_free(ssl);
}

DLLEXPORT_C void qcSSL_CTX_free(SSL_CTX * ctx)
{
	SSL_CTX_free(ctx);
}

DLLEXPORT_C int qcSSL_get_error(const SSL *s, int ret_code)
{
	return SSL_get_error(s, ret_code);
}

DLLEXPORT_C void qcSSL_set_connect_state(SSL *s)
{
	SSL_set_connect_state(s);
}

DLLEXPORT_C int qcSSL_do_handshake(SSL *s)
{
	return SSL_do_handshake(s);
}
