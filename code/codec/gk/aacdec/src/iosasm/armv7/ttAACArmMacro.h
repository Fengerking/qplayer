//
//  ttAACArmMacro.h
//  Player
//
//  Created by shun.chen on 14-9-15.
//
//

#if defined(__arm__)
#define ARMV6
#define ARMV7
#define ASM_IOS
#define AAC_ARM_OPT_OPEN 1
#else
#define AAC_ARM_OPT_OPEN 0
#endif
