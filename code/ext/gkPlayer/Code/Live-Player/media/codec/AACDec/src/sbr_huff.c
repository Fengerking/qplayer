

#include "sbr_dec.h"
#include "sbr_hufftab.h"
#include "sbr_ghdr.h"


#ifdef SBR_DEC

static int ttSBRHuffmanDec(BitStream *bs,
						   int SBRTabIndex
						   )
{
	int nBits, val=0;
	unsigned int bitBuf, Tab, Tab1;
	unsigned int *SBRHuffTab;
	const ttHuffInfo *tthi;

	tthi = &(ttHuffTabSBRInfo[SBRTabIndex]);
	bitBuf = BitStreamShowBits(bs, tthi->max_bits) << (20 - tthi->max_bits);

	SBRHuffTab = tthi->HuffTab_ptr;
	Tab  = *(SBRHuffTab) & 0xfffff;
	Tab1 = *(SBRHuffTab + 1) & 0xfffff;

	if(Tab <= bitBuf && Tab1 > bitBuf)
	{
		nBits = ((*(SBRHuffTab) >> 20) & 0x1F);
		BitStreamSkip(bs, nBits);
		val = (signed int)((*(SBRHuffTab) >> 25) - tthi->off_set);
		return val;
	}

	while(bitBuf > Tab)
	{
		SBRHuffTab++;
		Tab = *(SBRHuffTab) & 0xfffff;
		Tab1 = *(SBRHuffTab + 1) & 0xfffff;

		if(Tab <= bitBuf && Tab1 > bitBuf)
		{
			nBits = ((*(SBRHuffTab) >> 20) & 0x1F);
			BitStreamSkip(bs, nBits);
			val = (signed int)((*(SBRHuffTab) >> 25) - tthi->off_set);
			return val; 
		}
	}
	return val;
}

void UncoupleSBREnvelope(sbr_info *psi,             /* i/o: sbr_info struct */
						 SBRGrid *hGrid,          /* i:   SBRGrid struct for this channel */
						 SBRFreq *hFreq,          /* i:   SBRFreq struct for this SCE/CPE block */  
						 SBRChan *hChanR          /* i:   SBRChan struct for this channel including quantized envelope scalefactors*/
						 )
{
	int i, j, Bnums, scale_val, DE;
	int temp, SF_high, SF_low;

	scale_val = (hGrid->SBR_AmpRes_30 ? 0 : 1);
	SF_high = hFreq->nHigh;
	SF_low  = hFreq->nLow;

	for (i = 0; i < hGrid->L_E; i++) {

		Bnums = (hGrid->freqRes[i] ? SF_high : SF_low);
		psi->envDataDequantScale[1][i] = psi->envDataDequantScale[0][i]; 

		for (j = 0; j < Bnums; j++) {
			DE = hChanR->envDataQuant[i][j] >> scale_val;
			temp = psi->envDataDequant[0][i][j];
			psi->envDataDequant[1][i][j] = TT_Multi31(temp, ttDQTab[24 - DE]) << 1;
			psi->envDataDequant[0][i][j] = TT_Multi31(temp, ttDQTab[DE]) << 1;
		}
	}
}


void UncoupleSBRNoise(sbr_info *psi,             /* i/o: sbr_info struct */
					  SBRGrid *hGrid,            /* i:   SBRGrid struct for this channel */
					  SBRFreq *hFreq,            /* i:   SBRFreq struct for this SCE/CPE block */                           
					  SBRChan *hChanR            /* i:   SBRChan struct for this channel including quantized noise scalefactors*/
					  )
{
	int i, j, DQ;
	int NoiseBand = hFreq->NQ;
	int NFnum     = (int)hGrid->L_Q;
	int temp = 0;

	for (i = 0; i < NFnum; i++) {
		for (j= 0; j < NoiseBand; j++) {
			DQ = hChanR->noiseDataQuant[i][j];
			temp = psi->noiseDataDequant[0][i][j];
			psi->noiseDataDequant[1][i][j] = TT_Multi31(temp, ttDQTab[24 - DQ]) << 1;
			psi->noiseDataDequant[0][i][j] = TT_Multi31(temp, ttDQTab[DQ]) << 1;
		}
	}
}


