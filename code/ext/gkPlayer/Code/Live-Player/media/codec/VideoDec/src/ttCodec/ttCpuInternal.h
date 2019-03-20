#ifndef __TTPOD_TT_CPU_INTERNAL_H_
#define __TTPOD_TT_CPU_INTERNAL_H_

#include "ttCpu.h"

#define CPUEXT_SUFFIX(flags, suffix, cpuext)                            \
    (HAVE_ ## cpuext ## suffix && ((flags) & TTV_CPU_FLAG_ ## cpuext))

#define CPUEXT(flags, cpuext) CPUEXT_SUFFIX(flags, , cpuext)

int tt_get_cpu_flags_aarch64(void);
int tt_get_cpu_flags_arm(void);
int tt_get_cpu_flags_ppc(void);
int tt_get_cpu_flags_x86(void);

#endif /* __TTPOD_TT_CPU_INTERNAL_H_ */
