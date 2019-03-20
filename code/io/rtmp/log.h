/*
 *  Copyright (C) 2008-2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#ifndef __RTMP_LOG_H__
#define __RTMP_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Enable this to get full debugging output */
/* #define _DEBUG */

#ifdef _DEBUG
#undef NODEBUG
#endif

typedef enum
{ qcRTMP_LOGCRIT=0, qcRTMP_LOGERROR, qcRTMP_LOGWARNING, qcRTMP_LOGINFO,
  qcRTMP_LOGDEBUG, qcRTMP_LOGDEBUG2, qcRTMP_LOGALL
} qcRTMP_LogLevel;

extern qcRTMP_LogLevel qcRTMP_debuglevel;

typedef void (qcRTMP_LogCallback)(int level, const char *fmt, va_list);
void qcRTMP_LogSetCallback(qcRTMP_LogCallback *cb);
void qcRTMP_LogSetOutput(FILE *file);
#ifdef __GNUC__
void qcRTMP_LogPrintf(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void qcRTMP_LogStatus(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void qcRTMP_Log(int level, const char *format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
void qcRTMP_LogPrintf(const char *format, ...);
void qcRTMP_LogStatus(const char *format, ...);
void qcRTMP_Log(int level, const char *format, ...);
#endif
void qcRTMP_LogHex(int level, const uint8_t *data, unsigned long len);
void qcRTMP_LogHexString(int level, const uint8_t *data, unsigned long len);
void qcRTMP_LogSetLevel(qcRTMP_LogLevel lvl);
qcRTMP_LogLevel qcRTMP_LogGetLevel(void);

#ifdef __cplusplus
}
#endif

#endif
