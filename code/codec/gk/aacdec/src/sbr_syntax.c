
#include "sbr_dec.h"

#ifdef SBR_DEC

int get_sr_index(const int samplerate)
{
	if(samplerate == sampRateTab[0])  return 0;
	if(samplerate == sampRateTab[1])  return 1;
	if(samplerate == sampRateTab[2])  return 2;
	if(samplerate == sampRateTab[3])  return 3;
	if(samplerate == sampRateTab[4])  return 4;
	if(samplerate == sampRateTab[5])  return 5;
	if(samplerate == sampRateTab[6])  return 6;
	if(samplerate == sampRateTab[7])  return 7;
	if(samplerate == sampRateTab[8])  return 8;
	if(samplerate == sampRateTab[9])  return 9;
	if(samplerate == sampRateTab[10]) return 10;
	if(samplerate == sampRateTab[11]) return 11;

	return -1;
}

static int ttGet_MiddleBorder(SBRGrid *hGrid)
{
	int MidBorder = 0;
	unsigned char temp  = hGrid->L_E;
	unsigned char temp1 = hGrid->ptr;

	switch(hGrid->FrameType)
	{
	case ttFIXFIX:
		MidBorder = (temp >> 1);
		break;
	case ttFIXVAR:
		if (temp1 > 1)			
			MidBorder = temp + 1 - temp1;
		else								
			MidBorder = temp - 1;
		break;
	case ttVARFIX:
		if (temp1 == 0)			
			MidBorder = 1;
		else if (temp1 == 1)		
			MidBorder = temp - 1;
		else								
			MidBorder = temp - 1;
		break;

	case ttVARVAR:
		if (temp1 > 1)			
			MidBorder = temp + 1 - temp1;
		else								
			MidBorder = temp - 1;

		break;
	}
	return MidBorder;
}

