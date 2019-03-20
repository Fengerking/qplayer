#include "struct.h"
#include "decoder.h"
#include "ttAACDec.h"
#include "sbr_dec.h"

static TTInt32 RMAACDecInit(TTHandle * phCodec)
{
	int 	ch;
	TTInt32 nRet = 0;
	AACDecoder *decoder = NULL;
	int* pcoef = NULL;
	int* overlap = NULL;
	int* tmpbuf = NULL;
	unsigned char* start = NULL;

	decoder = (AACDecoder*)RMAACDecAlignedMalloc(sizeof(AACDecoder));
	if(decoder==NULL)
	{
		goto INIT_END;
	}

	tmpbuf = (int*)RMAACDecAlignedMalloc(4*MAX_SAMPLES*sizeof(TTInt32));
	if(tmpbuf == NULL) 
	{
		goto INIT_END;
	}

	decoder->tmpBuffer = (TTInt32 *)tmpbuf;
	pcoef = (int *)RMAACDecAlignedMalloc(MAX_SYNTAX_ELEMENTS*MAX_SAMPLES*sizeof(TTInt32));
	if(pcoef == NULL)
	{
		goto INIT_END;
	}
	for(ch = 0; ch < MAX_SYNTAX_ELEMENTS; ch++)
	{
		decoder->coef[ch] = pcoef + MAX_SAMPLES*ch;
	}

	start = (TTUint8 *)RMAACDecAlignedMalloc(BUFFER_DATA);
	if(start == NULL)
	{
		goto INIT_END;
	}
	decoder->Fstream.start = start;
	decoder->Fstream.maxLength = BUFFER_DATA;
	decoder->Fstream.storelength = 0;
	decoder->Fstream.length = 0;

	decoder->channelNum	= 2;
	decoder->sampleBits = 16;
	decoder->sampleRate	= 44100;
	decoder->profile	= TTAAC_AAC_LC;
	decoder->chSpec	= 0;
	decoder->seletedChs = TT_AUDIO_CHANNEL_ALL;//default,decode all channels if the sample is multichannels
	decoder->chSpec = TT_AUDIO_CODEC_CHAN_MULDOWNMIX2;
	decoder->nFlushFlag = 0;
	decoder->decoderNum = 0;
	decoder->disableSBR = 0;
	decoder->disablePS = 0;
	
	*phCodec = decoder;

	return TTKErrNone;

INIT_END:
	if(decoder) SafeAlignedFree(decoder);
	if(tmpbuf)  SafeAlignedFree(tmpbuf);
	if(pcoef)   SafeAlignedFree(pcoef);
	if(overlap)   SafeAlignedFree(overlap);
	if(start)   SafeAlignedFree(start);

	return TTKErrNoMemory;
}

static TTInt32 RMAACDecSetInputData(TTHandle hCodec, TTBuffer* pInput)
{
	int lenth;
	AACDecoder *decoder;
	FrameStream *fstream;

	decoder = (AACDecoder*)hCodec;
	if(decoder == NULL)
		return TTKErrArgument;

	if(pInput == NULL || pInput->pBuffer == NULL)
		return TTKErrArgument;

	fstream = &decoder->Fstream;
	
	fstream->input = pInput->pBuffer;
	fstream->inlen = pInput->nSize;

	fstream->this_frame = fstream->input;
	fstream->length = fstream->inlen;
	fstream->uselength = 0;


	if(fstream->storelength)
	{
		lenth = MIN(fstream->maxLength - fstream->storelength, fstream->inlen);
		memcpy(fstream->start + fstream->storelength, 
			fstream->input, lenth);

		fstream->this_frame = fstream->start;
		fstream->length = fstream->storelength + lenth;
		fstream->storelength = fstream->length;
	}

	return TTKErrNone;
}

