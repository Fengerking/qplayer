/*******************************************************************************
	File:		CMP4Parser.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-04		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "math.h"

#include "CMP4Parser.h"
#include "CMsgMng.h"

#include "UIntReader.h"
#include "ULogFunc.h"
#include "USourceFormat.h"
#include "UAVParser.h"

#define MOV_TFHD_BASE_DATA_OFFSET       0x01
#define MOV_TFHD_STSD_ID                0x02
#define MOV_TFHD_DEFAULT_DURATION       0x08
#define MOV_TFHD_DEFAULT_SIZE           0x10
#define MOV_TFHD_DEFAULT_FLAGS          0x20
#define MOV_TFHD_DURATION_IS_EMPTY		0x010000
#define MOV_TFHD_DEFAULT_BASE_IS_MOOF	0x020000

#define MOV_TRUN_DATA_OFFSET            0x01
#define MOV_TRUN_FIRST_SAMPLE_FLAGS     0x04
#define MOV_TRUN_SAMPLE_DURATION		0x100
#define MOV_TRUN_SAMPLE_SIZE			0x200
#define MOV_TRUN_SAMPLE_FLAGS			0x400
#define MOV_TRUN_SAMPLE_CTS				0x800

#define MOV_FRAG_SAMPLE_FLAG_DEGRADATION_PRIORITY_MASK 0x0000ffff
#define MOV_FRAG_SAMPLE_FLAG_IS_NON_SYNC               0x00010000
#define MOV_FRAG_SAMPLE_FLAG_PADDING_MASK              0x000e0000
#define MOV_FRAG_SAMPLE_FLAG_REDUNDANCY_MASK           0x00300000
#define MOV_FRAG_SAMPLE_FLAG_DEPENDED_MASK             0x00c00000
#define MOV_FRAG_SAMPLE_FLAG_DEPENDS_MASK              0x03000000

#define MOV_FRAG_SAMPLE_FLAG_DEPENDS_NO                0x02000000
#define MOV_FRAG_SAMPLE_FLAG_DEPENDS_YES               0x01000000

int CMP4Parser::ReadBoxMvex(long long aBoxPos, unsigned int aBoxLen)
{
	m_nTrexCount = (aBoxLen) / (8 + 24);
	m_pTrackExt = new QCMP4TrackExt[m_nTrexCount];

	int nRestLen = (int)aBoxLen;
	int nExtIndex = 0;
	int nBoxSize = 0;
	int nBoxId = 0;

	while (nRestLen > 24)
	{
		nBoxSize = m_pIOReader->ReadUint32BE(aBoxPos);
		nBoxId = m_pIOReader->ReadUint32BE(aBoxPos + 4);
		nRestLen -= nBoxSize;
		if (nBoxId != QC_MKBETAG('t', 'r', 'e', 'x'))
		{
			aBoxPos += nBoxSize;
			continue;
		}

		aBoxPos += 8; // box size and type
		aBoxPos += 4; // version and flag
		m_pTrackExt[nExtIndex].track_id = m_pIOReader->ReadUint32BE(aBoxPos);
		m_pTrackExt[nExtIndex].stsd_id = m_pIOReader->ReadUint32BE(aBoxPos + 4);
		m_pTrackExt[nExtIndex].duration = m_pIOReader->ReadUint32BE(aBoxPos + 8);
		m_pTrackExt[nExtIndex].size = m_pIOReader->ReadUint32BE(aBoxPos + 12);
		m_pTrackExt[nExtIndex].flags = m_pIOReader->ReadUint32BE(aBoxPos + 16);
		aBoxPos += (nBoxSize - 12); // size
		if (m_pTrackExt[nExtIndex].duration > m_llDuration)
			m_llDuration = m_pTrackExt[aBoxPos].duration;
		nExtIndex++;
	}
	m_bMoofIndexTab = false;
	return 0;
}

int CMP4Parser::ReadBoxMoof(long long nBoxPos, unsigned int nBoxLen)
{
	if (m_pFragment == NULL)
	{
		m_pFragment = new QCMP4Fragment();
		memset(m_pFragment, 0, sizeof(QCMP4Fragment));
	}
	m_pFragment->moof_offset = nBoxPos;
	m_pFragment->implicit_offset = nBoxPos;

	if (m_pBaseInst->m_bForceClose == true)
		return QC_ERR_FORCECLOSE;

	TTSampleInfo * pSampleInfo = NULL;
	int nBoxSize = m_pIOReader->ReadUint32BE(nBoxPos);
	int nBoxId = m_pIOReader->ReadUint32BE(nBoxPos + 4);
	if (nBoxSize > 0 && nBoxId != 0)
	{
		while (nBoxId != QC_MKBETAG('m', 'o', 'o', 'f'))
		{
			m_pFragment->moof_offset = nBoxPos + nBoxSize;
			m_pFragment->implicit_offset = nBoxPos + nBoxSize;

			if (nBoxId == QC_MKBETAG('s', 'i', 'd', 'x'))
				ReadBoxSidx(nBoxPos + 8, nBoxSize);

			nBoxPos += nBoxSize;
			nBoxSize = m_pIOReader->ReadUint32BE(nBoxPos);
			nBoxId = m_pIOReader->ReadUint32BE(nBoxPos + 4);
			if (nBoxPos + nBoxSize >= m_llFileSize)
				break;
			if (nBoxSize == 0 || nBoxId == 0)
				break;
			if (m_pBaseInst != NULL && m_pBaseInst->m_bForceClose)
				break;
		}
	}
	if (nBoxId != QC_MKBETAG('m', 'o', 'o', 'f'))
	{
		if (!m_bMoofIndexTab)
		{
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_DURATION, 0, m_llDuration);
		}
		m_bMoofIndexTab = true;
		return QC_ERR_FINISH;
	}
	if (nBoxPos + nBoxSize >= m_llFileSize)
		return QC_ERR_FORMAT;
	if (m_llMoofFirstPos == 0)
		m_llMoofFirstPos = nBoxPos;
	TTMoofIndexInfo * pMoofInfo = NULL;
	if (!m_bMoofIndexTab)
	{
		pMoofInfo = new TTMoofIndexInfo();
		pMoofInfo->llMoofOffset = nBoxPos;
		pMoofInfo->llMoofTime = -1;
		m_lstMoofInfo.AddTail(pMoofInfo);
	}
	int nDataPos = nBoxPos + nBoxSize;

	nBoxPos += 8;
	int nBoxLenLeft = nBoxSize;
	int nOffset = 8;
	int nRC = 0;
	while (nBoxLenLeft > 8)
	{
		nBoxSize = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxId = m_pIOReader->ReadUint32BE(nBoxPos + 4);
		if (nBoxSize == 1)
		{
			if (nBoxLenLeft < 16) {
				return QC_ERR_FORMAT;
			}
			nBoxSize = (unsigned int)m_pIOReader->ReadUint64BE(nBoxPos + 8);
			nOffset = 16;
		}
		if (nBoxSize < 8 || nBoxSize > nBoxLenLeft)
			return QC_ERR_UNSUPPORT;

		nBoxLenLeft -= nBoxSize;

		switch (nBoxId)
		{
		case QC_MKBETAG('m', 'f', 'h', 'd'):
			nRC = ReadBoxMfhd(nBoxPos, nBoxSize);
			break;

		case QC_MKBETAG('t', 'r', 'a', 'f'):
		{
			nRC = ReadBoxTraf(nBoxPos + 8, nBoxSize - 8);
			if (!m_bMoofIndexTab && pMoofInfo != NULL)
			{
				if (m_pVideoTrackInfo != NULL)
				{
					if (m_pFragment->pTrackInfo == m_pVideoTrackInfo)
					{
						pSampleInfo = m_lstSampleInfo.GetHead();
						if (pSampleInfo != NULL && pSampleInfo->iFlag > 0)
							pMoofInfo->llMoofTime = pSampleInfo->iSampleTimeStamp;
					}
				}
				else
				{
					if (m_pFragment->pTrackInfo == m_pAudioTrackInfo)
					{
						pSampleInfo = m_lstSampleInfo.GetHead();
						if (pSampleInfo != NULL)
							pMoofInfo->llMoofTime = pSampleInfo->iSampleTimeStamp;
					}
				}
				if (pMoofInfo->llMoofTime < 0)
				{
					m_lstMoofInfo.Remove(pMoofInfo);
					delete pMoofInfo;
				}
			}
			QC_DATA_BUFF * pBuff = new QC_DATA_BUFF;
			memset(pBuff, 0, sizeof(QC_DATA_BUFF));
			NODEPOS pos = m_lstSampleInfo.GetHeadPosition();
			while (pos != NULL)
			{
				pSampleInfo = m_lstSampleInfo.GetNext(pos);
				nRC = ReadSampleData(pSampleInfo, pBuff);
				if (nRC != QC_ERR_NONE)
					break;
				if (m_pBaseInst != NULL && m_pBaseInst->m_bForceClose)
					break;
			}
			delete pBuff;

			pSampleInfo = m_lstSampleInfo.RemoveHead();
			while (pSampleInfo != NULL)
			{
				delete pSampleInfo;
				pSampleInfo = m_lstSampleInfo.RemoveHead();
			}
		}
			break;

		default:
			break;
		}
		nBoxPos += nBoxSize;

		if (nRC != QC_ERR_NONE)
			break;
		if (nBoxPos >= m_llFileSize)
			break;
		if (m_pBaseInst != NULL && m_pBaseInst->m_bForceClose)
			break;
	}
	nBoxSize = m_pIOReader->ReadUint32BE(nDataPos);
	nBoxId = m_pIOReader->ReadUint32BE(nDataPos + 4);
	m_llMoofNextPos = nDataPos + nBoxSize;

	if (nRC != QC_ERR_NONE)
		return QC_ERR_FORMAT;
	return nRC;
}

int CMP4Parser::ReadBoxMfhd(long long aBoxPos, unsigned int aBoxLen)
{
	return 0;
}

int CMP4Parser::ReadBoxTraf(long long nBoxPos, unsigned int nBoxLen)
{
	int nRC = 0;
	int nBoxSize = 0;
	int nBoxId = 0;
	int nBoxLenLeft = nBoxLen;
	while (nBoxLenLeft > 8)
	{
		nBoxSize = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxId = m_pIOReader->ReadUint32BE(nBoxPos + 4);
		if (nBoxSize < 8 || nBoxSize > nBoxLenLeft)
			return QC_ERR_UNSUPPORT;
		nBoxLenLeft -= nBoxSize;

		switch (nBoxId)
		{
		case QC_MKBETAG('t', 'f', 'h', 'd'):
			nRC = ReadBoxTfhd(nBoxPos + 8, nBoxSize - 8);
			break;

		case QC_MKBETAG('t', 'f', 'd', 't'):
			nRC = ReadBoxTfdt(nBoxPos + 8, nBoxSize - 8);
			break;

		case QC_MKBETAG('t', 'r', 'u', 'n'):
			nRC = ReadBoxTrun(nBoxPos + 8, nBoxSize - 8);
			break;

		default:
			break;
		}
		nBoxPos += nBoxSize;
		if (nRC != QC_ERR_NONE)
			return nRC;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxTfhd(long long nBoxPos, unsigned int nBoxLen)
{
	QCMP4TrackExt *trex = NULL;
	int flags, track_id, i, found = 0;

	flags = m_pIOReader->ReadUint32BE(nBoxPos) & 0X00FFFFFF;
	nBoxPos += 4;
	track_id = m_pIOReader->ReadUint32BE(nBoxPos);
	nBoxPos += 4;
	if (track_id == 0)
		return QC_ERR_FORMAT;

	m_pFragment->track_id = track_id;
	for (i = 0; i < m_nTrexCount; i++)
	{
		if (m_pTrackExt[i].track_id == track_id) 
		{
			trex = &m_pTrackExt[i];
			break;
		}
	}
	if (trex == NULL)
		return QC_ERR_FORMAT;

	if (flags & MOV_TFHD_BASE_DATA_OFFSET)
	{
		m_pFragment->base_data_offset = m_pIOReader->ReadUint64BE(nBoxPos);
		nBoxPos += 8;
	}
	else
	{
		if (flags & MOV_TFHD_DEFAULT_BASE_IS_MOOF)
			m_pFragment->base_data_offset = m_pFragment->moof_offset;
		else
			m_pFragment->base_data_offset = m_pFragment->moof_offset;
	}

	if (flags & MOV_TFHD_STSD_ID)
	{
		m_pFragment->stsd_id = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	else
	{
		m_pFragment->stsd_id = trex->stsd_id;
	}

	if (flags & MOV_TFHD_DEFAULT_DURATION)
	{
		m_pFragment->duration = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	else
	{
		m_pFragment->duration = trex->duration;
	}

	if (flags & MOV_TFHD_DEFAULT_SIZE)
	{
		m_pFragment->size = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	else
	{
		m_pFragment->size = trex->size;
	}

	if (flags & MOV_TFHD_DEFAULT_FLAGS)
	{
		m_pFragment->flags = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	else
	{
		m_pFragment->flags = trex->flags;
	}
	m_pFragment->time = QC_MAX_NUM64_S;

	m_pFragment->pTrackInfo = GetTrackInfo(m_pFragment->track_id - 1);

/*
	for (i = 0; i < c->fragment_index_count; i++) {
		int j;
		MOVFragmentIndex* candidate = c->fragment_index_data[i];
		if (candidate->track_id == frag->track_id) {
			av_log(c->fc, AV_LOG_DEBUG,
				"found fragment index for track %u\n", frag->track_id);
			index = candidate;
			for (j = index->current_item; j < index->item_count; j++) {
				if (frag->implicit_offset == index->items[j].moof_offset) {
					av_log(c->fc, AV_LOG_DEBUG, "found fragment index entry "
						"for track %u and moof_offset %"PRId64"\n",
						frag->track_id, index->items[j].moof_offset);
					frag->time = index->items[j].time;
					index->current_item = j + 1;
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}
	if (index && !found) {
		av_log(c->fc, AV_LOG_DEBUG, "track %u has a fragment index but "
			"it doesn't have an (in-order) entry for moof_offset "
			"%"PRId64"\n", frag->track_id, frag->implicit_offset);
	}
	av_log(c->fc, AV_LOG_TRACE, "frag flags 0x%x\n", frag->flags);
*/
	return 0;
}

