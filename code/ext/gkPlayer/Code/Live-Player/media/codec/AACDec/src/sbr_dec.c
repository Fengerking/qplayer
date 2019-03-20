#include "decoder.h"
#include "sbr_dec.h"

#ifdef SBR_DEC

int sbr_init(AACDecoder* decoder)
{
	sbr_info *sbr;
	if(decoder->sbr==NULL)
	{
		/* allocate SBR state structure */
		sbr = (sbr_info *)RMAACDecAlignedMalloc(sizeof(sbr_info));
		if (sbr==NULL)
			return TTKErrNoMemory;

		decoder->sbr = sbr;
#ifdef PS_DEC        
		sbr->ps = NULL; 
		sbr->ps_used = 0; 
#endif
	}
	
	return 0 ;
}

void sbr_free(AACDecoder* decoder)
{
	int ch;
	if (decoder && decoder->sbr)
	{
 		sbr_info *sbr = decoder->sbr;
	#ifdef PS_DEC
		if (sbr->ps != NULL) 
        {
			ps_free(sbr->ps);
			sbr->ps = NULL;
        }			
	#endif

		for(ch = 0; ch < MAX_CHANNELS; ch++)
		{
			if(sbr->sbrFreq[ch])
			{
				SafeAlignedFree(sbr->sbrFreq[ch]);
			}

			if(sbr->sbrChan[ch])
			{
				SafeAlignedFree(sbr->sbrChan[ch]);
			}

			if(sbr->delayQMFA[ch])
			{
				SafeAlignedFree(sbr->delayQMFA[ch]);
			}

			if(sbr->delayQMFS[ch])
			{
				SafeAlignedFree(sbr->delayQMFS[ch]);
			}

			if(sbr->XBufDelay[ch])
			{
				SafeAlignedFree(sbr->XBufDelay[ch]);
			}
		}

		SafeAlignedFree(decoder->sbr);
	}


	return;
}

void ReSetSBRDate(sbr_info *psi)
{
	if(psi->sbrFreq[0])
	{
		memset(psi->sbrFreq[0], 0, sizeof(SBRFreq));
	}
	if(psi->sbrFreq[1])
	{
		memset(psi->sbrFreq[1], 0, sizeof(SBRFreq));
	}
	if(psi->sbrChan[0])
	{
		memset(psi->sbrChan[0], 0, sizeof(SBRChan));
		psi->sbrChan[0]->reset = 1;
		psi->sbrChan[0]->laPrev = -1;
	}
	if(psi->sbrChan[1])
	{
		memset(psi->sbrChan[1], 0, sizeof(SBRChan));
		psi->sbrChan[1]->reset = 1;
		psi->sbrChan[1]->laPrev = -1;
	}
	
	memset(psi->sbrHdrPrev, 0, sizeof(SBRHeader)*MAX_CHANNELS);
	memset(psi->sbrHdr, 0, sizeof(SBRHeader)*MAX_CHANNELS);
	memset(psi->sbrGrid, 0, sizeof(SBRGrid)*MAX_CHANNELS);
	
	//psi->ps_used = 0;       //delete for #8710, seek do not change the output channel number
	psi->number = 0;
	psi->crcCheckWord = 0;
	psi->couplingFlag = 0;
	psi->envBand = 0;
	psi->eOMGainMax = 0;
	psi->gainMax = 0;
	psi->gainMaxFBits = 0;
	psi->noiseFloorBand = 0;
	psi->qp1Inv = 0;
	psi->qqp1Inv = 0;
	psi->sMapped = 0;
	psi->sBand = 0;
	psi->highBand = 0;

	psi->sumEOrigMapped = 0;
	psi->sumECurrGLim = 0;
	psi->sumSM = 0;
	psi->sumQM = 0;
	memset(psi->G_LimBoost, 0, sizeof(int)*MQ_BANDS);
	memset(psi->Qm_LimBoost, 0, sizeof(int)*MQ_BANDS);
	memset(psi->Sm_Boost, 0, sizeof(int)*MQ_BANDS);

	memset(psi->Sm_Buf, 0, sizeof(int)*MQ_BANDS);
	memset(psi-> Qm_LimBuf, 0, sizeof(int)*MQ_BANDS);
	memset(psi->G_LimBuf, 0, sizeof(int)*MQ_BANDS);
	memset(psi->G_LimFbits, 0, sizeof(int)*MQ_BANDS);

	memset(psi->G_FiltLast, 0, sizeof(int)*MQ_BANDS);
	memset(psi->Q_FiltLast, 0, sizeof(int)*MQ_BANDS);

	return;
}