static int ttExtFrameInfo(AACDecoder* decoder,
						  BitStream *bs, 
						  SBRGrid *pGrid)
{
	int env, rel, pBits, border, middleBorder=0;
	unsigned char relBordLead[TT_MAX_NUM_ENV], relBordTrail[TT_MAX_NUM_ENV];
	int temp;
	unsigned char relBorder0[3], relBorder1[3], relBorder[3];
	unsigned char nL, nR, numRelBorder, numRelLead=0;
	unsigned char absBordLead=0, absBordTrail=0, absBorder;
	sbr_info *ttsbr = (sbr_info *)decoder->sbr;
	

	pGrid->SBR_AmpRes_30 = ttsbr->bs_ampRes;
	pGrid->FrameType = (TTUint8)SBRGetBits(bs, 2);

	switch (pGrid->FrameType) {

	case ttFIXFIX:
		temp = SBRGetBits(bs, ttSBR_ENV_BITS);

		pGrid->L_E = (1 << temp);
		if (pGrid->L_E == 1)
			pGrid->SBR_AmpRes_30 = 0;

#if ERROR_CHECK
		if(!(pGrid->L_E == 1 || pGrid->L_E == 2 || pGrid->L_E == 4))
		{
			return -1;
		}
#endif
		pGrid->L_E= MIN(pGrid->L_E, 4);

		pGrid->freqRes[0] = (TTUint8)SBRGetBits(bs, 1);
		for (env = 1; env < pGrid->L_E; env++)
			 pGrid->freqRes[env] = pGrid->freqRes[0];

		absBordTrail = NUM_TIME_SLOTS;
		numRelLead =   pGrid->L_E - 1;

		/* L_E = 1, 2, or 4 */
		if (pGrid->L_E == 1)		
			border = 16;
		else if (pGrid->L_E == 2)	
			border = 8;
		else							
			border = 4;

		for (rel = 0; rel < numRelLead; rel++)
			relBordLead[rel] = border;

		break;

	case ttFIXVAR:
		absBorder = (unsigned char)SBRGetBits(bs, 2) + NUM_TIME_SLOTS;
		numRelBorder = (unsigned char)SBRGetBits(bs, 2);


		pGrid->L_E = numRelBorder + 1;
		for (rel = 0; rel < numRelBorder; rel++)
			relBorder[rel] = 2*(unsigned char)SBRGetBits(bs, 2) + 2;

		pBits = ttLog2[pGrid->L_E + 1];
		pGrid->ptr = (TTUint8)SBRGetBits(bs, pBits);


		for (env = pGrid->L_E - 1; env >= 0; env--)
			pGrid->freqRes[env] = (TTUint8)SBRGetBits(bs, 1);

		absBordTrail = absBorder;
		numRelLead =   0;

		for (rel = 0; rel < numRelBorder; rel++)
			relBordTrail[rel] = relBorder[rel];

		break;

	case ttVARFIX:
		absBorder = (unsigned char)SBRGetBits(bs, 2);
		numRelBorder = (unsigned char)SBRGetBits(bs, 2);

		pGrid->L_E = numRelBorder + 1;
		for (rel = 0; rel < numRelBorder; rel++)
			relBorder[rel] = 2*(unsigned char)SBRGetBits(bs, 2) + 2;

		pBits = ttLog2[pGrid->L_E + 1];
		pGrid->ptr = (TTUint8)SBRGetBits(bs, pBits);


		for (env = 0; env < pGrid->L_E; env++)
			pGrid->freqRes[env] = (TTUint8)SBRGetBits(bs, 1);

		absBordLead =  absBorder;
		absBordTrail = NUM_TIME_SLOTS;
		numRelLead =   numRelBorder;

		for (rel = 0; rel < numRelLead; rel++)
			relBordLead[rel] = relBorder[rel];

		break;

	case ttVARVAR:
		absBordLead =   SBRGetBits(bs, ttSBR_ABS_BITS);	
		absBordTrail =  SBRGetBits(bs, ttSBR_ABS_BITS) + NUM_TIME_SLOTS;	
		nL = SBRGetBits(bs, ttSBR_NUM_BITS);
		nR = SBRGetBits(bs, ttSBR_NUM_BITS);

		pGrid->L_E = nL + nR + 1;
		pGrid->L_E = MIN(5, pGrid->L_E);

#if ERROR_CHECK
		if(!(pGrid->L_E <= 5))
		{
			//error(decoder,"sbr data error2",
			//		ERR_INVALID_SBR_BITSTREAM);
			return -1;
		}
#endif
		for (rel = 0; rel < nL; rel++)
			relBorder0[rel] = 2*(unsigned char)SBRGetBits(bs, 2) + 2;

		for (rel = 0; rel < nR; rel++)
			relBorder1[rel] = 2*(unsigned char)SBRGetBits(bs, 2) + 2;

		pBits = ttLog2[nL + nR + 2];
		pGrid->ptr = (TTUint8)SBRGetBits(bs, pBits);


		for (env = 0; env < pGrid->L_E; env++)
			pGrid->freqRes[env] = (TTUint8)SBRGetBits(bs, 1);

		numRelLead =  nL;

		for (rel = 0; rel < numRelLead; rel++)
			relBordLead[rel] = relBorder0[rel];

		for (rel = 0; rel < nR; rel++)
			relBordTrail[rel] = relBorder1[rel];

		break;
	}

	/* build time border vector */
	pGrid->t_E[0] = absBordLead * TT_SAMP_PER_SLOT;
	if(pGrid->t_E[0] > 38)
		return -1;

	rel = 0;
	border = absBordLead;
	for (env = 1; env <= numRelLead; env++) {
		border += relBordLead[rel++];
		pGrid->t_E[env] = border * TT_SAMP_PER_SLOT;
		if(pGrid->t_E[env] > 38)
			return -1;
	}

	rel = 0;
	border = absBordTrail;
	for (env = pGrid->L_E - 1; env > numRelLead; env--) {
		border -= relBordTrail[rel++];
		if(border < 0)
			return -1;
		pGrid->t_E[env] = border * TT_SAMP_PER_SLOT;
		if(pGrid->t_E[env] > 38)
			return -1;
	}

	pGrid->t_E[pGrid->L_E] = absBordTrail * TT_SAMP_PER_SLOT;
	if(pGrid->t_E[pGrid->L_E] > 38)
		return -1;

	if (pGrid->L_E > 1) {
		middleBorder = ttGet_MiddleBorder(pGrid);
		pGrid->L_Q = 2;
		pGrid->t_Q[0] = pGrid->t_E[0];
		pGrid->t_Q[1] = pGrid->t_E[middleBorder];
		pGrid->t_Q[2] = pGrid->t_E[pGrid->L_E];
	} else {
		pGrid->L_Q = 1;
		pGrid->t_Q[0] = pGrid->t_E[0];
		pGrid->t_Q[1] = pGrid->t_E[1];
	}

	return 0;
}

/*
* Brief: upack time/freq flags for delta coding of SBR envelops (table 4.63)
*/
static void ttSBR_dtdf(BitStream *hBitBuf,                       /* i: BitStream struct pointing to start of dt/df flags */
					   SBRGrid   *hGrid,						 /* i: grid structure */
					   SBRChan   *hChan                      /* o: chan structure */
					   )
{
	int i;
	int NE = hGrid->L_E;                                   
	int NQ = hGrid->L_Q;                                   
	unsigned char *del_env_flag = hChan->deltaFlagEnv;        
	unsigned char *del_noise_flag = hChan->deltaFlagNoise;    

	for (i = 0; i < NE; i++)
		del_env_flag[i] = SBRGetBits(hBitBuf, 1);

	for (i = 0; i < NQ; i++)
		del_noise_flag[i] = SBRGetBits(hBitBuf, 1);
}