static TTInt32 RMAACDecGetOutputData(TTHandle hCodec, TTBuffer *pOutput, TTAudioFormat *pOutInfo)
{
	int err=0;
	unsigned int length;
	unsigned char* inbuf;
	BitStream *bs;
	AACDecoder *decoder;
	FrameStream *fstream;
	TTAudioFormat *poutAudioFormat;

	decoder = (AACDecoder*)hCodec;
	if(decoder == NULL || pOutput == NULL || pOutput->pBuffer == NULL)
		return TTKErrArgument;

	length = MAX(decoder->seletedChDecoded, 2);
	if(decoder->chSpec == TT_AUDIO_CODEC_CHAN_MULDOWNMIX2)
		length = 2;
	length = length * (decoder->sbrEnabled ? 2 : 1);
	fstream = &decoder->Fstream;
	bs = &(decoder->bs);

	if(pOutput->nSize < length*MAX_SAMPLES*2)
	{
		pOutput->nSize = 0;
		return TTKErrOverflow;
	}

	if(fstream->length == 0)
	{
		return TTKErrUnderflow;
	}
	
	inbuf = fstream->this_frame;
	
	err = DecodeOneFrame(decoder, (TTInt16*)pOutput->pBuffer);	

	//Add for UnitTest
	if (err == (TT_AACDEC_ERR_AUDIO_UNSFEATURE))
	{
		err = TT_AACDEC_ERR_AAC_INVSTREAM;
	}

	if(err == TT_AACDEC_ERR_AAC_INVADTS) 
	{
		if(fstream->storelength) {
			fstream->this_frame = fstream->input;
			fstream->length = fstream->inlen;
			fstream->storelength = 0;
			fstream->uselength = 0;

			err =  DecodeOneFrame(decoder, (TTInt16*)pOutput->pBuffer);
		}

		if(err == TT_AACDEC_ERR_AAC_INVADTS)
		{
			err = TTKErrUnderflow;
		}
	}

	if(decoder->frametype != TTAAC_LATM)
	{
		if(err == TTKErrUnderflow)
		{
			length = fstream->length;		
			if(fstream->storelength == 0)
			{	
				memcpy(fstream->start, fstream->this_frame, length);
				fstream->this_frame = fstream->start;
			}
			fstream->storelength = length;
			fstream->uselength += length;
			pOutput->nSize = 0;

			if(fstream->this_frame != fstream->start)
			{
				memcpy(fstream->start, fstream->this_frame, fstream->length);
			}
			return TTKErrUnderflow;
		}
	}		
	
	if(decoder->frametype != TTAAC_LOAS)
		length =  (CalcBitsUsed(bs, fstream->this_frame, 0) + 7) >> 3;
	else
		length = decoder->frame_length;

	if(fstream->length < length)
	{
		fstream->this_frame += fstream->length;
		fstream->length = 0;
		fstream->uselength += fstream->length;
		
		return TTKErrUnknown;
	}

	if(decoder->frametype == TTAAC_RAWDATA && err)
	{
		length = fstream->length;
	}

	fstream->this_frame += length;
	fstream->length -= length;
	fstream->uselength += length;

	if(fstream->storelength)
	{
		int lenth;
		if(decoder->frametype != TTAAC_LOAS)
			length =  (CalcBitsUsed(bs, inbuf, 0) + 7) >> 3;

		lenth = length - (fstream->storelength - fstream->inlen);
		if(lenth >= 0)
		{
			fstream->this_frame = fstream->input + lenth;
			fstream->length = fstream->inlen - lenth;
			fstream->uselength -= (fstream->storelength  - fstream->inlen);
			fstream->storelength = 0;
		}
		else
		{
			lenth = fstream->storelength - length;
			memcpy(fstream->start, fstream->start + length, lenth);

			fstream->uselength -= length;
			fstream->storelength = lenth;
			fstream->this_frame = fstream->start;
			fstream->length = lenth;			
		}
	}

	if(err) 
	{
		pOutput->nSize = 0;
		return err;
	}

	poutAudioFormat = &decoder->outAudioFormat;		 		
	pOutput->nSize =  poutAudioFormat->Channels * MAX_SAMPLES * (decoder->sbrEnabled ? 2 : 1)*2;

	if(pOutInfo)
	{
		pOutInfo->SampleRate = poutAudioFormat->SampleRate; 
		pOutInfo->Channels = poutAudioFormat->Channels;    
		pOutInfo->SampleBits = 16;			
	}

	decoder->decoderNum++;
	if (decoder->nFlushFlag == 1)
	{
		pOutput->nSize = 0;
		decoder->nFlushFlag = 0;
	}

	return TTKErrNone;
}