static void ttDQNoise(int numbs,                      /*  i: number of scalefactors to process */ 
					  signed char *NsQuant,           /*  i: quantized noise scalefactors */
					  int *NsDequant                  /*  o: dequantized noise scalefactors */
					  )
{
	int exp = 0;
	int scale_val = 0;

	for(; numbs > 0; numbs--)
	{
		exp = *NsQuant++;
		scale_val = 30 - exp;
		if(scale_val < 0)
		{
			*NsDequant++ = 0;
		}
		else if(scale_val < 30)
		{
			*NsDequant++ = (1 << scale_val);
		}
		else
		{
			*NsDequant++ = 0x3fffffff;
		}
	}
}

int ttSBR_Noise(AACDecoder* decoder,   /* i:   AAC decoder global info struct */               
				BitStream *bs,                 /* i:   BitStream struct pointing to start of noise data */ 
				SBRGrid *hGrid,                /* i/o: SBRGrid struct for this channel*/ 
				SBRFreq *hFreq,                /* i:   SBRFreq struct for this SCE/CPE block */ 
				SBRChan *hChan,                /* i/o: SBRChan struct for this channel */
				int ch                         /* i:   index of current channel */
				)
{
	int i, j, sf, lastNF;
	int ttHuffTIndex;
	int ttHuffFIndex;
	int delta = 0;
	int cpFlag = 0;
	int N_Q = hFreq->NQ;
	int N_F = hGrid->L_Q;
	sbr_info *psi = (sbr_info *)decoder->sbr;


	cpFlag = psi->couplingFlag;
	if (cpFlag && ch) {
		delta = 1;
		ttHuffTIndex = ttHuffTab_t_Noise30b;
		ttHuffFIndex = ttHuffTab_f_Noise30b;
	} else {
		delta = 0;
		ttHuffTIndex = ttHuffTab_t_Noise30;
		ttHuffFIndex = ttHuffTab_f_Noise30;
	}

	for (i = 0; i < N_F; i++) {

		if (hChan->deltaFlagNoise[i] == 0) {

			hChan->noiseDataQuant[i][0] = SBRGetBits(bs, 5) << delta;
			for (j = 1; j < N_Q; j++) {
				sf = ttSBRHuffmanDec(bs, ttHuffFIndex) << delta;
				hChan->noiseDataQuant[i][j] = sf + hChan->noiseDataQuant[i][j-1];
			}
		} else {
			lastNF = (i == 0 ?  hGrid->numNoiseFloorsPrev-1 : i-1);
			if(lastNF < 0)
				lastNF = 0;  

			for (j = 0; j < N_Q; j++) {
				sf = ttSBRHuffmanDec(bs, ttHuffTIndex) << delta;
				hChan->noiseDataQuant[i][j] = sf + hChan->noiseDataQuant[lastNF][j];
			}
		}

		if ((N_Q > 0) && (cpFlag != 1 || ch != 1))
			ttDQNoise(N_Q, 
			hChan->noiseDataQuant[i], 
			psi->noiseDataDequant[ch][i]);
	}
	hGrid->numNoiseFloorsPrev = N_F;

	return 0;
}

static int ttDQEnvelope(int numbs,                        /*  i: number of scalefactors to process */
						int ampRes,                       /*  i: amplitude resolution flag for this frame */
						signed char *QcEnv,               /*  i: quantized envelope scalefactors */
						int *QdeEnv                       /*  o: dequantized envelop scalefactors */
						)
{
	int exp;
	int MaxTemp = 0;
	int i, temp = 0;
	int scale_value = 0;

	for (i = 0; i < numbs; i++) {
		if (QcEnv[i] > MaxTemp)
			MaxTemp = QcEnv[i];
	}

	if (ampRes) {
		for(; numbs > 0; numbs--)
		{
			exp = *QcEnv++;
			temp = MaxTemp - exp;
			scale_value = MIN(temp, 31);
			*QdeEnv++ = ttEnvDQTab[0] >> scale_value;
		}

		return (6 + MaxTemp);
	} else {
		MaxTemp >>= 1;
		for(; numbs > 0; numbs--)
		{
			exp = *QcEnv++;
			temp = MaxTemp - (exp >> 1);
			scale_value = MIN(temp, 31);
			*QdeEnv++ = ttEnvDQTab[exp & 0x01] >> scale_value;
		}

		return (6 + MaxTemp);
	}
}