/*
* Brief: unpack invf flags for chirp factor calculation (table 4.64)
*/

static void ttInvf_mode(BitStream *bs,                    /* i: bitstream struct pointing to start of invf flags */              
						int N_Q,                          /* i: number of noise floor bands */
						unsigned char *mode               /* o: invf flags for noise floor bands */
						)
{
	int n;	
	for (n = 0; n < N_Q; n++)
		mode[n] = SBRGetBits(bs, 2);
}

/* 
* Brief: unpack sideband info(grid, delta flags, invf flags, 
*        envelope and noise floor configuration, sinusoids) for a single channel
*/

int ttSBR_Single_Channel_Element(AACDecoder* decoder,
								 BitStream *bs, 
								 int chBase
								 )
{
	int bitsLeft;
	int nIdx;
	int Dext_flg;
	int ext_date_pre; 
	int ext_data_size;

	sbr_info *ttsbr = (sbr_info *)decoder->sbr;
	SBRGrid *pGridL = &(ttsbr->sbrGrid[chBase+0]);
	SBRFreq *sbrFreq =  ttsbr->sbrFreq[chBase];
	SBRChan *sbrChanL = ttsbr->sbrChan[chBase+0];

	Dext_flg = (TTUint8)SBRGetBits(bs, 1);
	if (Dext_flg)
		SBRGetBits(bs, 4);

	if(ttExtFrameInfo(decoder,bs, pGridL) < 0) 
		return -1;

	ttSBR_dtdf(bs, pGridL, sbrChanL);

	ttInvf_mode(bs, sbrFreq->NQ, sbrChanL->invfMode[1]);

	if(ttSBR_Envelope(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
		return -1;
	if(ttSBR_Noise(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
		return -1;

	//UnpackSinusoids function 
	//memset(sbrChanL->addHarmonic[1], 0, 48);
	for (nIdx = 0; nIdx < 48; nIdx ++)
	{
		sbrChanL->addHarmonic[1][nIdx] = 0;
	}
	
	if(SBRGetBits(bs, 1))
	{
		for(nIdx =0; nIdx < sbrFreq->nHigh; nIdx++)
			sbrChanL->addHarmonic[1][nIdx] = SBRGetBits(bs, 1);
	}

	
	ext_date_pre = SBRGetBits(bs, 1);
	if (ext_date_pre) {
		ext_data_size = SBRGetBits(bs, 4);
		if (ext_data_size == 15)
			ext_data_size += SBRGetBits(bs, 8);

		bitsLeft = 8 * ext_data_size;

		/* get ID, unpack extension info, do whatever is necessary with it... */
#ifdef PS_DEC
		if(decoder->disablePS==0)
			RMAACReadPSData(decoder, bs,ttsbr,bitsLeft);
		else
#endif//PS_DEC
		{
			while (bitsLeft> 0) {
				SBRGetBits(bs, 8);//hbfTODO:ref line 406 of sbr_syntax.c of faad 
				bitsLeft-= 8;
			}
		}
	}

	return 0;
}

/* 
* Brief: unpack sideband info(grid, delta flags, invf flags, 
*        envelope and noise floor configuration, sinusoids) for a pair channel
*/

int ttSBR_Channel_Pair_Element(AACDecoder* decoder,
							   BitStream *bs, 
							   int chBase)
{
	int i,Dext_flg;
	int bitsLeft;
	int ext_date_pre;
	int ext_data_size;
	int bs_extension_id;

	sbr_info *ttsbr = (sbr_info *)decoder->sbr;
	SBRGrid *pGridL = &(ttsbr->sbrGrid[chBase+0]);
	SBRGrid *pGridR = &(ttsbr->sbrGrid[chBase+1]);
	SBRFreq *sbrFreq =  ttsbr->sbrFreq[chBase];
	SBRChan *sbrChanL = ttsbr->sbrChan[chBase+0];
	SBRChan *sbrChanR = ttsbr->sbrChan[chBase+1];

	Dext_flg = (TTUint8)SBRGetBits(bs, 1);
	if (Dext_flg) {
		SBRGetBits(bs, SI_SBR_RESERVED_BITS_DATA);       /* reserve bits */
		SBRGetBits(bs, SI_SBR_RESERVED_BITS_DATA);       /* reserve bits */
	}

	ttsbr->couplingFlag = SBRGetBits(bs, 1);
	if (ttsbr->couplingFlag) {
		if(ttExtFrameInfo(decoder,bs, pGridL) < 0)
			return -1;
		
		/* need to copy some data from left to right */
		pGridR->FrameType     =    pGridL->FrameType;
		pGridR->SBR_AmpRes_30 =    pGridL->SBR_AmpRes_30;
		pGridR->ptr           =    pGridL->ptr;
		pGridR->L_E           =    pGridL->L_E;

		for (i = 0; i < pGridL->L_E; i++) {
			pGridR->t_E[i] = pGridL->t_E[i];
			pGridR->freqRes[i] =  pGridL->freqRes[i];
		}
		pGridR->t_E[i] = pGridL->t_E[i];
		pGridR->L_Q = pGridL->L_Q;

		for (i = 0; i <= pGridL->L_Q; i++)
			pGridR->t_Q[i] = pGridL->t_Q[i];


		ttSBR_dtdf(bs, pGridL, sbrChanL);
		ttSBR_dtdf(bs, pGridR, sbrChanR);
		ttInvf_mode(bs, sbrFreq->NQ, sbrChanL->invfMode[1]);

		for (i = 0; i < sbrFreq->NQ; i++)
		{
			sbrChanR->invfMode[1][i] = sbrChanL->invfMode[1][i];
		}
		
		
		if(ttSBR_Envelope(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
			return -1;
		if(ttSBR_Noise(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
			return -1;
		if(ttSBR_Envelope(decoder,bs, pGridR, sbrFreq, sbrChanR, 1) < 0) 
			return -1;
		if(ttSBR_Noise(decoder,bs, pGridR, sbrFreq, sbrChanR, 1) < 0) 
			return -1;

		/* pass RIGHT sbrChan struct */
		UncoupleSBREnvelope(ttsbr, pGridL, sbrFreq, sbrChanR);
		UncoupleSBRNoise(ttsbr, pGridL, sbrFreq, sbrChanR);

	} else {
		if(ttExtFrameInfo(decoder,bs, pGridL) < 0) 
			return -1;

		if(ttExtFrameInfo(decoder,bs, pGridR) < 0) 
			return -1;

		ttSBR_dtdf(bs, pGridL, sbrChanL);
		ttSBR_dtdf(bs, pGridR, sbrChanR);
		ttInvf_mode(bs, sbrFreq->NQ, sbrChanL->invfMode[1]);
		ttInvf_mode(bs, sbrFreq->NQ, sbrChanR->invfMode[1]);

		if(ttSBR_Envelope(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
			return -1;
		if(ttSBR_Envelope(decoder,bs, pGridR, sbrFreq, sbrChanR, 1) < 0) 
			return -1;
		if(ttSBR_Noise(decoder,bs, pGridL, sbrFreq, sbrChanL, 0) < 0) 
			return -1;
		if(ttSBR_Noise(decoder,bs, pGridR, sbrFreq, sbrChanR, 1) < 0) 
			return -1;
	}

	//UnpackSinusoids function 
	//memset(sbrChanL->addHarmonic[1], 0, 48);
	for (i = 0; i < 48; i++)
	{
		sbrChanL->addHarmonic[1][i] = 0;
	}
	
	if(SBRGetBits(bs, 1))
	{
		for(i =0; i < sbrFreq->nHigh; i++)
			sbrChanL->addHarmonic[1][i] = SBRGetBits(bs, 1);
	}

	//UnpackSinusoids function 
	//memset(sbrChanR->addHarmonic[1], 0, 48);
	for (i = 0; i < 48; i++)
	{
		sbrChanR->addHarmonic[1][i] = 0;
	}

	if(SBRGetBits(bs, 1))
	{
		for(i =0; i < sbrFreq->nHigh; i++)
			sbrChanR->addHarmonic[1][i] = SBRGetBits(bs, 1);
	}


	ext_date_pre = SBRGetBits(bs, 1);
	if (ext_date_pre) {
		ext_data_size = SBRGetBits(bs, 4);
		if (ext_data_size == 15)
			ext_data_size += SBRGetBits(bs, 8);
		bitsLeft = 8 * ext_data_size;

		while (bitsLeft > 7)
        {
            bs_extension_id = SBRGetBits(bs, 2);
            bitsLeft -= 2;

			if(bs_extension_id == EXTENSION_ID_PS)
				return -1;

			SBRGetBits(bs, 6);
            bitsLeft -= 6;
        }

		/* get ID, unpack extension info, do whatever is necessary with it... */
		if(bitsLeft) {
			SBRGetBits(bs, bitsLeft);
		}
	}

	ttsbr->ps_used = 0;

	return 0;
}


#endif