int ttSBRExtData(AACDecoder *decoder, 
				 int chBase)
{
	int   headerFlag; 
	int   ret;
	TTUint8 ttHdrEx1;
	TTUint8 ttHdrEx2;
	SBRHeader *ttSBRHdr;
	SBRHeader *ttSBRHdrPrev;
	BitStream bs;
	sbr_info *psi;

	psi = (sbr_info *)(decoder->sbr);

	if (decoder->id_syn_ele != ID_FIL || 
		(decoder->fillExtType != SBR_EXTENSION && decoder->fillExtType != SBR_EXTENSION_CRC))
		return 0 ;

	BitStreamInit(&bs, decoder->fillCount, decoder->fillBuf);
	if (BitStreamGetBits(&bs, 4) != (unsigned int)decoder->fillExtType)
		return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
	
	if (decoder->fillExtType == SBR_EXTENSION_CRC)
		psi->crcCheckWord = BitStreamGetBits(&bs, 10);

	if(psi->sbrFreq[chBase] == NULL)
	{
		psi->sbrFreq[chBase] = (SBRFreq *)RMAACDecAlignedMalloc(sizeof(SBRFreq));
		if(psi->sbrFreq[chBase] == NULL)
			return TTKErrNoMemory;
	}

	if(psi->sbrChan[chBase] == NULL)
	{
		psi->sbrChan[chBase] = (SBRChan *)RMAACDecAlignedMalloc(sizeof(SBRChan));
		if(psi->sbrChan[chBase] == NULL)
			return TTKErrNoMemory;

		psi->sbrChan[chBase]->reset = 1;
		psi->sbrChan[chBase]->laPrev = -1;
	}
	
	if(decoder->old_id_syn_ele == ID_CPE)
	{
		if(psi->sbrChan[chBase+1] == NULL)
		{
			psi->sbrChan[chBase+1] = (SBRChan *)RMAACDecAlignedMalloc(sizeof(SBRChan));
			if(psi->sbrChan[chBase+1] == NULL)
				return TTKErrNoMemory;
			
			psi->sbrChan[chBase+1]->reset = 1;
			psi->sbrChan[chBase+1]->laPrev = -1;
		}
	}

	if((decoder->channelNum == 2) && (psi->last_syn_ele != decoder->old_id_syn_ele))
	{
		ReSetSBRDate(psi);
	}

	headerFlag = BitStreamGetBits(&bs, 1);
	if (headerFlag) {
		/* get sample rate index for output sample rate (2x base rate) */
		psi->sampRateIdx = get_sr_index(2 * decoder->sampleRate);
		if (psi->sampRateIdx < 0 || psi->sampRateIdx >= NUM_SAMPLE_RATES)
			return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
		else if (psi->sampRateIdx >= TT_SAMP_RATES_NUM )
			return TT_AACDEC_ERR_AAC_INVSBRSTREAM;

		/*
		* Brief: unpack header data from bitstream  (table 4.56)
		*/

		ttSBRHdr = &(psi->sbrHdr[chBase]);
		ttSBRHdrPrev = &(psi->sbrHdrPrev[chBase]);
		/* save previous values so we know whether to reset decoder */
		ttSBRHdrPrev->startFreq =     ttSBRHdr->startFreq;
		ttSBRHdrPrev->stopFreq =      ttSBRHdr->stopFreq;
		ttSBRHdrPrev->freqScale =     ttSBRHdr->freqScale;
		ttSBRHdrPrev->alterScale =    ttSBRHdr->alterScale;
		ttSBRHdrPrev->crossOverBand = ttSBRHdr->crossOverBand;
		ttSBRHdrPrev->noiseBands =    ttSBRHdr->noiseBands;

		psi->bs_ampRes = ttSBRHdr->ampRes =        SBRGetBits(&bs, SI_SBR_AMP_RES_BITS);
		psi->bs_startFreq = ttSBRHdr->startFreq =     SBRGetBits(&bs, SI_SBR_START_FREQ_BITS);
		psi->bs_stopFreq = ttSBRHdr->stopFreq =      SBRGetBits(&bs, SI_SBR_STOP_FREQ_BITS);
		psi->bs_crossOverBand = ttSBRHdr->crossOverBand = SBRGetBits(&bs, SI_SBR_XOVER_BAND_BITS);
		SBRGetBits(&bs, SI_SBR_RESERVED_BITS_HDR);
		ttHdrEx1 =     SBRGetBits(&bs, SI_SBR_HEADER_EXTRA_1_BITS);
		ttHdrEx2 =     SBRGetBits(&bs, SI_SBR_HEADER_EXTRA_2_BITS);

		if (ttHdrEx1) {
			ttSBRHdr->freqScale =    SBRGetBits(&bs, SI_SBR_FREQ_SCALE_BITS);
			ttSBRHdr->alterScale =   SBRGetBits(&bs, SI_SBR_ALTER_SCALE_BITS);
			ttSBRHdr->noiseBands =   SBRGetBits(&bs, SI_SBR_NOISE_BANDS_BITS);
		} else {
			/* defaults */
			ttSBRHdr->freqScale =    SBR_FREQ_SCALE_DEFAULT;
			ttSBRHdr->alterScale =   SBR_ALTER_SCALE_DEFAULT;
			ttSBRHdr->noiseBands =   SBR_NOISE_BANDS_DEFAULT;
		}

		if (ttHdrEx2) {
			ttSBRHdr->limiterBands = SBRGetBits(&bs, SI_SBR_LIMITER_BANDS_BITS);
			ttSBRHdr->limiterGains = SBRGetBits(&bs, SI_SBR_LIMITER_GAINS_BITS);
			ttSBRHdr->interpFreq =   SBRGetBits(&bs, SI_SBR_INTERPOL_FREQ_BITS);
			ttSBRHdr->smoothMode =   SBRGetBits(&bs, SI_SBR_SMOOTHING_LENGTH_BITS);
		} else {
			/* defaults */
			ttSBRHdr->limiterBands = SBR_LIMITER_BANDS_DEFAULT;
			ttSBRHdr->limiterGains = SBR_LIMITER_GAINS_DEFAULT;
			ttSBRHdr->interpFreq =   SBR_INTERPOL_FREQ_DEFAULT;
			ttSBRHdr->smoothMode =   SBR_SMOOTHING_LENGTH_DEFAULT;
		}
		ttSBRHdr->count++;

		/* if any of these have changed from previous frame, reset the SBR module */
	    /* reset flag = 1 if header values changed */
		if (ttSBRHdr->startFreq != ttSBRHdrPrev->startFreq || ttSBRHdr->stopFreq != ttSBRHdrPrev->stopFreq ||
			ttSBRHdr->freqScale != ttSBRHdrPrev->freqScale || ttSBRHdr->alterScale != ttSBRHdrPrev->alterScale ||
			ttSBRHdr->crossOverBand != ttSBRHdrPrev->crossOverBand || ttSBRHdr->noiseBands != ttSBRHdrPrev->noiseBands
			)
			psi->sbrChan[chBase]->reset = 1;
	
		/* first valid SBR header should always trigger CalcFreqTables(), since psi->reset was set in InitSBR() */
		if (psi->sbrChan[chBase]->reset)
		{
			SBRHeader *sbrHdr;
			SBRHeader *sbrHdrPrevOK;
			ret = ttSBRDecUpdateFreqTables(decoder, 
				                           &(psi->sbrHdr[chBase+0]), 
										   psi->sbrFreq[chBase], 
										   psi->sampRateIdx
										   );
			if(ret)
			{
				if(psi->sbrHdr[chBase+0].count >= 2)
				{
					ret = ttSBRDecUpdateFreqTables(decoder, &(psi->sbrHdrPrev[chBase+0]), psi->sbrFreq[chBase], psi->sampRateIdx);
					if(ret)
					{
						ret = ttSBRDecUpdateFreqTables(decoder, &(psi->sbrHdrPrevOK[chBase+0]), psi->sbrFreq[chBase], psi->sampRateIdx);
						if(ret){
							psi->sbrHdr[chBase].count = 0;
							return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
						}
					}
				}
				else
					psi->sbrHdr[chBase+0].count = 0;
			}
			else
			{
				sbrHdr = &(psi->sbrHdr[chBase+0]);
				sbrHdrPrevOK = &(psi->sbrHdrPrevOK[chBase+0]);	
				sbrHdrPrevOK->startFreq =     sbrHdr->startFreq;
				sbrHdrPrevOK->stopFreq =      sbrHdr->stopFreq;
				sbrHdrPrevOK->freqScale =     sbrHdr->freqScale;
				sbrHdrPrevOK->alterScale =    sbrHdr->alterScale;
				sbrHdrPrevOK->crossOverBand = sbrHdr->crossOverBand;
				sbrHdrPrevOK->noiseBands =    sbrHdr->noiseBands;
			}
		}

		/* copy and reset state to right channel for CPE */
		if (decoder->old_id_syn_ele == ID_CPE)
			psi->sbrChan[chBase+1]->reset = psi->sbrChan[chBase+0]->reset;
	}

	/* if no header has been received, upsample only */
	if (psi->sbrHdr[chBase].count == 0)
		return 0 ;

	if (decoder->old_id_syn_ele == ID_SCE) 
	{
		ret = ttSBR_Single_Channel_Element(decoder,&bs, chBase);		
	} else if (decoder->old_id_syn_ele == ID_CPE) 
	{
		ret = ttSBR_Channel_Pair_Element(decoder,&bs, chBase);
	} else 
	{
		return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
	}

	if(ret < 0) 
	{
		if (psi->sbrChan[chBase]->reset && headerFlag && psi->sbrHdr[chBase+0].count >= 2)
		{
			ret = ttSBRDecUpdateFreqTables(decoder, &(psi->sbrHdrPrev[chBase+0]), psi->sbrFreq[chBase], psi->sampRateIdx);
			if(ret)
			{
				ret = ttSBRDecUpdateFreqTables(decoder, &(psi->sbrHdrPrevOK[chBase+0]), psi->sbrFreq[chBase], psi->sampRateIdx);
				if(ret){
					psi->sbrHdr[chBase].count = 0;
				}
			}
		}
		return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
	}

	psi->last_syn_ele = decoder->old_id_syn_ele;

	BitStreamByteAlign(&bs);

	return 0 ;
}

