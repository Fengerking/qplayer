/** libavcodec DCE definitions
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "libavcodec/avcodec.h"
#include "libavcodec/xvmc_internal.h"
#include "libavcodec/xvididct.h"
#include "libavcodec/wmv2dsp.h"
#include "libavcodec/vp9dsp.h"
#include "libavcodec/vp8dsp.h"
#include "libavcodec/vp56dsp.h"
#include "libavcodec/vp3dsp.h"
#include "libavcodec/vorbisdsp.h"
#include "libavcodec/videodsp.h"
#include "libavcodec/vdpau_compat.h"
#include "libavcodec/x86/vc1dsp.h"
#include "libavcodec/vc1dsp.h"
#include "libavcodec/synth_filter.h"
#include "libavcodec/svq1enc.h"
#include "libavcodec/x86/simple_idct.h"
#include "libavcodec/sbrdsp.h"
#include "libavcodec/rv34dsp.h"
#include "libavcodec/rdft.h"
#include "libavcodec/qpeldsp.h"
#include "libavcodec/aacpsdsp.h"
#include "libavcodec/pixblockdsp.h"
#include "libavcodec/mpegvideo.h"
#include "libavcodec/mpegvideoencdsp.h"
#include "libavcodec/mpegvideodsp.h"
#include "libavcodec/mpegaudiodsp.h"
#include "libavcodec/mlpdsp.h"
#include "libavcodec/me_cmp.h"
#include "libavcodec/lossless_videodsp.h"
#include "libavcodec/lossless_audiodsp.h"
#include "libavcodec/iirfilter.h"
#include "libavcodec/idctdsp.h"
#include "libavcodec/hpeldsp.h"
#include "libavcodec/x86/hevcdsp.h"
#include "libavcodec/hevcpred.h"
#include "libavcodec/hevcdsp.h"
#include "libavcodec/h264qpel.h"
#include "libavcodec/h264dsp.h"
#include "libavcodec/h264chroma.h"
#include "libavcodec/h264pred.h"
#include "libavcodec/h263dsp.h"
#include "libavcodec/g722dsp.h"
#include "libavcodec/fmtconvert.h"
#include "libavcodec/flacdsp.h"
#include "libavcodec/fft.h"
#include "libavcodec/fdctdsp.h"
#include "libavcodec/x86/fdct.h"
#include "libavcodec/celp_math.h"
#include "libavcodec/celp_filters.h"
#include "libavcodec/blockdsp.h"
#include "libavcodec/audiodsp.h"
#include "libavcodec/acelp_vectors.h"
#include "libavcodec/acelp_filters.h"
#include "libavcodec/ac3dsp.h"
#include "libavcodec/aacsbr.h"
#include "libavcodec/aac.h"
#include "libavcodec/aacenc.h"
#include "libavcodec/msmpeg4.h"

#include "libavformat/avformat.h"
#include "libavformat/rtsp.h"
#include "libavformat/mpegts.h"

/* these matrixes will be permuted for the idct */
/*
const int16_t ff_mpeg4_default_intra_matrix[64] = {
	8, 17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45,
};

const int16_t ff_mpeg4_default_non_intra_matrix[64] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33,
};


const uint16_t ff_h263_format[8][2] = {
	{ 0, 0 },
	{ 128, 96 },
	{ 176, 144 },
	{ 352, 288 },
	{ 704, 576 },
	{ 1408, 1152 },
};

const uint8_t ff_h263_chroma_qscale_table[32] = {
	//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
	0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15
};
*/

//void ff_h263dsp_init(H263DSPContext *ctx) { return; }
void ff_mpeg1_clean_buffers(MpegEncContext *s) { return; }
void ff_mpeg1_encode_picture_header(MpegEncContext *s, int picture_number) { return; }
void ff_mpeg1_encode_mb(MpegEncContext *s, int16_t block[8][64], int motion_x, int motion_y){ return; }
void ff_mpeg1_encode_init(MpegEncContext *s) { return; }
void ff_mpeg1_encode_slice_header(MpegEncContext *s) { return; }

int ff_h261_get_picture_format(int width, int height) { return 0; }
void ff_h261_reorder_mb_index(MpegEncContext *s) { return; }
void ff_h261_encode_mb(MpegEncContext *s, int16_t block[6][64], int motion_x, int motion_y) { return; }
void ff_h261_encode_picture_header(MpegEncContext *s, int picture_number) { return; }
void ff_h261_encode_init(MpegEncContext *s) { return; }
void ff_h261_loop_filter(MpegEncContext *s) { return; }

