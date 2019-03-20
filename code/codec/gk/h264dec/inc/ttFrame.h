#ifndef __TTPOD_TT_FRAME_H_
#define __TTPOD_TT_FRAME_H_

#include <stdint.h>

#include "ttAvutil.h"
#include "ttBuffer.h"
#include "ttDict.h"
#include "ttRational.h"
#include "ttPixfmt.h"
#include "ttVersion.h"


/**
 * @defgroup lavu_frame TTFrame
 * @ingroup lavu_data
 *
 * @{
 * TTFrame is an abstraction for reference-counted raw multimedia data.
 */

enum TTFrameSideDataType {
    /**
     * The data is the TTPanScan struct defined in libavcodec.
     */
    TTV_FRAME_DATA_PANSCAN,
    /**
     * ATSC A53 Part 4 Closed Captions.
     * A53 CC bitstream is stored as uint8_t in TTFrameSideData.data.
     * The number of bytes of CC data is TTFrameSideData.size.
     */
    TTV_FRAME_DATA_A53_CC,
    /**
     * Stereoscopic 3d metadata.
     * The data is the AVStereo3D struct defined in libavutil/stereo3d.h.
     */
    TTV_FRAME_DATA_STEREO3D,
    /**
     * The data is the AVMatrixEncoding enum defined in libavutil/channel_layout.h.
     */
    TTV_FRAME_DATA_MATRIXENCODING,
    /**
     * Metadata relevant to a downmix procedure.
     * The data is the AVDownmixInfo struct defined in libavutil/downmix_info.h.
     */
    TTV_FRAME_DATA_DOWNMIX_INFO,
    /**
     * ReplayGain information in the form of the AVReplayGain struct.
     */
    TTV_FRAME_DATA_REPLAYGAIN,
    /**
     * This side data contains a 3x3 transformation matrix describing an affine
     * transformation that needs to be applied to the frame for correct
     * presentation.
     *
     * See ttDisplay.h for a detailed description of the data.
     */
    TTV_FRAME_DATA_DISPLAYMATRIX,
    /**
     * Active Format Description data consisting of a single byte as specified
     * in ETSI TS 101 154 using TTActiveFormatDescription enum.
     */
    TTV_FRAME_DATA_AFD,
    /**
     * Motion vectors exported by some codecs (on demand through the export_mvs
     * flag set in the libavcodec TTCodecContext flags2 option).
     * The data is the AVMotionVector struct defined in
     * libavutil/motion_vector.h.
     */
    TTV_FRAME_DATA_MOTION_VECTORS,
    /**
     * Recommmends skipping the specified number of samples. This is exported
     * only if the "skip_manual" AVOption is set in libavcodec.
     * This has the same format as TTV_PKT_DATA_SKIP_SAMPLES.
     * @code
     * u32le number of samples to skip from start of this packet
     * u32le number of samples to skip from end of this packet
     * u8    reason for start skip
     * u8    reason for end   skip (0=padding silence, 1=convergence)
     * @endcode
     */
    TTV_FRAME_DATA_SKIP_SAMPLES,
};

enum TTActiveFormatDescription {
    TTV_AFD_SAME         = 8,
    TTV_AFD_4_3          = 9,
    TTV_AFD_16_9         = 10,
    TTV_AFD_14_9         = 11,
    TTV_AFD_4_3_SP_14_9  = 13,
    TTV_AFD_16_9_SP_14_9 = 14,
    TTV_AFD_SP_4_3       = 15,
};

typedef struct TTFrameSideData {
    enum TTFrameSideDataType type;
    uint8_t *data;
    int      size;
    TTDictionary *metadata;
} TTFrameSideData;