TTInt32 RMAACDecSetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	AACDecoder*	decoder = (AACDecoder*)hCodec;
	int i, err, lValue;
	if (decoder==NULL || pData == NULL )//avoid invalid pointer 
		return TTKErrArgument;

	switch(uParamID)
	{
	case TT_PID_AUDIO_DECODER_INFO:
		{	
			TTMP4DecoderSpecificInfo* params = (TTMP4DecoderSpecificInfo*)pData;
			//BitStream bs2;
			BitStream *bs = &decoder->bs;
			int profile,sampIdx,chanNum,sampFreq;
			int length=params->iSize;
			if(params->iSize>1024)
				length = 1024;

			BitStreamInit(bs, length, params->iData);

			if(params->iSize > 4 && IS_ADIFHEADER(params->iData))
			{
				err = ParseADIFHeader(decoder, bs);
				if(err)
					return err;
				break;
			}		

			profile = BitStreamGetBits(bs,5);
			if(profile==31)
			{
				profile = BitStreamGetBits(bs,6);
				profile +=32;
			}

			sampIdx = BitStreamGetBits(bs,4);
			if(sampIdx==0x0f)
			{
				sampFreq = BitStreamGetBits(bs,24);
			}
			else
			{
				if(sampIdx < NUM_SAMPLE_RATES) {
					sampFreq = sampRateTab[sampIdx];
				}
				else
				{
					if(decoder->sampleRate)			
						sampFreq = decoder->sampleRate;
					else
						return TT_AACDEC_ERR_AUDIO_UNSSAMPLERATE;
				}
			}

			chanNum = BitStreamGetBits(bs,4);

			if(chanNum > 2 && chanNum <= 6)
				chanNum = 6;

			if(chanNum <= 0 || chanNum > MAX_CHANNELS)
			{
				if(decoder->channelNum)			
					chanNum = decoder->channelNum;
				else
					return TT_AACDEC_ERR_AUDIO_UNSCHANNEL;
			}
			
			decoder->sampleRate = sampFreq;
			decoder->sampRateIdx = sampIdx;
			decoder->profile = profile;
			decoder->channelNum = chanNum;

			Channelconfig(decoder);
			break;
		}
		
	case TT_AACDEC_PID_PROFILE:
		lValue = *((int *)pData);
		err = updateProfile(decoder, lValue);
		if (err)
		{
			return err;
		}
		break;
	case TT_AACDEC_PID_FRAMETYPE:
		lValue = *((int *)pData);
		if(lValue < TTAAC_RAWDATA&& lValue > TTAAC_LOAS)
			return TT_AACDEC_ERR_AAC_UNSPROFILE;
		decoder->frametype = lValue;
		break;
	case TT_AACDEC_PID_CHANNELSPEC:
		lValue = *((int *)pData);
		decoder->chSpec = lValue;
		if(lValue == TT_AUDIO_CODEC_CHAN_DUALLEFT)
			decoder->seletedChs = TT_AUDIO_CHANNEL_FRONT_LEFT;
		else if(lValue == TT_AUDIO_CODEC_CHAN_DUALRIGHT)
			decoder->seletedChs = TT_AUDIO_CHANNEL_FRONT_RIGHT;
		else
			decoder->seletedChs = TT_AUDIO_CHANNEL_ALL;
		break;
	case TT_PID_AUDIO_CHANNELS:
		lValue = *((int *)pData);
		if(lValue<=0||lValue>MAX_CHANNELS)
			return TT_AACDEC_ERR_AUDIO_UNSCHANNEL;
		decoder->channelNum = lValue;
		break;
	case TT_PID_AUDIO_SAMPLEREATE:
		lValue = *((int *)pData);
        err = updateSampleRate(decoder, lValue);
		if(err)
		{
			return err;
		}
		break;
	case TT_AACDEC_PID_DISABLEAACPLUSV1:
		lValue = *((int *)pData);
		decoder->disableSBR = lValue;
		break;
	case TT_AACDEC_PID_DISABLEAACPLUSV2:
		lValue = *((int *)pData);
		decoder->disablePS = lValue;
		break;
	case TT_AACDEC_PID_SELECTCHS:
		lValue = *((int *)pData);
		decoder->seletedChs = lValue;
		break;
	case TT_PID_AUDIO_FLUSH:
		lValue = *((int *)pData);
		for(i=0;i<decoder->channelNum;i++)
		{
			if(i>=MAX_CHANNELS)//in error bitstream,it may appear
				break;
			if(decoder->overlap[i])
			{
				memset(decoder->overlap[i], 0, MAX_SAMPLES*sizeof(int));
			}
		}	

		decoder->Fstream.storelength = 0;
		decoder->Fstream.inlen = 0;
		decoder->Fstream.length = 0;
		decoder->decoderNum = 0;

		decoder->nFlushFlag = 1;  

#ifdef SBR_DEC		
		if(decoder->sbr)
			ReSetSBRDate(decoder->sbr);
#endif

		break;
	case TT_PID_AUDIO_FORMAT:
		{
			TTAudioFormat* fmt = (TTAudioFormat*)pData;
			if(fmt->SampleRate!=0)
			{
				decoder->sampleRate = fmt->SampleRate;
			}

			if(fmt->Channels > 0 && fmt->Channels <= MAX_CHANNELS)
			{
				decoder->channelNum = fmt->Channels;
			}

			if(fmt->SampleBits == 16 || fmt->SampleBits == 32)
			{
				decoder->sampleBits = fmt->SampleBits;
			}

			break;
		}
	default:
		return TTKErrArgument;
	}

	return TTKErrNone;
}

