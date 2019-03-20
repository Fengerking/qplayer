#ifndef __TTPOD_TT_MACROS_H_
#define __TTPOD_TT_MACROS_H_

#define TTV_GLUE(a, b) a ## b
#define TTV_JOIN(a, b) TTV_GLUE(a, b)
#define TTV_PRAGMA(s) _Pragma(#s)

#endif /* __TTPOD_TT_MACROS_H_ */
