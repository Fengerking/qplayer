#ifndef __TTPOD_TT_ERROR_H
#define __TTPOD_TT_ERROR_H
#include "config.h"
#include <errno.h>
#include <stddef.h>

/* error handling */
#if EDOM > 0
#define AVERROR(e) (-(e))   ///< Returns a negative error code from a POSIX error code, to return from library functions.
#define AVUNERROR(e) (-(e)) ///< Returns a POSIX error code from a library function error return value.
#else
/* Some platforms have E* and errno already negated. */
#define AVERROR(e) (e)
#define AVUNERROR(e) (e)
#endif

#define FFERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))

#define TTERROR_BSF_NOT_FOUND      FFERRTAG(0xF8,'B','S','F') ///< Bitstream filter not found
#define TTERROR_BUG                FFERRTAG( 'B','U','G','!') ///< Internal bug, also see TTERROR_BUG2
#define TTERROR_BUFFER_TOO_SMALL   FFERRTAG( 'B','U','F','S') ///< Buffer too small
#define TTERROR_DECODER_NOT_FOUND  FFERRTAG(0xF8,'D','E','C') ///< Decoder not found
#define TTERROR_DEMUXER_NOT_FOUND  FFERRTAG(0xF8,'D','E','M') ///< Demuxer not found
#define TTERROR_ENCODER_NOT_FOUND  FFERRTAG(0xF8,'E','N','C') ///< Encoder not found
#define TTERROR_EOF                FFERRTAG( 'E','O','F',' ') ///< End of file
#define TTERROR_EXIT               FFERRTAG( 'E','X','I','T') ///< Immediate exit was requested; the called function should not be restarted
#define TTERROR_EXTERNAL           FFERRTAG( 'E','X','T',' ') ///< Generic error in an external library
#define TTERROR_FILTER_NOT_FOUND   FFERRTAG(0xF8,'F','I','L') ///< Filter not found
#define TTERROR_INVALIDDATA        FFERRTAG( 'I','N','D','A') ///< Invalid data found when processing input
#define TTERROR_MUXER_NOT_FOUND    FFERRTAG(0xF8,'M','U','X') ///< Muxer not found
#define TTERROR_OPTION_NOT_FOUND   FFERRTAG(0xF8,'O','P','T') ///< Option not found
#define TTERROR_PATCHWELCOME       FFERRTAG( 'P','A','W','E') ///< Not yet implemented in FFmpeg, patches welcome
#define TTERROR_PROTOCOL_NOT_FOUND FFERRTAG(0xF8,'P','R','O') ///< Protocol not found

#define TTERROR_STREAM_NOT_FOUND   FFERRTAG(0xF8,'S','T','R') ///< Stream not found

#define TTERROR_BUG2               FFERRTAG( 'B','U','G',' ')
#define TTERROR_UNKNOWN            FFERRTAG( 'U','N','K','N') ///< Unknown error, typically from an external library
#define TTERROR_EXPERIMENTAL       (-0x2bb2afa8) ///< Requested feature is flagged experimental. Set strict_std_compliance if you really want to use it.
#define TTERROR_INPUT_CHANGED      (-0x636e6701) ///< Input changed between calls. Reconfiguration is required. (can be OR-ed with TTERROR_OUTPUT_CHANGED)
#define TTERROR_OUTPUT_CHANGED     (-0x636e6702) ///< Output changed between calls. Reconfiguration is required. (can be OR-ed with TTERROR_INPUT_CHANGED)
/* HTTP & RTSP errors */
#define TTERROR_HTTP_BAD_REQUEST   FFERRTAG(0xF8,'4','0','0')
#define TTERROR_HTTP_UNAUTHORIZED  FFERRTAG(0xF8,'4','0','1')
#define TTERROR_HTTP_FORBIDDEN     FFERRTAG(0xF8,'4','0','3')
#define TTERROR_HTTP_NOT_FOUND     FFERRTAG(0xF8,'4','0','4')
#define TTERROR_HTTP_OTHER_4XX     FFERRTAG(0xF8,'4','X','X')
#define TTERROR_HTTP_SERVER_ERROR  FFERRTAG(0xF8,'5','X','X')


#endif /* __TTPOD_TT_ERROR_H */