int CMP4Parser::ReadBoxTfdt(long long nBoxPos, unsigned int nBoxLen)
{
	QCMP4TrackInfo * pTrackInfo = GetTrackInfo (m_pFragment->track_id -1);
	if (pTrackInfo == NULL)
	{
		QCLOGW("It can't find the track in TFDT!");
		return QC_ERR_FORMAT;
	}

	int nVer = (m_pIOReader->ReadUint32BE(nBoxPos) & 0XFF000000) >> 24;
	nBoxPos += 4;
	if (nVer > 0) {
		pTrackInfo->llTrackEnd = m_pIOReader->ReadUint64BE(nBoxPos);
	}
	else {
		pTrackInfo->llTrackEnd = m_pIOReader->ReadUint32BE(nBoxPos);
	}

/*
	MOVFragment *frag = &c->fragment;
	AVStream *st = NULL;
	MOVStreamContext *sc;
	int version, i;

	for (i = 0; i < c->fc->nb_streams; i++) {
		if (c->fc->streams[i]->id == frag->track_id) {
			st = c->fc->streams[i];
			break;
		}
	}
	if (!st) {
		av_log(c->fc, AV_LOG_ERROR, "could not find corresponding track id %d\n", frag->track_id);
		return AVERROR_INVALIDDATA;
	}
	sc = st->priv_data;
	if (sc->pseudo_stream_id + 1 != frag->stsd_id)
		return 0;
	version = avio_r8(pb);
	avio_rb24(pb); // flags 
	if (version) {
		sc->track_end = avio_rb64(pb);
	}
	else {
		sc->track_end = avio_rb32(pb);
	}
	*/

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxTrun(long long nBoxPos, unsigned int nBoxLen)
{
	QCMP4TrackInfo * pTrackInfo = GetTrackInfo(m_pFragment->track_id - 1);
	if (pTrackInfo == NULL)
	{
		QCLOGW("It can't find the track in TFDT!");
		return QC_ERR_FORMAT;
	}

	int nVer = m_pIOReader->ReadUint32BE(nBoxPos);
	int nFlags = nVer & 0X00FFFFFF;
	nVer = (nVer & 0XFF000000) >> 24;
	nBoxPos += 4;

	int nEntries = m_pIOReader->ReadUint32BE(nBoxPos);
	nBoxPos += 4;

	int data_offset = 0;
	int first_sample_flags = m_pFragment->flags;
	if (nFlags & MOV_TRUN_DATA_OFFSET)
	{
		data_offset = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	if (nFlags & MOV_TRUN_FIRST_SAMPLE_FLAGS)
	{
		first_sample_flags = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;
	}
	long long	dts = pTrackInfo->llTrackEnd - pTrackInfo->iTimeOffset;
	long long	offset = m_pFragment->base_data_offset + data_offset;
	int			distance = 0;
	TTimeToSample	tmSample;
	for (int i = 0; i < nEntries; i++)
	{
		unsigned	sample_size = m_pFragment->size;
		int			sample_flags = i ? m_pFragment->flags : first_sample_flags;
		unsigned	sample_duration = m_pFragment->duration;
		int			keyframe = 0;

		if (nFlags & MOV_TRUN_SAMPLE_DURATION)
		{
			sample_duration = m_pIOReader->ReadUint32BE(nBoxPos);
			nBoxPos += 4;
		}
		if (nFlags & MOV_TRUN_SAMPLE_SIZE)
		{
			sample_size = m_pIOReader->ReadUint32BE(nBoxPos);
			nBoxPos += 4;
		}
		if (nFlags & MOV_TRUN_SAMPLE_FLAGS)
		{
			sample_flags = m_pIOReader->ReadUint32BE(nBoxPos);
			nBoxPos += 4;
		}

		tmSample.iSampleCount = 1;
		if (nFlags & MOV_TRUN_SAMPLE_CTS)
		{
			tmSample.iSampleDelta = m_pIOReader->ReadUint32BE(nBoxPos);
			nBoxPos += 4;
		}
		else
		{
			tmSample.iSampleDelta = 0;
		}

		if (pTrackInfo->iAudio)
			keyframe = 1;
		else {
			keyframe = !(sample_flags & (MOV_FRAG_SAMPLE_FLAG_IS_NON_SYNC |
										  MOV_FRAG_SAMPLE_FLAG_DEPENDS_YES));
		}

		int	nScale = pTrackInfo->iScale;
		if (nScale == 0)
			nScale = 1000;

		// here can add the sample info
		//err = av_add_index_entry(st, offset, dts, sample_size, distance, keyframe ? AVINDEX_KEYFRAME : 0);
		if (pTrackInfo->iAudio)
		{
			//QCLOGI("The moof audio offset:% 8lld   Time:% 8lld   Size:% 8d   .", offset, dts * 1000 / nScale, sample_size);
		}
		else
		{
			//QCLOGI("The moof video offset:% 8lld   Time:% 8lld   Size:% 8d   .", offset, dts * 1000 / nScale, sample_size);
		}

		TTSampleInfo * pSampleInfo = new TTSampleInfo();
		memset(pSampleInfo, 0, sizeof(TTSampleInfo));
		pSampleInfo->iSampleFileOffset = offset;
		pSampleInfo->iSampleEntrySize = sample_size;
		pSampleInfo->iSampleTimeStamp = dts * 1000 / nScale;
		pSampleInfo->iFlag = keyframe;
		m_lstSampleInfo.AddTail(pSampleInfo);

		dts += sample_duration;
		offset += sample_size;
	}
	m_pFragment->implicit_offset = offset;
	pTrackInfo->llTrackEnd = dts + pTrackInfo->iTimeOffset;
/*
	MOVFragment *frag = &c->fragment;
	AVStream *st = NULL;
	MOVStreamContext *sc;
	MOVStts *ctts_data;
	uint64_t offset;
	int64_t dts;
	int data_offset = 0;
	unsigned entries, first_sample_flags = frag->flags;
	int flags, distance, i, err;

	for (i = 0; i < c->fc->nb_streams; i++) {
		if (c->fc->streams[i]->id == frag->track_id) {
			st = c->fc->streams[i];
			break;
		}
	}
	if (!st) {
		av_log(c->fc, AV_LOG_ERROR, "could not find corresponding track id %d\n", frag->track_id);
		return AVERROR_INVALIDDATA;
	}
	sc = st->priv_data;
	if (sc->pseudo_stream_id + 1 != frag->stsd_id && sc->pseudo_stream_id != -1)
		return 0;
	avio_r8(pb); // version 
	flags = avio_rb24(pb);
	entries = avio_rb32(pb);
	av_log(c->fc, AV_LOG_TRACE, "flags 0x%x entries %d\n", flags, entries);

	// Always assume the presence of composition time offsets.
	// Without this assumption, for instance, we cannot deal with a track in fragmented movies that meet the following.
	//  1) in the initial movie, there are no samples.
	//  2) in the first movie fragment, there is only one sample without composition time offset.
	//  3) in the subsequent movie fragments, there are samples with composition time offset. 
	if (!sc->ctts_count && sc->sample_count)
	{
		// Complement ctts table if moov atom doesn't have ctts atom. 
		ctts_data = av_realloc(NULL, sizeof(*sc->ctts_data));
		if (!ctts_data)
			return AVERROR(ENOMEM);
		sc->ctts_data = ctts_data;
		sc->ctts_data[sc->ctts_count].count = sc->sample_count;
		sc->ctts_data[sc->ctts_count].duration = 0;
		sc->ctts_count++;
	}
	if ((uint64_t)entries + sc->ctts_count >= UINT_MAX / sizeof(*sc->ctts_data))
		return AVERROR_INVALIDDATA;
	if ((err = av_reallocp_array(&sc->ctts_data, entries + sc->ctts_count,
		sizeof(*sc->ctts_data))) < 0) {
		sc->ctts_count = 0;
		return err;
	}
	if (flags & MOV_TRUN_DATA_OFFSET)        data_offset = avio_rb32(pb);
	if (flags & MOV_TRUN_FIRST_SAMPLE_FLAGS) first_sample_flags = avio_rb32(pb);
	dts = sc->track_end - sc->time_offset;
	offset = frag->base_data_offset + data_offset;
	distance = 0;
	av_log(c->fc, AV_LOG_TRACE, "first sample flags 0x%x\n", first_sample_flags);
	for (i = 0; i < entries && !pb->eof_reached; i++) {
		unsigned sample_size = frag->size;
		int sample_flags = i ? frag->flags : first_sample_flags;
		unsigned sample_duration = frag->duration;
		int keyframe = 0;

		if (flags & MOV_TRUN_SAMPLE_DURATION) sample_duration = avio_rb32(pb);
		if (flags & MOV_TRUN_SAMPLE_SIZE)     sample_size = avio_rb32(pb);
		if (flags & MOV_TRUN_SAMPLE_FLAGS)    sample_flags = avio_rb32(pb);
		sc->ctts_data[sc->ctts_count].count = 1;
		sc->ctts_data[sc->ctts_count].duration = (flags & MOV_TRUN_SAMPLE_CTS) ?
			avio_rb32(pb) : 0;
		mov_update_dts_shift(sc, sc->ctts_data[sc->ctts_count].duration);
		if (frag->time != AV_NOPTS_VALUE) {
			if (c->use_mfra_for == FF_MOV_FLAG_MFRA_PTS) {
				int64_t pts = frag->time;
				av_log(c->fc, AV_LOG_DEBUG, "found frag time %"PRId64
					" sc->dts_shift %d ctts.duration %d"
					" sc->time_offset %"PRId64" flags & MOV_TRUN_SAMPLE_CTS %d\n", pts,
					sc->dts_shift, sc->ctts_data[sc->ctts_count].duration,
					sc->time_offset, flags & MOV_TRUN_SAMPLE_CTS);
				dts = pts - sc->dts_shift;
				if (flags & MOV_TRUN_SAMPLE_CTS) {
					dts -= sc->ctts_data[sc->ctts_count].duration;
				}
				else {
					dts -= sc->time_offset;
				}
				av_log(c->fc, AV_LOG_DEBUG, "calculated into dts %"PRId64"\n", dts);
			}
			else {
				dts = frag->time - sc->time_offset;
				av_log(c->fc, AV_LOG_DEBUG, "found frag time %"PRId64
					", using it for dts\n", dts);
			}
			frag->time = AV_NOPTS_VALUE;
		}
		sc->ctts_count++;
		if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			keyframe = 1;
		else
			keyframe =
			!(sample_flags & (MOV_FRAG_SAMPLE_FLAG_IS_NON_SYNC |
			MOV_FRAG_SAMPLE_FLAG_DEPENDS_YES));
		if (keyframe)
			distance = 0;
		err = av_add_index_entry(st, offset, dts, sample_size, distance,
			keyframe ? AVINDEX_KEYFRAME : 0);
		if (err < 0) {
			av_log(c->fc, AV_LOG_ERROR, "Failed to add index entry\n");
		}
		av_log(c->fc, AV_LOG_TRACE, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
			"size %d, distance %d, keyframe %d\n", st->index, sc->sample_count + i,
			offset, dts, sample_size, distance, keyframe);
		distance++;
		dts += sample_duration;
		offset += sample_size;
		sc->data_size += sample_size;
		sc->duration_for_fps += sample_duration;
		sc->nb_frames_for_fps++;
	}

	if (pb->eof_reached)
		return AVERROR_EOF;

	frag->implicit_offset = offset;

	sc->track_end = dts + sc->time_offset;
	if (st->duration < sc->track_end)
		st->duration = sc->track_end;
*/
	return 0;
}

int CMP4Parser::ReadBoxSidx(long long nBoxPos, unsigned int nBoxLen)
{
	int				nRC = 0;
	QCMP4TrackExt *	trex = NULL;
	long long		llPTS = 0;
	long long		llOffset = nBoxPos - 8 + nBoxLen;

	int nVer = m_pIOReader->ReadUint32BE(nBoxPos);
	int nFlags = nVer & 0X00FFFFFF;
	nVer = (nVer & 0XFF000000) >> 24;
	nBoxPos += 4;

	int track_id = m_pIOReader->ReadUint32BE(nBoxPos);
	nBoxPos += 4;
	if (track_id == 0)
		return QC_ERR_FORMAT;
	for (int i = 0; i < m_nTrexCount; i++)
	{
		if (m_pTrackExt[i].track_id == track_id)
		{
			trex = &m_pTrackExt[i];
			break;
		}
	}
	if (trex == NULL)
		return QC_ERR_FORMAT;
	QCMP4TrackInfo * pTrackInfo = GetTrackInfo(track_id - 1);
	if (pTrackInfo == NULL)
		return QC_ERR_FORMAT;

	int nScale = m_pIOReader->ReadUint32BE(nBoxPos);
	if (nScale == 0)
		nScale = 1000;
	nBoxPos += 4;

	if (nVer == 0)
	{
		llPTS = m_pIOReader->ReadUint32BE(nBoxPos);
		llOffset += m_pIOReader->ReadUint32BE(nBoxPos + 4);
		nBoxPos += 8;
	}
	else
	{
		llPTS = m_pIOReader->ReadUint64BE(nBoxPos);
		llOffset += m_pIOReader->ReadUint64BE(nBoxPos + 8);
		nBoxPos += 16;
	}
	int nItemCount = m_pIOReader->ReadUint32BE(nBoxPos);
	nItemCount = nItemCount & 0XFF;
	nBoxPos += 4;
	
	int nSize = 0;
	int nDur = 0;
	int nFlag = 0;
	for (int i = 0; i < nItemCount; i++) 
	{
		nSize = m_pIOReader->ReadUint32BE(nBoxPos);
		nDur = m_pIOReader->ReadUint32BE(nBoxPos + 4);
		nDur = nDur * 1000 / nScale;
		nBoxPos += 8;
		if (nSize & 0x80000000) {
			return QC_ERR_FORMAT;
		}
		nFlag = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxPos += 4;

		TTMoofIndexInfo * pMoofInfo = new TTMoofIndexInfo();
		pMoofInfo->llMoofTime = llPTS;
		pMoofInfo->llMoofOffset = llOffset;
		m_lstMoofInfo.AddTail(pMoofInfo);

		llOffset += nSize;
		llPTS += nDur;
	}
	m_llDuration = llPTS;
	m_bMoofIndexTab = true;

	return nRC;
}

QCMP4TrackInfo * CMP4Parser::GetTrackInfo(int nTrackID)
{
	if (m_pVideoTrackInfo != NULL && m_pVideoTrackInfo->nTrackId == nTrackID)
		return m_pVideoTrackInfo;

	QCMP4TrackInfo * pTrackInfo = NULL;
	NODEPOS pos = m_lstAudioTrackInfo.GetHeadPosition();
	while (pos != NULL)
	{
		pTrackInfo = m_lstAudioTrackInfo.GetNext(pos);
		if (pTrackInfo->nTrackId == nTrackID)
			return pTrackInfo;
	}

	return NULL;
}

int CMP4Parser::ReadSampleData(TTSampleInfo * pSampleInfo, QC_DATA_BUFF * pBuff)
{
	if (pSampleInfo == NULL || m_pFragment == NULL || pBuff == NULL)
		return QC_ERR_ARG;

	int nRC = QC_ERR_NONE;
	if (pBuff->nMediaType == 0)
	{
		if (m_pFragment->pTrackInfo == m_pVideoTrackInfo)
		{
			pBuff->nMediaType = QC_MEDIA_Video;
			if (!m_bReadVideoHead)
			{
				m_bReadVideoHead = true;
				CreateHeadDataBuff(pBuff);
			}
		}
		else
		{
			pBuff->nMediaType = QC_MEDIA_Audio;
		}
	}

	QC_DATA_BUFF * pNewBuff = m_pBuffMng->GetEmpty(pBuff->nMediaType, pSampleInfo->iSampleEntrySize + 1024);
	if (pNewBuff == NULL)
		return QC_ERR_MEMORY;
	pNewBuff->uBuffType = QC_BUFF_TYPE_Data;
	pNewBuff->nMediaType = pBuff->nMediaType;
	pNewBuff->llTime = pSampleInfo->iSampleTimeStamp;
	if (pSampleInfo->iFlag > 0)
		pNewBuff->uFlag = QCBUFF_KEY_FRAME;
	if (pNewBuff->uBuffSize < pSampleInfo->iSampleEntrySize + 1024)
	{
		QC_DEL_A(pNewBuff->pBuff);
		pNewBuff->uBuffSize = pSampleInfo->iSampleEntrySize + 1024;
	}
	if (pNewBuff->pBuff == NULL)
		pNewBuff->pBuff = new unsigned char[pNewBuff->uBuffSize];

	int nReadSize = 0;
	if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if (m_bADTSHeader)
			nReadSize = ReadSourceData(pSampleInfo->iSampleFileOffset, pNewBuff->pBuff + 7, pSampleInfo->iSampleEntrySize, QCIO_READ_AUDIO);
		else
			nReadSize = ReadSourceData(pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_AUDIO);
	}
	else if (pBuff->nMediaType == QC_MEDIA_Video)
		nReadSize = ReadSourceData(pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_VIDEO);
	else
		nReadSize = ReadSourceData(pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_DATA);
	if (nReadSize != pSampleInfo->iSampleEntrySize)
	{
		m_pBuffMng->Return(pNewBuff);
		if (nReadSize == QC_ERR_Disconnected || m_nExitRead > 0)
			return QC_ERR_RETRY;
		return QC_ERR_FINISH;
	}

	if (pBuff->nMediaType == QC_MEDIA_Audio)
		m_pCurAudioInfo++;
	else if (pBuff->nMediaType == QC_MEDIA_Video)
		m_pCurVideoInfo++;

	pNewBuff->uSize = nReadSize;

	// For encrype data
	if (m_nMP4KeySize > 0)
	{
		int nKeySize = strlen(m_szFileKeyTxt);
		for (int i = 0; i < nReadSize + 8; i++)
		{
			for (int j = 0; j < nKeySize; j++)
				pNewBuff->pBuff[i] = pNewBuff->pBuff[i] ^ (m_szFileKeyTxt[j] + (nKeySize - j));
		}
	}

	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
		if (m_pVideoTrackInfo->iCodecType == QC_CODEC_ID_MPEG4)
		{

		}
		else
		{
			unsigned int	nFrame = 0;
			int				nKeyFrame = 0;
			int	nErr = ConvertAVCFrame(pNewBuff->pBuff, nReadSize, nFrame, nKeyFrame);
			if (nErr != QC_ERR_NONE)
			{
				m_pBuffMng->Return(pNewBuff);
				return nErr;
			}
			if (m_pVideoTrackInfo->iCodecType == QC_CODEC_ID_H264 && nKeyFrame)
				pNewBuff->uFlag = QCBUFF_KEY_FRAME;
			if (m_nNALLengthSize < 3)
				pNewBuff->uSize = nFrame;
		}
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if (pNewBuff->llTime < 0)
		{
			//			m_pBuffMng->Return(pNewBuff);
			//			return QC_ERR_RETRY;
		}
		if (m_bADTSHeader)
		{
			int	nHeadSize = qcAV_ConstructAACHeader(pNewBuff->pBuff, pNewBuff->uBuffSize, m_pFmtAudio->nSampleRate, m_pFmtAudio->nChannels, nReadSize);
			if (nHeadSize != 7)
			{
				m_pBuffMng->Return(pNewBuff);
				return QC_ERR_RETRY;
			}
			pNewBuff->uSize = nReadSize + 7;
		}
		if (!m_bReadAudioHead)
		{
			m_bReadAudioHead = true;
			pNewBuff->uFlag += QCBUFF_NEW_FORMAT;
			pNewBuff->pFormat = m_pFmtAudio;
		}
	}
	if (pBuff->nMediaType == QC_MEDIA_Audio)
		pNewBuff->llTime += m_nAudioLoopTimes * m_llLoopTime;
	else
		pNewBuff->llTime += m_nVideoLoopTimes * m_llLoopTime;

	m_pBuffMng->Send(pNewBuff);

	if (!m_bMoofIndexTab && m_llDuration < pNewBuff->llTime)
		m_llDuration = pNewBuff->llTime;

	return nRC;
}

int	CMP4Parser::SetFragPos(long long llPos)
{
	CAutoLock lock(&m_mtSample);
	TTMoofIndexInfo * pMoofSeek = NULL;
	TTMoofIndexInfo * pMoofInfo = NULL;
	NODEPOS pos = m_lstMoofInfo.GetHeadPosition();
	while (pos != NULL)
	{
		pMoofInfo = m_lstMoofInfo.GetNext(pos);
		if (pMoofInfo->llMoofTime == llPos)
		{
			pMoofSeek = pMoofInfo;
			break;
		}
		else if (pMoofInfo->llMoofTime > llPos)
		{
			break;
		}
		pMoofSeek = pMoofInfo;
	}

	if (pMoofSeek != NULL)
	{
		m_llMoofNextPos = pMoofSeek->llMoofOffset;
		m_nVideoLoopTimes = 0;
		m_nAudioLoopTimes = 0;
		return QC_ERR_NONE;
	}
	return QC_ERR_STATUS;
}