static TTInt32 RMAACDecGetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	AACDecoder* decoder = (AACDecoder*)hCodec;
#ifdef WIN32
	int err = 0;
#endif
	if(decoder==NULL || pData == NULL)
		return TTKErrArgument;

	switch(uParamID)
	{
	case TT_PID_AUDIO_FORMAT:
		{
			TTAudioFormat* fmt = (TTAudioFormat*)pData;
			if(decoder->outAudioFormat.SampleRate != 0) {
				fmt->Channels = decoder->outAudioFormat.Channels;
				fmt->SampleRate = decoder->outAudioFormat.SampleRate;
				fmt->SampleBits = 16;
			} else if(decoder->sampleRate!=0)
			{
				fmt->Channels = decoder->channelNum;
				fmt->SampleRate = decoder->sampleRate * (decoder->sbrEnabled ? 2 : 1);
				fmt->SampleBits = 16;
			}
			else
			{
				return TTKErrNotReady;
			}
			break;
		}	
	case TT_AACDEC_PID_PROFILE:	
		*((int *)pData) =  decoder->profile;
		break;
	case TT_AACDEC_PID_FRAMETYPE:	
		*((int *)pData) =  decoder->frametype;
		break;
	case TT_AACDEC_PID_CHANNELSPEC:
		*((int *)pData) = decoder->chSpec;
		break;
	case TT_PID_AUDIO_CHANNELS:
		*((int *)pData) = decoder->channelNum;
		break;
	case TT_PID_AUDIO_SAMPLEREATE:
		*((int *)pData) = decoder->sampleRate * (decoder->sbrEnabled ? 2 : 1);
		break;
	case TT_AACDEC_PID_CHANNELMODE:
		*((int *)pData) = decoder->ChansMode;
		break;
	case TT_AACDEC_PID_CHANNELPOSTION:
		pData = (TTPtr)decoder->channel_position;
		break;
	default:
		return TTKErrArgument;
	}

	return TTKErrNone;
}

static TTInt32 RMAACDecUninit(TTHandle hCodec)
{
	AACDecoder* decoder = (AACDecoder*)hCodec;
	int i;

	if (decoder == NULL)
	{
		return TTKErrArgument;
	}
	
	if(decoder)
	{
#ifdef SBR_DEC
		sbr_free(decoder);
#endif

#ifndef _SYMBIAN_
		for(i=0;i<MAX_CHANNELS;i++)
		{
			if(decoder->overlap[i])
			{
				SafeAlignedFree(decoder->overlap[i]);
			}
		}
#else
		if(decoder->overlap[0])
		{
			SafeAlignedFree(decoder->overlap[0]);
		}
		decoder->overlap[1] = NULL;
#endif

		if(decoder->latm)
		{
			SafeAlignedFree(decoder->latm);
		}

#ifdef LTP_DEC
		for(i = 0; i  < MAX_CHANNELS; i++)
		{
			if(decoder->ltp_coef[i])
			{
				SafeAlignedFree(decoder->ltp_coef[i]);
			}
		}

		if(decoder->t_est_buf)
			SafeAlignedFree(decoder->t_est_buf);
		if(decoder->f_est_buf)
			SafeAlignedFree(decoder->f_est_buf);
#endif

#ifdef MAIN_DEC
		for(i = 0; i  < MAX_CHANNELS; i++)
		{
			if(decoder->pred_stat[i])
			{
				SafeAlignedFree(decoder->pred_stat[i]);
			}
		}
#endif


		SafeAlignedFree(decoder->coef[0]);
		for(i = 0; i  < MAX_SYNTAX_ELEMENTS; i++)
		{
			decoder->coef[i] = NULL;
		}

		if(decoder->tmpBuffer)
			SafeAlignedFree(decoder->tmpBuffer);
		if(decoder->bsac)
			SafeAlignedFree(decoder->bsac);
		if(decoder->Fstream.start)
		{
			SafeAlignedFree(decoder->Fstream.start);
			decoder->Fstream.length = 0;
			decoder->Fstream.maxLength = 0;
		}

		SafeAlignedFree(decoder);
	}


	return TTKErrNone;
}

TTInt32 ttGetAACDecAPI (TTAudioCodecAPI* pDecHandle)
{
	if(pDecHandle == NULL)
		return TTKErrArgument;
		
	pDecHandle->Open = RMAACDecInit;
	pDecHandle->SetInput = RMAACDecSetInputData;
	pDecHandle->Process = RMAACDecGetOutputData;
	pDecHandle->SetParam = RMAACDecSetParam;
	pDecHandle->GetParam = RMAACDecGetParam;
	pDecHandle->Close = RMAACDecUninit;

	return TTKErrNone;
}
