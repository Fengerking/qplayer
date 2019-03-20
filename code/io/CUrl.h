/*******************************************************************************
	File:		CUrl.h
 
	Contains: url api header file
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/14/16		liangliang			Create file
 
    List of unreserved characters is here: http://en.wikipedia.org/wiki/Percent-encoding
 *******************************************************************************/

#ifndef __CUrl_H__
#define __CUrl_H__

#include <string>

#include "CBaseObject.h"

class CUrl : CBaseObject
{
public:
    CUrl (void);
    CUrl (const char * s);
    virtual ~CUrl (void) {};
    
    /**
     * Paser a URL string into its components
     * @param s a URL string
     */
    void paserString (const char * s);
    
    /**
     * Gets the absolute url
     * @retruns A string of url
     */
    const std::string & url () const
    {
        return m_sUrl;
    }
    
    /**
     * Gets the protocol component of the URL.
     * @returns A string specifying the protocol of the URL. Examples include
     * http, https.
     */
    const std::string & protocol () const
    {
        return m_sProtocol;
    }
    
    /**
     * Gets the host component of the URL.
     * @returns A string containing the host name of the URL.
     */
    const std::string & host () const
    {
        return m_sHost;
    }
    
    /**
     * Gets the port component of the URL.
     * @returns The port number of the URL.
     *
     * @discussion
     * If the URL string did not specify a port, and the protocol is one of
     * http, https, an appropriate default port number is returned.
     */
    unsigned short port () const
    {
        if (!m_sPort.empty())
#ifdef __QC_OS_WIN32__
            return atoi (m_sPort.c_str ());
#else
            return std::stoi(m_sPort.c_str());
#endif // __QC_OS_WIN32__
        if (m_sProtocol == "http")
            return 80;
        if (m_sProtocol == "https")
            return 443;
        if (m_sProtocol == "ftp")
            return 21;
        return 0;
    }

    /**
     * Gets the path component of the URL.
     * @returns A string containing the path of the URL.
     *
     * @discussion
     * The path string is unescaped. To obtain the path in escaped form, use
     * to_string(url::path_component).
     */
    std::string path () const
    {
        return unescapePath(m_sPath);
    }
    
    /**
     * Gets the query component of the URL.
     * @returns A string containing the query string of the URL.
     *
     * @discussion
     * The query string is not unescaped, but is returned in whatever form it
     * takes in the original URL string.
     */
    const std::string & query () const
    {
        return m_sQuery;
    }
    
    /**
     * Gets the fragment component of the URL.
     * @returns A string containing the fragment of the URL.
     */
    const std::string & fragment () const
    {
        return m_sFragment;
    }
    
private:
    inline static bool isChar(int c)
    {
        return c >= 0 && c<= 127;
    }
    
    inline static bool isDigit(int c)
    {
        return c >= '0' && c <= '9';
    }
    
    inline static bool isCtl(int c)
    {
        return (c >= 0 && c <= 32) || 127 == c;
    }
    
    inline static bool isUnreservedChar(const char c)
    {
        return (c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') ||
               ('-' == c) ||
               ('_' == c) ||
               ('.' == c) ||
               ('~' == c);
    }
    
    inline static bool isReservedChar(const char c)
    {
        return ('!'  == c) ||
               ('*'  == c) ||
               ('\'' == c) ||
               ('('  == c) ||
               (')'  == c) ||
               (';'  == c) ||
               (':'  == c) ||
               ('@'  == c) ||
               ('&'  == c) ||
               ('='  == c) ||
               ('+'  == c) ||
               ('$'  == c) ||
               (','  == c) ||
               ('/'  == c) ||
               ('?'  == c) ||
               ('#'  == c) ||
               ('['  == c) ||
               (']'  == c);
    }
    
    inline static bool isTspecial(int c)
    {
        return (' ' == c) || ('`' == c) || ('{' == c) || ('}' == c) || ('^' == c) || ('|' == c);
    }
    
    static std::string toHex(const std::string & s);
    static std::string escapeString(const std::string & s);
    static std::string unescapePath(const std::string & s);
    
    
    void reset()
    {
        m_bIPV6 = false;
        m_sUrl.clear();
        m_sProtocol.clear();
        m_sHost.clear();
        m_sPort.clear();
        m_sPath.clear();
        m_sQuery.clear();
        m_sFragment.clear();
    }
    
    bool        m_bIPV6;
    std::string m_sUrl;
    std::string m_sProtocol;
    std::string m_sHost;
    std::string m_sPort;
    std::string m_sPath;
    std::string m_sQuery;
    std::string m_sFragment;
};

#endif /* __CUrl_H__ */
