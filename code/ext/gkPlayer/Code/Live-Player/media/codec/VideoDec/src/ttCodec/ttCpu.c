#include <stdint.h>

#include "ttCpu.h"
#include "ttCpuInternal.h"
#include "config.h"
#include "ttOpt.h"
#include "ttCommon.h"
#include "ttH264Log.h"

#if HAVE_SCHED_GETAFFINITY
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <sched.h>
#endif
#if HAVE_GETPROCESSAFFINITYMASK
#include <windows.h>
#endif
#if HAVE_SYSCTL
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

static int flags, checked;

int ttv_get_cpu_flags(void)
{
    if (checked)
        return flags;
#if ARCH_AARCH64
    if (ARCH_AARCH64)
        flags = tt_get_cpu_flags_aarch64();
#endif
#if ARCH_ARM
    if (ARCH_ARM)
        flags = tt_get_cpu_flags_arm();
#endif
#if ARCH_PPC
    if (ARCH_PPC)
        flags = tt_get_cpu_flags_ppc();
#endif
#if ARCH_X86
    if (ARCH_X86)
        flags = tt_get_cpu_flags_x86();
#endif

    checked = 1;
    return flags;
}



int ttv_cpu_count(void)
{
    static volatile int printed;

    int nb_cpus = 1;
#if HAVE_SCHED_GETAFFINITY && defined(CPU_COUNT)
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);

    if (!sched_getaffinity(0, sizeof(cpuset), &cpuset))
        nb_cpus = CPU_COUNT(&cpuset);
#elif HAVE_GETPROCESSAFFINITYMASK
    DWORD_PTR proc_aff, sys_aff;
    if (GetProcessAffinityMask(GetCurrentProcess(), &proc_aff, &sys_aff))
        nb_cpus = ttv_popcount64(proc_aff);
#elif HAVE_SYSCTL && defined(HW_NCPU)
    int mib[2] = { CTL_HW, HW_NCPU };
    size_t len = sizeof(nb_cpus);

    if (sysctl(mib, 2, &nb_cpus, &len, NULL, 0) == -1)
        nb_cpus = 0;
#elif HAVE_SYSCONF && defined(_SC_NPROC_ONLN)
    nb_cpus = sysconf(_SC_NPROC_ONLN);
#elif HAVE_SYSCONF && defined(_SC_NPROCESSORS_ONLN)
    nb_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    if (!printed) {
        ttv_log(NULL, TTV_LOG_DEBUG, "detected %d logical cores\n", nb_cpus);
        printed = 1;
    }

    return nb_cpus;
}