/**
 * This structure describes decoded (raw) audio or video data.
 *
 * TTFrame must be allocated using ttv_frame_alloc(). Note that this only
 * allocates the TTFrame itself, the buffers for the data must be managed
 * through other means (see below).
 * TTFrame must be freed with ttv_frame_free().
 *
 * TTFrame is typically allocated once and then reused multiple times to hold
 * different data (e.g. a single TTFrame to hold frames received from a
 * decoder). In such a case, ttv_frame_unref() will free any references held by
 * the frame and reset it to its original clean state before it
 * is reused again.
 *
 * The data described by an TTFrame is usually reference counted through the
 * AVBuffer API. The underlying buffer references are stored in TTFrame.buf /
 * TTFrame.extended_buf. An TTFrame is considered to be reference counted if at
 * least one reference is set, i.e. if TTFrame.buf[0] != NULL. In such a case,
 * every single data plane must be contained in one of the buffers in
 * TTFrame.buf or TTFrame.extended_buf.
 * There may be a single buffer for all the data, or one separate buffer for
 * each plane, or anything in between.
 *
 * sizeof(TTFrame) is not a part of the public ABI, so new fields may be added
 * to the end with a minor bump.
 * Similarly fields that are marked as to be only accessed by
 * ttv_opt_ptr() can be reordered. This allows 2 forks to add fields
 * without breaking compatibility with each other.
 */