int DecodeSBRData(AACDecoder *decoder, int chBase, short *outbuf)
{
	int k, l, ch, chBlock, qmfaBands, qmfsBands = 0;
	int upsampleOnly, gbIdx, gbMask, ret;
	int *inbuf;
	int *input;
	short *outptr,*outptr2;
	sbr_info *psi;
	SBRHeader *sbrHdr;
	SBRGrid *sbrGrid;
	SBRFreq *sbrFreq;
	SBRChan *sbrChan;
	int channelNum = decoder->channelNum;
	int channel_stride = decoder->channelNum;
	int *outCh = decoder->channel_offsize;

	if (decoder->id_syn_ele != ID_FIL || (decoder->fillExtType != SBR_EXTENSION && decoder->fillExtType != SBR_EXTENSION_CRC))
		return 0 ;

	/* validate pointers */
	if (!decoder->sbr)
		return TT_AACDEC_ERR_AAC_FAILDECSBR;
	psi = (sbr_info *)(decoder->sbr);
	if(psi->sbrError)
		return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
	/* same header and freq tables for both channels in CPE */
	sbrHdr =  &(psi->sbrHdr[chBase]);
	sbrFreq = psi->sbrFreq[chBase];

	/* upsample only if we haven't received an SBR header yet or if we have an LFE block */
	if (decoder->id_syn_ele == ID_LFE) {
		chBlock = 1;
		upsampleOnly = 1;
	} else if (decoder->id_syn_ele == ID_FIL) {
		if (decoder->old_id_syn_ele == ID_SCE) 
			chBlock = 1;
		else if (decoder->old_id_syn_ele == ID_CPE)
			chBlock = 2;
		else
			return 0 ;
		
		upsampleOnly = (sbrHdr->count == 0 ? 1 : 0);
		if (decoder->fillExtType != SBR_EXTENSION && decoder->fillExtType != SBR_EXTENSION_CRC)
			return 0 ;
	} else {
		/* ignore non-SBR blocks */
		return 0 ;
	}

	//if(psi->ps_used==0)//if ps is used,the upsample's quality is bad,so do not force it to upsample 
	if((decoder->channelNum!=1&&decoder->sampleRate>24000)||(decoder->forceUpSample))
	{
		upsampleOnly = 1;
	}

	if (upsampleOnly) {
		if(sbrFreq->kStart <= 0 || sbrFreq->kStart > 32)
		{
			sbrFreq->kStart = 32;
			sbrFreq->numQMFBands = 0;
		}
	}

	for (ch = 0; ch < chBlock; ch++) {
		if(!EnableDecodeCurrSBRChannel(decoder,ch))
			continue;
		sbrGrid = &(psi->sbrGrid[chBase + ch]);	
		sbrChan = psi->sbrChan[chBase + ch];

		if (decoder->rawSampleBuf[ch] == 0 || decoder->rawSampleBytes != 4)
			return TT_AACDEC_ERR_AAC_INVSBRSTREAM;
		inbuf = (int *)decoder->rawSampleBuf[ch];
#if SUPPORT_MUL_CHANNEL
		if(decoder->channelNum > 2)//multi channel
		{
			//outbuf += outCh[chOut];
			if(decoder->seletedChs==TT_AUDIO_CHANNEL_ALL)
				outptr = outbuf + outCh[chBase + ch];
			else
			{
				outptr = outbuf + decoder->seletedSBRChDecoded+ch;
				channel_stride = 2;
			}
		}
		else
#endif//SUPPORT_MUL_CHANNEL
		outptr = outbuf + chBase + ch;

		if(psi->delayQMFA[chBase + ch] == NULL)
		{
			psi->delayQMFA[chBase + ch] = (int *)RMAACDecAlignedMalloc(DELAY_SAMPS_QMFA*sizeof(int));
			if(psi->delayQMFA[chBase + ch] == NULL)
				return TTKErrNoMemory;
		}

		if(psi->delayQMFS[chBase + ch] == NULL)
		{
			psi->delayQMFS[chBase + ch] = (int *)RMAACDecAlignedMalloc(DELAY_SAMPS_QMFS*sizeof(int));
			if(psi->delayQMFS[chBase + ch] == NULL)
				return TTKErrNoMemory;
		}
		
		if(psi->XBufDelay[chBase + ch] == NULL)
		{
			psi->XBufDelay[chBase + ch] = (int *)RMAACDecAlignedMalloc(SBR_HF_GEN*128*sizeof(int));
			if(psi->XBufDelay[chBase + ch] == NULL)
				return TTKErrNoMemory;
		}

		/* restore delay buffers (could use ring buffer or keep in temp buffer for channelNum == 1) */
		input = psi->XBufDelay[chBase + ch];
		for (l = 0; l < SBR_HF_GEN; l++) {
			for (k = 0; k < 64; k++) {
				psi->XBuf[l][k][0] = *input++;
				psi->XBuf[l][k][1] = *input++;
			}
		}
		
		/* step 1 - analysis QMF */
		qmfaBands = sbrFreq->kStart;
		for (l = 0; l < 32; l++) {
			gbMask = QMFAnalysis(inbuf + l*32, 
				                 psi->delayQMFA[chBase + ch], 
								 psi->XBuf[l + SBR_HF_GEN][0], 
				                 decoder->rawSampleFBits, 
								 &(psi->delayIdxQMFA[chBase + ch]), 
								 qmfaBands
								 );

			gbIdx = ((l + SBR_HF_GEN) >> 5) & 0x01;	
			sbrChan->gbMask[gbIdx] |= gbMask;	/* gbIdx = (0 if i < 32), (1 if i >= 32) */
		}

		if (upsampleOnly) {
			qmfsBands = 32;
#ifdef PS_DEC
			if(psi->ps_used)
			{
				ps_info *inps = psi->ps;

				channelNum = 2;

				RMAACDecodePS(decoder,psi,sbrGrid,sbrFreq);

				/* step 4 - synthesis QMF */
				for (l = 0; l < 32; l++) {
					QMFSynthesisAfterPS(inps->LBuf[l][0], psi->delayQMFS[chBase + ch], &(psi->delayIdxQMFS[chBase + ch]), qmfsBands, outptr, channelNum);
					outptr += 64*channelNum;
				}
#if SUPPORT_MUL_CHANNEL
				if(decoder->channelNum > 2)//multi channel
				{
					if(decoder->seletedChs==TT_AUDIO_CHANNEL_ALL)
						outptr2 = outbuf + outCh[chBase + ch+1];
					else
						outptr2 = outbuf + decoder->seletedSBRChDecoded+ch+1;//TODO
				}
				else
#endif//SUPPORT_MUL_CHANNEL	
					outptr2 = outbuf + chBase + ch + 1;

				if(psi->delayQMFS[chBase + ch + 1] == NULL)
				{
					psi->delayQMFS[chBase + ch + 1] = (int *)RMAACDecAlignedMalloc(DELAY_SAMPS_QMFS*sizeof(int));
					if(psi->delayQMFS[chBase + ch + 1] == NULL)
						return TTKErrNoMemory;
				}
				
				for (l = 0; l < 32; l++) {
					QMFSynthesisAfterPS(inps->RBuf[l][0], psi->delayQMFS[chBase + ch +1], &(psi->delayIdxQMFS[chBase + ch +1]), qmfsBands, outptr2, channelNum);
					outptr2 += 64*channelNum;
				}
			}
			else
#endif//PS_DEC
			{
				qmfsBands = 32;
				for (l = 0; l < 32; l++) {
					/* step 4 - synthesis QMF */
					QMFSynthesis(psi->XBuf[l + SBR_HF_ADJ][0], 
								 psi->delayQMFS[chBase + ch], 
								 &(psi->delayIdxQMFS[chBase + ch]), 
								 qmfsBands, 
								 outptr, 
								 channel_stride);

					outptr += 64*channel_stride;
				}
			}
			
		} else {
			if(sbrFreq->kStartPrev<sbrFreq->kStart)
			{
				for (l = 0; l < sbrGrid->t_E[0] + SBR_HF_ADJ; l++) {
					for (k = sbrFreq->kStartPrev; k < sbrFreq->kStart; k++) {
						
						psi->XBuf[l][k][0] = 0;
						psi->XBuf[l][k][1] = 0;
					}
				}
			}
			

			/* step 2 - HF generation */
			if((ret = ttHFGen(psi, sbrGrid, sbrFreq, sbrChan)) < 0)
				return -1;
			/* restore SBR bands that were cleared before patch generation (time slots 0, 1 no longer needed) */
			if(sbrFreq->kStartPrev<sbrFreq->kStart)
			{
				
				for (l = SBR_HF_ADJ; l < sbrGrid->t_E[0] + SBR_HF_ADJ; l++) {
					input = psi->XBufDelay[chBase + ch] + l*128 + sbrFreq->kStartPrev*2;
					for (k = sbrFreq->kStartPrev; k < sbrFreq->kStart; k++) {
						
						psi->XBuf[l][k][0] = *input++;
						psi->XBuf[l][k][1] = *input++;
					}
				}
			}			

			/* step 3 - HF adjustment */
			ret |= ttHFAdj(decoder,sbrHdr, sbrGrid, sbrFreq, sbrChan, ch);

#ifdef PS_DEC
			if(psi->ps_used)
			{
				ps_info *inps = psi->ps;
				
				channelNum = 2;
				
				RMAACDecodePS(decoder,psi,sbrGrid,sbrFreq);

				/* step 4 - synthesis QMF */
				for (l = 0; l < 32; l++) {
					QMFSynthesisAfterPS(inps->LBuf[l][0], psi->delayQMFS[chBase + ch], &(psi->delayIdxQMFS[chBase + ch]), qmfsBands, outptr, channelNum);
					outptr += 64*channelNum;
				}
#if SUPPORT_MUL_CHANNEL
				if(decoder->channelNum>2)//multi channel
				{
					//outbuf += outCh[chOut];
					if(decoder->seletedChs==TT_AUDIO_CHANNEL_ALL)
						outptr2 = outbuf + outCh[chBase + ch+1];
					else
						outptr2 = outbuf + decoder->seletedSBRChDecoded+ch+1;//TODO
				}
				else
#endif//SUPPORT_MUL_CHANNEL	
					outptr2 = outbuf + chBase + ch + 1;

				if(psi->delayQMFS[chBase + ch + 1] == NULL)
				{
					psi->delayQMFS[chBase + ch + 1] = (int *)RMAACDecAlignedMalloc(DELAY_SAMPS_QMFS*sizeof(int));
					if(psi->delayQMFS[chBase + ch + 1] == NULL)
						return TTKErrNoMemory;
				}
				
				for (l = 0; l < 32; l++) {
					/* if new envelope starts mid-frame, use old settings until start of first envelope in this frame */
					QMFSynthesisAfterPS(inps->RBuf[l][0], psi->delayQMFS[chBase + ch +1], &(psi->delayIdxQMFS[chBase + ch +1]), qmfsBands, outptr2, channelNum);
					outptr2 += 64*channelNum;
				}
			}
			else
#endif//PS_DEC
			{
				int NeedCopy = (decoder->chSpec==TT_AUDIO_CODEC_CHAN_DUALONE&&decoder->channelNum==1);
				int channelNum = decoder->channelNum;
				if(NeedCopy||channel_stride==2)//TODO
					channelNum = 2;
				/* step 4 - synthesis QMF */
				qmfsBands = sbrFreq->kStartPrev + sbrFreq->numQMFBandsPrev;
				for (l = 0; l < sbrGrid->t_E[0]; l++) {
					/* if new envelope starts mid-frame, use old settings until start of first envelope in this frame */
					QMFSynthesis(psi->XBuf[l + SBR_HF_ADJ][0], 
								 psi->delayQMFS[chBase + ch], 
								 &(psi->delayIdxQMFS[chBase + ch]), 
								 qmfsBands, 
								 outptr, 
								 channelNum
								 );

					if(!NeedCopy)
						outptr += 64*channelNum;
					else
					{
						int count=64;
						do {
							outptr[1]=outptr[0];
							outptr+=2;
						} while(--count>0);
					}					
				}
				
				qmfsBands = sbrFreq->kStart + sbrFreq->numQMFBands;
				for (     ; l < 32; l++) {
					/* use new settings for rest of frame (usually the entire frame, unless the first envelope starts mid-frame) */
					QMFSynthesis(psi->XBuf[l + SBR_HF_ADJ][0], 
								 psi->delayQMFS[chBase + ch], 
								 &(psi->delayIdxQMFS[chBase + ch]), 
								 qmfsBands, 
								 outptr, 
								 channelNum
								 );
				
					if(!NeedCopy)
						outptr += 64*channelNum;
					else
					{
						int count=64;
						do {
							outptr[1]=outptr[0];
							outptr+=2;
						} while(--count>0);
					}
				}
			}
		}

		/* save delay */
		input = psi->XBufDelay[chBase + ch];
		for (l = 0; l < SBR_HF_GEN; l++) {
			for (k = 0; k < 64; k++) {		
				*input++ = psi->XBuf[l+32][k][0];
				*input++ = psi->XBuf[l+32][k][1];
			}
		}
		
		sbrChan->gbMask[0] = sbrChan->gbMask[1];
		sbrChan->gbMask[1] = 0;

		if (sbrHdr->count > 0)
			sbrChan->reset = 0;
	}
	sbrFreq->kStartPrev = sbrFreq->kStart;
	sbrFreq->numQMFBandsPrev = sbrFreq->numQMFBands;

	if (decoder->channelNum > 0 && (chBase + ch) == decoder->channelNum)
		psi->number++;

	return 0 ;
}

#endif
