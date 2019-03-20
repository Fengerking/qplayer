#ifndef __MSVC_COMPAT_H__
#define __MSVC_COMPAT_H__


#ifdef _MSC_VER

#define inline __inline
#define __asm__ __asm

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.718281828
#endif

#include <float.h>
#define snprintf _snprintf
#define strcasecmp(string1, string2) _stricmp(string1, string2)

#endif /* _MSC_VER */

#endif /* __MSVC_COMPAT_H__ */