typedef struct TTFrame {
#define TTV_NUM_DATA_POINTERS 8
    /**
     * pointer to the picture/channel planes.
     * This might be different from the first allocated byte
     *
     * Some decoders access areas outside 0,0 - width,height, please
     * see ttcodec_align_dimensions2(). Some filters and swscale can read
     * up to 16 bytes beyond the planes, if these filters are to be used,
     * then 16 extra bytes must be allocated.
     */
    uint8_t *data[TTV_NUM_DATA_POINTERS];

    /**
     * For video, size in bytes of each picture line.
     * For audio, size in bytes of each plane.
     *
     * For audio, only linesize[0] may be set. For planar audio, each channel
     * plane must be the same size.
     *
     * For video the linesizes should be multiples of the CPUs alignment
     * preference, this is 16 or 32 for modern desktop CPUs.
     * Some code requires such alignment other code can be slower without
     * correct alignment, for yet other it makes no difference.
     *
     * @note The linesize may be larger than the size of usable data -- there
     * may be extra padding present for performance reasons.
     */
    int linesize[TTV_NUM_DATA_POINTERS];

    /**
     * pointers to the data planes/channels.
     *
     * For video, this should simply point to data[].
     *
     * For planar audio, each channel has a separate data pointer, and
     * linesize[0] contains the size of each channel buffer.
     * For packed audio, there is just one data pointer, and linesize[0]
     * contains the total size of the buffer for all channels.
     *
     * Note: Both data and extended_data should always be set in a valid frame,
     * but for planar audio with more channels that can fit in data,
     * extended_data must be used in order to access all channels.
     */
    uint8_t **extended_data;

    /**
     * width and height of the video frame
     */
    int width, height;

    /**
     * number of audio samples (per channel) described by this frame
     */
    int nb_samples;

    /**
     * format of the frame, -1 if unknown or unset
     * Values correspond to enum TTPixelFormat for video frames,
     * enum AVSampleFormat for audio)
     */
    int format;

    /**
     * 1 -> keyframe, 0-> not
     */
    int key_frame;

    /**
     * Picture type of the frame.
     */
    enum TTPictureType pict_type;

#if TT_API_AVFRAME_LAVC
    attribute_deprecated
    uint8_t *base[TTV_NUM_DATA_POINTERS];
#endif

    /**
     * Sample aspect ratio for the video frame, 0/1 if unknown/unspecified.
     */
    TTRational sample_aspect_ratio;

    /**
     * Presentation timestamp in time_base units (time when frame should be shown to user).
     */
    int64_t pts;

    /**
     * PTS copied from the TTPacket that was decoded to produce this frame.
     */
    int64_t pkt_pts;

    /**
     * DTS copied from the TTPacket that triggered returning this frame. (if frame threading isn't used)
     * This is also the Presentation time of this TTFrame calculated from
     * only TTPacket.dts values without pts values.
     */
    int64_t pkt_dts;

    /**
     * picture number in bitstream order
     */
    int coded_picture_number;
    /**
     * picture number in display order
     */
    int display_picture_number;

    /**
     * quality (between 1 (good) and TT_LAMBDA_MAX (bad))
     */
    int quality;

#if TT_API_AVFRAME_LAVC
    attribute_deprecated
    int reference;

    /**
     * QP table
     */
    attribute_deprecated
    int8_t *qscale_table;
    /**
     * QP store stride
     */
    attribute_deprecated
    int qstride;

    attribute_deprecated
    int qscale_type;

    /**
     * mbskip_table[mb]>=1 if MB didn't change
     * stride= mb_width = (width+15)>>4
     */
    attribute_deprecated
    uint8_t *mbskip_table;

    /**
     * motion vector table
     * @code
     * example:
     * int mv_sample_log2= 4 - motion_subsample_log2;
     * int mb_width= (width+15)>>4;
     * int mv_stride= (mb_width << mv_sample_log2) + 1;
     * motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];
     * @endcode
     */
    int16_t (*motion_val[2])[2];

    /**
     * macroblock type table
     * mb_type_base + mb_width + 2
     */
    attribute_deprecated
    uint32_t *mb_type;

    /**
     * DCT coefficients
     */
    attribute_deprecated
    short *dct_coeff;

    /**
     * motion reference frame index
     * the order in which these are stored can depend on the codec.
     */
    attribute_deprecated
    int8_t *ref_index[2];
#endif

    /**
     * for some private data of the user
     */
    void *opaque;

    /**
     * error
     */
    uint64_t error[TTV_NUM_DATA_POINTERS];

#if TT_API_AVFRAME_LAVC
    attribute_deprecated
    int type;
#endif

    /**
     * When decoding, this signals how much the picture must be delayed.
     * extra_delay = repeat_pict / (2*fps)
     */
    int repeat_pict;

    /**
     * The content of the picture is interlaced.
     */
    int interlaced_frame;

    /**
     * If the content is interlaced, is top field displayed first.
     */
    int top_field_first;

    /**
     * Tell user application that palette has changed from previous frame.
     */
    int palette_has_changed;

#if TT_API_AVFRAME_LAVC
    attribute_deprecated
    int buffer_hints;

    /**
     * Pan scan.
     */
    attribute_deprecated
    struct TTPanScan *pan_scan;
#endif

    /**
     * reordered opaque 64bit (generally an integer or a double precision float
     * PTS but can be anything).
     * The user sets TTCodecContext.reordered_opaque to represent the input at
     * that time,
     * the decoder reorders values as needed and sets TTFrame.reordered_opaque
     * to exactly one of the values provided by the user through TTCodecContext.reordered_opaque
     * @deprecated in favor of pkt_pts
     */
    int64_t reordered_opaque;

#if TT_API_AVFRAME_LAVC
    /**
     * @deprecated this field is unused
     */
    attribute_deprecated void *hwaccel_picture_private;

    attribute_deprecated
    struct TTCodecContext *owner;
    attribute_deprecated
    void *thread_opaque;

    /**
     * log2 of the size of the block which a single vector in motion_val represents:
     * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)
     */
    uint8_t motion_subsample_log2;
#endif

    /**
     * Sample rate of the audio data.
     */
    int sample_rate;

    /**
     * Channel layout of the audio data.
     */
    uint64_t channel_layout;

    /**
     * AVBuffer references backing the data for this frame. If all elements of
     * this array are NULL, then this frame is not reference counted.
     *
     * There may be at most one AVBuffer per data plane, so for video this array
     * always contains all the references. For planar audio with more than
     * TTV_NUM_DATA_POINTERS channels, there may be more buffers than can fit in
     * this array. Then the extra TTBufferRef pointers are stored in the
     * extended_buf array.
     */
    TTBufferRef *buf[TTV_NUM_DATA_POINTERS];

    /**
     * For planar audio which requires more than TTV_NUM_DATA_POINTERS
     * TTBufferRef pointers, this array will hold all the references which
     * cannot fit into TTFrame.buf.
     *
     * Note that this is different from TTFrame.extended_data, which always
     * contains all the pointers. This array only contains the extra pointers,
     * which cannot fit into TTFrame.buf.
     *
     * This array is always allocated using ttv_malloc() by whoever constructs
     * the frame. It is freed in ttv_frame_unref().
     */
    TTBufferRef **extended_buf;
    /**
     * Number of elements in extended_buf.
     */
    int        nb_extended_buf;

    TTFrameSideData **side_data;
    int            nb_side_data;

/**
 * @defgroup lavu_frame_flags TTV_FRAME_FLAGS
 * Flags describing additional frame properties.
 *
 * @{
 */

/**
 * The frame data may be corrupted, e.g. due to decoding errors.
 */
#define TTV_FRAME_FLAG_CORRUPT       (1 << 0)
/**
 * @}
 */

    /**
     * Frame flags, a combination of @ref lavu_frame_flags
     */
    int flags;

    /**
     * MPEG vs JPEG YUV range.
     * It must be accessed using ttv_frame_get_color_range() and
     * ttv_frame_set_color_range().
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorRange color_range;

    enum TTColorPrimaries color_primaries;

    enum TTColorTransferCharacteristic color_trc;

    /**
     * YUV colorspace type.
     * It must be accessed using ttv_frame_get_colorspace() and
     * ttv_frame_set_colorspace().
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorSpace colorspace;

    enum TTChromaLocation chroma_location;

    /**
     * frame timestamp estimated using various heuristics, in stream time base
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_best_effort_timestamp(frame)
     * - encoding: unused
     * - decoding: set by libavcodec, read by user.
     */
    int64_t best_effort_timestamp;

    /**
     * reordered pos from the last TTPacket that has been input into the decoder
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_pkt_pos(frame)
     * - encoding: unused
     * - decoding: Read by user.
     */
    int64_t pkt_pos;

    /**
     * duration of the corresponding packet, expressed in
     * AVStream->time_base units, 0 if unknown.
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_pkt_duration(frame)
     * - encoding: unused
     * - decoding: Read by user.
     */
    int64_t pkt_duration;

    /**
     * metadata.
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_metadata(frame)
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    TTDictionary *metadata;

    /**
     * decode error flags of the frame, set to a combination of
     * TT_DECODE_ERROR_xxx flags if the decoder produced a frame, but there
     * were errors during the decoding.
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_decode_error_flags(frame)
     * - encoding: unused
     * - decoding: set by libavcodec, read by user.
     */
    int decode_error_flags;
#define TT_DECODE_ERROR_INVALID_BITSTREAM   1
#define TT_DECODE_ERROR_MISSING_REFERENCE   2

    /**
     * number of audio channels, only used for audio.
     * Code outside libavcodec should access this field using:
     * ttv_frame_get_channels(frame)
     * - encoding: unused
     * - decoding: Read by user.
     */
    int channels;

    /**
     * size of the corresponding packet containing the compressed
     * frame. It must be accessed using ttv_frame_get_pkt_size() and
     * ttv_frame_set_pkt_size().
     * It is set to a negative value if unknown.
     * - encoding: unused
     * - decoding: set by libavcodec, read by user.
     */
    int pkt_size;

    /**
     * Not to be accessed directly from outside libavutil
     */
    TTBufferRef *qp_table_buf;
} TTFrame;