void ff_h263_encode_mb(MpegEncContext *s, int16_t block[6][64], int motion_x, int motion_y) { return; }
void ff_h263_encode_picture_header(MpegEncContext *s, int picture_number) { return; }
void ff_h263_encode_gob_header(MpegEncContext * s, int mb_line) { return; }
//int16_t *ff_h263_pred_motion(MpegEncContext * s, int block, int dir, int *px, int *py) { return 0; }
void ff_h263_encode_init(MpegEncContext *s) { return; }
//void ff_h263_decode_init_vlc(void) { return; }
//int ff_h263_decode_picture_header(MpegEncContext *s) { return 0; }
//int ff_h263_decode_gob_header(MpegEncContext *s) { return 0; }
//void ff_h263_update_motion_val(MpegEncContext * s) { return; }
//void ff_h263_loop_filter(MpegEncContext * s) { return; }
void ff_clean_h263_qscales(MpegEncContext *s) { return; }

int ff_mpeg4_encode_picture_header(MpegEncContext *s, int picture_number) { return 0; }
//int ff_mpeg4_decode_picture_header(Mpeg4DecContext *ctx, GetBitContext *gb) { return 0; }
//void ff_mpeg4_encode_video_packet_header(MpegEncContext *s) { return; }
//void ff_mpeg4_clean_buffers(MpegEncContext *s) { return; }
void ff_mpeg4_stuffing(PutBitContext *pbc) { return; }
void ff_mpeg4_init_partitions(MpegEncContext *s) { return; }
void ff_mpeg4_merge_partitions(MpegEncContext *s) { return; }
void ff_mpeg4_encode_mb(MpegEncContext *s, int16_t block[6][64], int motion_x, int motion_y) { return; }
//int ff_mpeg4_set_direct_mv(MpegEncContext *s, int mx, int my) { return 0; }
void ff_set_mpeg4_time(MpegEncContext *s) { return; }
void ff_clean_mpeg4_qscales(MpegEncContext *s) { return; }
void ff_mpeg4_encode_video_packet_header(MpegEncContext *s) { return; }

int ff_wmv2_encode_picture_header(MpegEncContext * s, int picture_number) { return 0; }
void ff_wmv2_encode_mb(MpegEncContext * s, int16_t block[6][64], int motion_x, int motion_y) { return; }
void ff_wmv2_add_mb(MpegEncContext *s, int16_t block[6][64], uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr) { return; }
int ff_wmv2_decode_picture_header(MpegEncContext * s){ return 0; }
int ff_wmv2_decode_secondary_picture_header(MpegEncContext * s) { return 0; }

int ff_rv10_encode_picture_header(MpegEncContext *s, int picture_number) { return 0; }
void ff_rv20_encode_picture_header(MpegEncContext *s, int picture_number) { return; }
int ff_rv_decode_dc(MpegEncContext *s, int n) { return 0; }

void ff_flv_encode_picture_header(MpegEncContext *s, int picture_number) { return; }
void ff_flv2_encode_ac_esc(PutBitContext *pb, int slevel, int level, int run, int last) { return; }

void ff_hpeldsp_vp3_init_x86(HpelDSPContext *c, int cpu_flags, int flags) { return; }

void ff_mspel_motion(MpegEncContext *s,uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,uint8_t **ref_picture, op_pixels_func(*pix_op)[4],int motion_x, int motion_y, int h) {return;}

// for RTSP
int ff_rtsp_setup_output_streams(AVFormatContext *s, const char *addr) { return 0; }

int ff_rtsp_tcp_write_packet(AVFormatContext *s, RTSPStream *rtsp_st) { return 0; }

int ff_wms_parse_sdp_a_line(AVFormatContext *s, const char *p) { return 0; }

int ff_rtp_chain_mux_open(AVFormatContext **out, AVFormatContext *s,
	AVStream *st, URLContext *handle, int packet_size, int id) {return 0;}

MpegTSContext *avpriv_mpegts_parse_open(AVFormatContext *s) {return NULL;}
int avpriv_mpegts_parse_packet(MpegTSContext *ts, AVPacket *pkt, const uint8_t *buf, int len) { return 0; }
void avpriv_mpegts_parse_close(MpegTSContext *ts) { return 0; }