int ttSBR_Envelope(AACDecoder* decoder,        /* i:   AAC decoder global info struct */  
				   BitStream *bs,              /* i:   BitStream struct pointing to start of noise data */ 
				   SBRGrid *hGrid,             /* i/o: SBRGrid struct for this channel*/ 
				   SBRFreq *hFreq,             /* i:   SBRFreq struct for this SCE/CPE block */ 
				   SBRChan *hChan,             /* i/o: SBRChan struct for this channel */
				   int ch                      /* i:   index of current channel */
				   )
{
	int ttHuffTIndex;
	int ttHuffFIndex;
	int k, j, StartOffset, Bnum, sf, LastE;
	int cpFlag = 0;
	int freqRes, freqResPrev, dShift, i;
	sbr_info *psi = (sbr_info *)decoder->sbr;
	unsigned char FH_temp, FL_temp;

	cpFlag = psi->couplingFlag;
	if (ch && cpFlag) {
		if (hGrid->SBR_AmpRes_30) {
			ttHuffTIndex = ttHuffTab_t_Env30b;
			ttHuffFIndex = ttHuffTab_f_Env30b;
			StartOffset = 5;
		} else {
			ttHuffTIndex = ttHuffTab_t_Env15b;
			ttHuffFIndex = ttHuffTab_f_Env15b;
			StartOffset = 6;
		}
		dShift = 1;
	} else {
		if (hGrid->SBR_AmpRes_30) {
			ttHuffTIndex = ttHuffTab_t_Env30;
			ttHuffFIndex = ttHuffTab_f_Env30;
			StartOffset = 6;
		} else {
			ttHuffTIndex = ttHuffTab_t_Env15;
			ttHuffFIndex = ttHuffTab_f_Env15;
			StartOffset = 7;
		}
		dShift = 0;
	}

	for (k = 0; k < hGrid->L_E; k++) 
	{
		freqRes =     (hGrid->freqRes[k]);
		Bnum =      (hGrid->freqRes[k] ? hFreq->nHigh : hFreq->nLow);
		freqResPrev = (k == 0 ? hGrid->freqResPrev : hGrid->freqRes[k-1]);
		LastE =     (k == 0 ? hGrid->numEnvPrev-1 : k-1);

		if (hChan->deltaFlagEnv[k] == 0) 
		{
			sf = SBRGetBits(bs, StartOffset) << dShift;
			hChan->envDataQuant[k][0] = sf;
			for (j = 1; j < Bnum; j++) {
				sf = ttSBRHuffmanDec(bs, ttHuffFIndex) << dShift;
				hChan->envDataQuant[k][j] = sf + hChan->envDataQuant[k][j-1];
			}
		} else if (freqRes == 0 && freqResPrev == 1) 
		{
			if(LastE < 0)
				LastE = 0;
			for (j = 0; j < Bnum; j++) {
				sf = ttSBRHuffmanDec(bs, ttHuffTIndex) << dShift;
				hChan->envDataQuant[k][j] = sf;
				FL_temp = hFreq->freqBandLow[j];
				for (i = 0; i < hFreq->nHigh; i++) {
					if (hFreq->freqBandHigh[i] == FL_temp) {
						hChan->envDataQuant[k][j] += hChan->envDataQuant[LastE][i];
						break;
					}
				}
			}
		} else if (freqRes == 1 && freqResPrev == 0) 
		{
			if(LastE < 0)
				LastE = 0;
			for (j = 0; j < Bnum; j++) 
			{
				sf = ttSBRHuffmanDec(bs, ttHuffTIndex) << dShift;
				hChan->envDataQuant[k][j] = sf;
				FH_temp = hFreq->freqBandHigh[j];
				for (i = 0; i < hFreq->nLow; i++) 
				{
					if (hFreq->freqBandLow[i] <= FH_temp && FH_temp < hFreq->freqBandLow[i+1] ) 
					{
						hChan->envDataQuant[k][j] += hChan->envDataQuant[LastE][i];
						break;
					}
				}
			}
		} else if (freqRes == freqResPrev)
		{
			if(LastE < 0)
				LastE = 0;
			for (j = 0; j < Bnum; j++) {
				sf = ttSBRHuffmanDec(bs, ttHuffTIndex) << dShift;
				hChan->envDataQuant[k][j] = sf + hChan->envDataQuant[LastE][j];
			}
		}

		if ((Bnum > 0) && (cpFlag != 1 || ch != 1))
			psi->envDataDequantScale[ch][k] = ttDQEnvelope(Bnum, 
			hGrid->SBR_AmpRes_30, 
			hChan->envDataQuant[k], 
			psi->envDataDequant[ch][k]);
	}

	hGrid->freqResPrev = hGrid->freqRes[hGrid->L_E-1];
	hGrid->numEnvPrev = hGrid->L_E;
	return 0;
}

#endif