/**
 * Accessors for some TTFrame fields.
 * The position of these field in the structure is not part of the ABI,
 * they should not be accessed directly outside libavcodec.
 */

void    ttv_frame_set_best_effort_timestamp(TTFrame *frame, int64_t val);
int64_t ttv_frame_get_pkt_duration         (const TTFrame *frame);
void    ttv_frame_set_pkt_duration         (TTFrame *frame, int64_t val);
int64_t ttv_frame_get_pkt_pos              (const TTFrame *frame);
void    ttv_frame_set_pkt_pos              (TTFrame *frame, int64_t val);
int64_t ttv_frame_get_channel_layout       (const TTFrame *frame);
void    ttv_frame_set_channel_layout       (TTFrame *frame, int64_t val);
int     ttv_frame_get_channels             (const TTFrame *frame);
void    ttv_frame_set_channels             (TTFrame *frame, int     val);
int     ttv_frame_get_sample_rate          (const TTFrame *frame);
void    ttv_frame_set_sample_rate          (TTFrame *frame, int     val);
TTDictionary *ttv_frame_get_metadata       (const TTFrame *frame);
void          ttv_frame_set_metadata       (TTFrame *frame, TTDictionary *val);
int     ttv_frame_get_decode_error_flags   (const TTFrame *frame);
void    ttv_frame_set_decode_error_flags   (TTFrame *frame, int     val);
int     ttv_frame_get_pkt_size(const TTFrame *frame);
void    ttv_frame_set_pkt_size(TTFrame *frame, int val);
TTDictionary **ttpriv_frame_get_metadatap(TTFrame *frame);

int ttv_frame_set_qp_table(TTFrame *f, TTBufferRef *buf, int stride, int type);
enum TTColorSpace ttv_frame_get_colorspace(const TTFrame *frame);
void    ttv_frame_set_colorspace(TTFrame *frame, enum TTColorSpace val);
enum TTColorRange ttv_frame_get_color_range(const TTFrame *frame);
void    ttv_frame_set_color_range(TTFrame *frame, enum TTColorRange val);


