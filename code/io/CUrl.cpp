/*******************************************************************************
	File:		CUrl.cpp
 
	Contains: url api implement file
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/14/16		liangliang			Create file
 
 *******************************************************************************/

#include "ULogFunc.h"
#include "CUrl.h"

static const char khexChars[] = "0123456789abcdef";

CUrl::CUrl (void)
{
    SetObjectName("Curl");
    reset();
}
CUrl::CUrl (const char * s)
{
    SetObjectName("CUrl");
    paserString(s);
}

void CUrl::paserString (const char *s)
{
    reset();
    
    if (NULL == s)
        return;
    
    QCLOGI("parser url : %s", s);
        
    // Url
    m_sUrl.assign(s, s + strlen(s));
    
    // Protocol.
    std::size_t length = std::strcspn(s, ":");
    m_sProtocol.assign(s, s + length);
#ifdef __QC_OS_WIN32__
	char szProt[1024];
	strcpy (szProt, m_sProtocol.c_str ());
	strlwr (szProt);
	m_sProtocol = szProt;
#else
    std::transform(m_sProtocol.begin(), m_sProtocol.end(), m_sProtocol.begin(), std::tolower);
#endif // __QC_OS_WIN32__
    s += length;
    
    // "://".
    if (std::strncmp(s, "://", 3))
    {
        reset();
        return;
    }
    s += 3;
    
    // Host.
    if (*s == '[')
    {
        length = std::strcspn(++s, "]");
        if (s[length] != ']')
        {
            reset();
            return;
        }
        m_sHost.assign(s, s + length);
        m_bIPV6 = true;
        s += length + 1;
        if (std::strcspn(s, ":/?#") != 0)
        {
            reset();
            return;
        }
    }
    else
    {
        length = std::strcspn(s, ":/?#");
        m_sHost.assign(s, s + length);
        s += length;
    }
    
    // Port.
    if (*s == ':')
    {
        length = std::strcspn(++s, "/?#");
        if (length == 0)
        {
            reset();
            return;
        }
        m_sPort.assign(s, s + length);
        for (std::size_t i = 0; i < m_sPort.length(); ++i)
        {
            if (!isdigit(m_sPort[i]))
            {
                reset();
                return;
            }
        }
        s += length;
    }
    
    // Path.
    if (*s == '/')
    {
        length = std::strcspn(s, "?#");
        m_sPath.assign(s, s + length);
        std::string tmp_path;
        m_sPath = unescapePath(m_sPath);
        s += length;
    }
    else
        m_sPath = "/";
    
    // Query.
    if (*s == '?')
    {
        length = std::strcspn(++s, "#");
        m_sPath.assign(s, s + length);
        s += length;
    }
    
    // Fragment.
    if (*s == '#')
        m_sFragment.assign(++s);
    
    QCLOGI("host : %s", m_sHost.c_str());
    QCLOGI("path : %s", m_sPath.c_str());
}

std::string CUrl::toHex (const std::string & s)
{
    std::string ret;
    for (std::string::const_iterator i = s.begin(); i != s.end(); i++)
    {
        ret += khexChars[(int)*i >> 4];
        ret += khexChars[(int)*i & 0xf];
    }
    return ret;
}

std::string CUrl::escapeString (const std::string & s)
{
    std::string ret;
    std::string h;
    
    for (std::string::const_iterator i = s.begin(); i != s.end(); i++)
    {
        h = *i;
        if (!isUnreservedChar(*i))
        {
            h = "%" + toHex(h);
        }
        ret += h;
    }
    return ret;
}

std::string CUrl::unescapePath (const std::string & s)
{
    std::string ret;
    
    for (std::size_t i = 0; i < s.size(); i++)
    {
        if ('%' == s[i])
        {
            if (i + 3 <= s.size())
            {
                unsigned int value = 0;
                for (std::size_t j = i + 1; j < i + 3; j++)
                {
                    value <<= 4;
                    if ('0' <= s[j] && s[j] <= '9')
                    {
                        value += s[j] - '0';
                    }
                    else if ('a' <= s[j] && s[j] <= 'f')
                    {
                        value += s[j] - 'a' + 10;
                    }
                    else if ('A' <= s[j] && s[j] <= 'F')
                    {
                        value += s[j] - 'A' + 10;
                    }
                    else
                    {
                        return ret;
                    }
                }
                ret += static_cast<char>(value);
                i += 2;
            }
            else
            {
                return ret;
            }
        }
        else if (isalnum(s[i]) || isUnreservedChar(s[i]) || isReservedChar(s[i]))
        {
            ret += s[i];
        }
        else
        {
            return ret;
        }
    }
    
    return ret;
}
