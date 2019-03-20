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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "rtmp_sys.h"
#include "log.h"

#define MAX_PRINT_LEN	2048

qcRTMP_LogLevel qcRTMP_debuglevel = qcRTMP_LOGERROR;

static int qc_neednl;

static FILE *qc_fmsg;

static qcRTMP_LogCallback qc_rtmp_log_default, *cb = qc_rtmp_log_default;

static const char *qc_levels[] = {
  "CRIT", "ERROR", "WARNING", "INFO",
  "DEBUG", "DEBUG2"
};

static void qc_rtmp_log_default(int level, const char *format, va_list vl)
{
	char str[MAX_PRINT_LEN]="";

	vsnprintf(str, MAX_PRINT_LEN-1, format, vl);

	/* Filter out 'no-name' */
	if ( qcRTMP_debuglevel<qcRTMP_LOGALL && strstr(str, "no-name" ) != NULL )
		return;

	if ( !qc_fmsg ) qc_fmsg = stderr;

	if ( level <= qcRTMP_debuglevel ) {
		if (qc_neednl) {
			putc('\n', qc_fmsg);
			qc_neednl = 0;
		}
		fprintf(qc_fmsg, "%s: %s\n", qc_levels[level], str);
#ifdef _DEBUG
		fflush(qc_fmsg);
#endif
	}
}

void qcRTMP_LogSetOutput(FILE *file)
{
	qc_fmsg = file;
}

void qcRTMP_LogSetLevel(qcRTMP_LogLevel level)
{
	qcRTMP_debuglevel = level;
}

void qcRTMP_LogSetCallback(qcRTMP_LogCallback *cbp)
{
	cb = cbp;
}

qcRTMP_LogLevel qcRTMP_LogGetLevel()
{
	return qcRTMP_debuglevel;
}

void qcRTMP_Log(int level, const char *format, ...)
{
	va_list args;

	if ( level > qcRTMP_debuglevel )
		return;

	va_start(args, format);
	cb(level, format, args);
	va_end(args);
}

static const char qc_hexdig[] = "0123456789abcdef";

void qcRTMP_LogHex(int level, const uint8_t *data, unsigned long len)
{
	unsigned long i;
	char line[50], *ptr;

	if ( level > qcRTMP_debuglevel )
		return;

	ptr = line;

	for(i=0; i<len; i++) {
		*ptr++ = qc_hexdig[0x0f & (data[i] >> 4)];
		*ptr++ = qc_hexdig[0x0f & data[i]];
		if ((i & 0x0f) == 0x0f) {
			*ptr = '\0';
			ptr = line;
			qcRTMP_Log(level, "%s", line);
		} else {
			*ptr++ = ' ';
		}
	}
	if (i & 0x0f) {
		*ptr = '\0';
		qcRTMP_Log(level, "%s", line);
	}
}

void qcRTMP_LogHexString(int level, const uint8_t *data, unsigned long len)
{
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN	80
	char	line[BP_LEN];
	unsigned long i;

	if ( !data || level > qcRTMP_debuglevel )
		return;

	/* in case len is zero */
	line[0] = '\0';

	for ( i = 0 ; i < len ; i++ ) {
		int n = i % 16;
		unsigned off;

		if( !n ) {
			if( i ) qcRTMP_Log( level, "%s", line );
			memset( line, ' ', sizeof(line)-2 );
			line[sizeof(line)-2] = '\0';

			off = i % 0x0ffffU;

			line[2] = qc_hexdig[0x0f & (off >> 12)];
			line[3] = qc_hexdig[0x0f & (off >>  8)];
			line[4] = qc_hexdig[0x0f & (off >>  4)];
			line[5] = qc_hexdig[0x0f & off];
			line[6] = ':';
		}

		off = BP_OFFSET + n*3 + ((n >= 8)?1:0);
		line[off] = qc_hexdig[0x0f & ( data[i] >> 4 )];
		line[off+1] = qc_hexdig[0x0f & data[i]];

		off = BP_GRAPH + n + ((n >= 8)?1:0);

		if ( isprint( data[i] )) {
			line[BP_GRAPH + n] = data[i];
		} else {
			line[BP_GRAPH + n] = '.';
		}
	}

	qcRTMP_Log( level, "%s", line );
}

/* These should only be used by apps, never by the library itself */
void qcRTMP_LogPrintf(const char *format, ...)
{
	char str[MAX_PRINT_LEN]="";
	int len;
	va_list args;
	va_start(args, format);
	len = vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( qcRTMP_debuglevel==qcRTMP_LOGCRIT )
		return;

	if ( !qc_fmsg ) qc_fmsg = stderr;

	if (qc_neednl) {
		putc('\n', qc_fmsg);
		qc_neednl = 0;
	}

    if (len > MAX_PRINT_LEN-1)
          len = MAX_PRINT_LEN-1;
	fprintf(qc_fmsg, "%s", str);
    if (str[len-1] == '\n')
		fflush(qc_fmsg);
}

void qcRTMP_LogStatus(const char *format, ...)
{
	char str[MAX_PRINT_LEN]="";
	va_list args;
	va_start(args, format);
	vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( qcRTMP_debuglevel==qcRTMP_LOGCRIT )
		return;

	if ( !qc_fmsg ) qc_fmsg = stderr;

	fprintf(qc_fmsg, "%s", str);
	fflush(qc_fmsg);
	qc_neednl = 1;
}