/**
 * Allocate an TTFrame and set its fields to default values.  The resulting
 * struct must be freed using ttv_frame_free().
 *
 * @return An TTFrame filled with default values or NULL on failure.
 *
 * @note this only allocates the TTFrame itself, not the data buffers. Those
 * must be allocated through other means, e.g. with ttv_frame_get_buffer() or
 * manually.
 */
TTFrame *ttv_frame_alloc(void);

/**
 * Free the frame and any dynamically allocated objects in it,
 * e.g. extended_data. If the frame is reference counted, it will be
 * unreferenced first.
 *
 * @param frame frame to be freed. The pointer will be set to NULL.
 */
void ttv_frame_free(TTFrame **frame);

/**
 * Set up a new reference to the data described by the source frame.
 *
 * Copy frame properties from src to dst and create a new reference for each
 * TTBufferRef from src.
 *
 * If src is not reference counted, new buffers are allocated and the data is
 * copied.
 *
 * @return 0 on success, a negative AVERROR on error
 */
int ttv_frame_ref(TTFrame *dst, const TTFrame *src);


/**
 * Unreference all the buffers referenced by frame and reset the frame fields.
 */
void ttv_frame_unref(TTFrame *frame);

/**
 * Move everythnig contained in src to dst and reset src.
 */
void ttv_frame_move_ref(TTFrame *dst, TTFrame *src);

/**
 * Allocate new buffer(s) for audio or video data.
 *
 * The following fields must be set on frame before calling this function:
 * - format (pixel format for video, sample format for audio)
 * - width and height for video
 * - nb_samples and channel_layout for audio
 *
 * This function will fill TTFrame.data and TTFrame.buf arrays and, if
 * necessary, allocate and fill TTFrame.extended_data and TTFrame.extended_buf.
 * For planar formats, one buffer will be allocated for each plane.
 *
 * @param frame frame in which to store the new buffers.
 * @param align required buffer size alignment
 *
 * @return 0 on success, a negative AVERROR on error.
 */
int ttv_frame_get_buffer(TTFrame *frame, int align);

/**
 * Check if the frame data is writable.
 *
 * @return A positive value if the frame data is writable (which is true if and
 * only if each of the underlying buffers has only one reference, namely the one
 * stored in this frame). Return 0 otherwise.
 *
 * If 1 is returned the answer is valid until ttv_buffer_ref() is called on any
 * of the underlying AVBufferRefs (e.g. through ttv_frame_ref() or directly).
 *
 * @see ttv_frame_make_writable(), ttv_buffer_is_writable()
 */
int ttv_frame_is_writable(TTFrame *frame);

/**
 * Ensure that the frame data is writable, avoiding data copy if possible.
 *
 * Do nothing if the frame is writable, allocate new buffers and copy the data
 * if it is not.
 *
 * @return 0 on success, a negative AVERROR on error.
 *
 * @see ttv_frame_is_writable(), ttv_buffer_is_writable(),
 * ttv_buffer_make_writable()
 */
int ttv_frame_make_writable(TTFrame *frame);

/**
 * Copy the frame data from src to dst.
 *
 * This function does not allocate anything, dst must be already initialized and
 * allocated with the same parameters as src.
 *
 * This function only copies the frame data (i.e. the contents of the data /
 * extended data arrays), not any other properties.
 *
 * @return >= 0 on success, a negative AVERROR on error.
 */
int ttv_frame_copy(TTFrame *dst, const TTFrame *src);

/**
 * Copy only "metadata" fields from src to dst.
 *
 * Metadata for the purpose of this function are those fields that do not affect
 * the data layout in the buffers.  E.g. pts, sample rate (for audio) or sample
 * aspect ratio (for video), but not width/height or channel layout.
 * Side data is also copied.
 */
int ttv_frame_copy_props(TTFrame *dst, const TTFrame *src);



/**
 * Add a new side data to a frame.
 *
 * @param frame a frame to which the side data should be added
 * @param type type of the added side data
 * @param size size of the side data
 *
 * @return newly added side data on success, NULL on error
 */
TTFrameSideData *ttv_frame_new_side_data(TTFrame *frame,
                                        enum TTFrameSideDataType type,
                                        int size);


#endif /* __TTPOD_TT_FRAME_H_ */
