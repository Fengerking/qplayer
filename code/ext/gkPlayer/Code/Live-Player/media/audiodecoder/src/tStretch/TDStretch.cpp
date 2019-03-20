#include <string.h>
#include "TDStretch.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))


/*****************************************************************************
 *
 * Constant definitions
 *
 *****************************************************************************/

// Table for the hierarchical mixing position seeking algorithm
static const short _scanOffsets[5][24]={
    { 124,  186,  248,  310,  372,  434,  496,  558,  620,  682,  744, 806,
      868,  930,  992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488,   0},
    {-100,  -75,  -50,  -25,   25,   50,   75,  100,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { -20,  -15,  -10,   -5,    5,   10,   15,   20,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {  -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { 121,  114,   97,  114,   98,  105,  108,   32,  104,   99,  117,  111,
      116,  100,  110,  117,  111,  115,    0,    0,    0,    0,    0,   0}};

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/


TDStretch::TDStretch()
{
    bQuickSeek = false;
    channels = 2;

    pMidBuffer = NULL;
    overlapLength = 0;

    bAutoSeqSetting = true;
    bAutoSeekSetting = true;

	inputBuffer = new SAMPLETYPE[320*1024];
	nInMaxSize = 320*1024;

	seekTable = NULL;
	
	nInOffset = 0;
	nLastOffset = 0;
	nInSample = 0;

//    outDebt = 0;
    skipFract = 0;

    tempo = 1.0f;
    setParameters(44100, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
    setTempo(1.0f);

    clear();
}



TDStretch::~TDStretch()
{
    delete[] pMidBuffer;
	delete[] inputBuffer;
	delete[] seekTable;
}



// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping 
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch::setParameters(int aSampleRate, int aSequenceMS, 
                              int aSeekWindowMS, int aOverlapMS)
{
    // accept only positive parameter values - if zero or negative, use old values instead
    if (aSampleRate > 0)   this->sampleRate = aSampleRate;
    if (aOverlapMS > 0)    this->overlapMs = aOverlapMS;

    if (aSequenceMS > 0)
    {
        this->sequenceMs = aSequenceMS;
        bAutoSeqSetting = false;
    } 
    else if (aSequenceMS == 0)
    {
        // if zero, use automatic setting
        bAutoSeqSetting = true;
    }

    if (aSeekWindowMS > 0) 
    {
        this->seekWindowMs = aSeekWindowMS;
        bAutoSeekSetting = false;
    } 
    else if (aSeekWindowMS == 0) 
    {
        // if zero, use automatic setting
        bAutoSeekSetting = true;
    }

    calcSeqParameters();

    calculateOverlapLength(overlapMs);

    // set tempo to recalculate 'sampleReq'
    setTempo(tempo);
}



/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch::getParameters(int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs) const
{
    if (pSampleRate)
    {
        *pSampleRate = sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = (bAutoSeqSetting) ? (USE_AUTO_SEQUENCE_LEN) : sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = (bAutoSeekSetting) ? (USE_AUTO_SEEKWINDOW_LEN) : seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = overlapMs;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapMono(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput) const
{
    int i;
    SAMPLETYPE m1, m2;

    m1 = (SAMPLETYPE)0;
    m2 = (SAMPLETYPE)overlapLength;

    for (i = 0; i < overlapLength ; i ++) 
    {
        pOutput[i] = (pInput[i] * m1 + pMidBuffer[i] * m2 ) >> (overlapDividerBits + 1);
        m1 += 1;
        m2 -= 1;
    }
}



void TDStretch::clearMidBuffer()
{
    memset(pMidBuffer, 0, channels * sizeof(SAMPLETYPE) * overlapLength);
}


void TDStretch::clearInput()
{
	nInOffset = 0;
	nLastOffset = 0;
	nInSample = 0;
    clearMidBuffer();
}


// Clears the sample buffers
void TDStretch::clear()
{
    clearInput();
}



// Enables/disables the quick position seeking algorithm. Zero to disable, nonzero
// to enable
void TDStretch::enableQuickSeek(bool enable)
{
    bQuickSeek = enable;
}


// Returns nonzero if the quick seeking algorithm is enabled.
bool TDStretch::isQuickSeekEnabled() const
{
    return bQuickSeek;
}


// Seeks for the optimal overlap-mixing position.
int TDStretch::seekBestOverlapPosition(const SAMPLETYPE *refPos)
{
    if (bQuickSeek) 
    {
        return seekBestOverlapPositionQuick(refPos);
    } 
    else 
    {
        return seekBestOverlapPositionFull(refPos);
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInputBuffer' at position
// of 'ovlPos'.
inline void TDStretch::overlap(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput, unsigned int ovlPos) const
{
#ifndef USE_MULTICH_ALWAYS
    if (channels == 1)
    {
        // mono sound.
        overlapMono(pOutput, pInput + ovlPos);
    }
    else if (channels == 2)
    {
        // stereo sound
        overlapStereo(pOutput, pInput + 2 * ovlPos);
    } 
    else 
#endif // USE_MULTICH_ALWAYS
    {
        overlapMulti(pOutput, pInput + channels * ovlPos);
    }
}



// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int TDStretch::seekBestOverlapPositionFull(const SAMPLETYPE *refPos) 
{
    int bestOffs;
    int64 bestCorr;
    int i;
    unsigned int norm;

    bestCorr = 0x8000000;
    bestOffs = 0;

    // Scans for the best correlation value by testing each possible position
    // over the permitted range.
    bestCorr = calcCrossCorr(refPos, pMidBuffer, norm);

    for (i = 1; i < seekLength; i ++) 
    {
        int corr;
		int64 corr64;
        // Calculates correlation value for the mixing position corresponding to 'i'
#ifdef _OPENMP
        // in parallel OpenMP mode, can't use norm accumulator version as parallel executor won't
        // iterate the loop in sequential order
        corr = calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
#else
        // In non-parallel version call "calcCrossCorrAccumulate" that is otherwise same
        // as "calcCrossCorr", but saves time by reusing & updating previously stored 
        // "norm" value
        corr = calcCrossCorrAccumulate(refPos + channels * i, pMidBuffer, norm);
#endif
        // heuristic rule to slightly favour values close to mid of the range
        //double tmp = (double)(2 * i - seekLength) / (double)seekLength;
        //corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));

		corr64 = (((int64)(corr + 1)) * (int64)(seekTable[i]));

        // Checks for the highest correlation value
        if (corr64 > bestCorr) 
        {
            // For optimal performance, enter critical section only in case that best value found.
            // in such case repeat 'if' condition as it's possible that parallel execution may have
            // updated the bestCorr value in the mean time
            if (corr64 > bestCorr)
            {
                bestCorr = corr64;
                bestOffs = i;
            }
        }
    }

    return bestOffs;
}


// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int TDStretch::seekBestOverlapPositionQuick(const SAMPLETYPE *refPos) 
{
    int j;
    int bestOffs;
    int64 bestCorr, corr;
    int scanCount, corrOffset, tempOffset;

    bestCorr = 0x8000000;
    bestOffs = _scanOffsets[0][0];
    corrOffset = 0;
    tempOffset = 0;

    // Scans for the best correlation value using four-pass hierarchical search.
    //
    // The look-up table 'scans' has hierarchical position adjusting steps.
    // In first pass the routine searhes for the highest correlation with 
    // relatively coarse steps, then rescans the neighbourhood of the highest
    // correlation with better resolution and so on.
    for (scanCount = 0;scanCount < 4; scanCount ++) 
    {
        j = 0;
        while (_scanOffsets[scanCount][j]) 
        {
            unsigned int norm;
            tempOffset = corrOffset + _scanOffsets[scanCount][j];
            if (tempOffset >= seekLength) break;

            // Calculates correlation value for the mixing position corresponding
            // to 'tempOffset'
            corr = calcCrossCorr(refPos + channels * tempOffset, pMidBuffer, norm);
            // heuristic rule to slightly favour values close to mid of the range
            corr = (((int64)(corr + 1)) * (int64)(seekTable[tempOffset]));

            // Checks for the highest correlation value
            if (corr > bestCorr) 
            {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
            j ++;
        }
        corrOffset = bestOffs;
    }

    return bestOffs;
}


/// Calculates processing sequence length according to tempo setting
void TDStretch::calcSeqParameters()
{
    // Adjust tempo param according to tempo, so that variating processing sequence length is used
    // at varius tempo settings, between the given low...top limits
    #define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
    #define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

    // sequence-ms setting values at above low & top tempo
    #define AUTOSEQ_AT_MIN      125.0
    #define AUTOSEQ_AT_MAX      50.0
    #define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

    // seek-window-ms setting values at above low & top tempo
    #define AUTOSEEK_AT_MIN     25.0
    #define AUTOSEEK_AT_MAX     15.0
    #define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

    #define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))

    double seq, seek;
    
    if (bAutoSeqSetting)
    {
        seq = AUTOSEQ_C + AUTOSEQ_K * tempo;
        seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
        sequenceMs = (int)(seq + 0.5);
    }

    if (bAutoSeekSetting)
    {
        seek = AUTOSEEK_C + AUTOSEEK_K * tempo;
        seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
        seekWindowMs = (int)(seek + 0.5);
    }

    // Update seek window lengths
    seekWindowLength = (sampleRate * sequenceMs) / 1000;
    if (seekWindowLength < 2 * overlapLength) 
    {
        seekWindowLength = 2 * overlapLength;
    }
    seekLength = (sampleRate * seekWindowMs) / 1000;

	if(seekTable != NULL) {
		delete[] seekTable;
		seekTable = NULL;
	}

	seekTable = new int[seekLength];
	for (int i = 0; i < seekLength; i ++) 
    {
        // heuristic rule to slightly favour values close to mid of the range
        double tmp = (double)(2 * i - seekLength) / (double)seekLength;
        double corr = 1.0 - 0.25 * tmp * tmp;

		seekTable[i] = (int)(corr*(1 << 15));
    }
}



// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
// tempo, larger faster tempo.
void TDStretch::setTempo(float newTempo)
{
    int intskip;

    tempo = newTempo;

    // Calculate new sequence duration
    calcSeqParameters();

    // Calculate ideal skip length (according to tempo value) 
    nominalSkip = tempo * (seekWindowLength - overlapLength);
    intskip = (int)(nominalSkip + 0.5f);

    // Calculate how many samples are needed in the 'inputBuffer' to 
    // process another batch of samples
    //sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength / 2;
    sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength;
}



// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch::setChannels(int numChannels)
{
    if (channels == numChannels) return;

    channels = numChannels;
    // re-init overlap/buffer
    overlapLength = 0;
    setParameters(sampleRate);
}


// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
int TDStretch::processSamples(SAMPLETYPE *Outsamples,  unsigned int  numOutSamples)
{
    int ovlSkip, offset;
    int temp;
	int outOffset = 0;

	SAMPLETYPE* inputBuf = inputBuffer + nInOffset*channels;

    // Process samples as long as there are enough samples in 'inputBuffer'
    // to form a processing frame.
    while ((int)nInSample >= sampleReq) 
    {
        // If tempo differs from the normal ('SCALE'), scan for the best overlapping
        // position
        offset = seekBestOverlapPosition(inputBuf);

        // Mix the samples in the 'inputBuffer' at position of 'offset' with the 
        // samples in 'midBuffer' using sliding overlapping
        // ... first partially overlap with the end of the previous sequence
        // (that's in 'midBuffer')
        overlap(Outsamples + outOffset*channels, inputBuf, (unsigned int)offset);
        outOffset += overlapLength;

        // ... then copy sequence samples from 'inputBuffer' to output:

        // length of sequence
        temp = (seekWindowLength - 2 * overlapLength);

        // crosscheck that we don't have buffer overflow...
        if ((int)nInSample < (offset + temp + overlapLength * 2))
        {
            continue;    // just in case, shouldn't really happen
        }

		memcpy(Outsamples + outOffset*channels, inputBuf + channels * (offset + overlapLength), temp*channels*sizeof(SAMPLETYPE));
		outOffset += temp;

		nLastOffset = nInOffset + offset + overlapLength + temp;
		
		// Copies the end of the current sequence from 'inputBuffer' to 
        // 'midBuffer' for being mixed with the beginning of the next 
        // processing sequence and so on
         memcpy(pMidBuffer, inputBuf + channels * (offset + temp + overlapLength), 
            channels * sizeof(SAMPLETYPE) * overlapLength);

        // Remove the processed samples from the input buffer. Update
        // the difference between integer & nominal skip step to 'skipFract'
        // in order to prevent the error from accumulating over time.
        skipFract += nominalSkip;   // real skip size
        ovlSkip = (int)skipFract;   // rounded to integer skip
        skipFract -= ovlSkip;       // maintain the fraction part, i.e. real vs. integer skip
		nInOffset += ovlSkip;
		nInSample -= ovlSkip;
		inputBuf = inputBuffer + nInOffset*channels;
    }

	return outOffset;
}

int TDStretch::getLeftBuffer(SAMPLETYPE *Outsamples, unsigned int  numOutSamples)
{
	int nOut = nInOffset + nInSample - nLastOffset;
	memcpy(Outsamples, inputBuffer + nLastOffset*channels, nOut*channels*sizeof(SAMPLETYPE));
	nInOffset = 0;
	nInSample = 0;
	nLastOffset = 0;
	
	return nOut;
}

// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
int TDStretch::process(const SAMPLETYPE *InSamples, unsigned int nIn, SAMPLETYPE *OutSamples, unsigned int nOut)
{
    // Add the samples into the input buffer
	if((nInOffset + nInSample + nIn)*channels > nInMaxSize){
		memcpy(inputBuffer, inputBuffer + nInOffset*channels, nInSample*channels*sizeof(SAMPLETYPE));
		nLastOffset -= nInOffset;
		if(nLastOffset < 0) {
			nLastOffset = 0;
		}
		nInOffset = 0;
	}
	
	memcpy(inputBuffer + (nInOffset + nInSample)*channels, InSamples, nIn*channels*sizeof(SAMPLETYPE));
	nInSample += nIn;

    // Process the samples in input buffer
   return  processSamples(OutSamples, nOut);
}

/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch::acceptNewOverlapLength(int newOverlapLength)
{
    int prevOvl;
    prevOvl = overlapLength;
    overlapLength = newOverlapLength;

    if (overlapLength > prevOvl)
    {
        delete[] pMidBuffer;
        pMidBuffer = new SAMPLETYPE[overlapLength * channels];

        clearMidBuffer();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Integer arithmetics specific algorithm implementations.
//
//////////////////////////////////////////////////////////////////////////////
// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Stereo' 
// version of the routine.
void TDStretch::overlapStereo(short *poutput, const short *input) const
{
    int i;
    short temp;
    int cnt2;

    for (i = 0; i < overlapLength ; i ++) 
    {
        temp = overlapLength - i;
        cnt2 = 2 * i;
        poutput[cnt2] = (input[cnt2] * i + pMidBuffer[cnt2] * temp )  >> (overlapDividerBits + 1);
        poutput[cnt2 + 1] = (input[cnt2 + 1] * i + pMidBuffer[cnt2 + 1] * temp ) >> (overlapDividerBits + 1);
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Multi'
// version of the routine.
void TDStretch::overlapMulti(SAMPLETYPE *poutput, const SAMPLETYPE *input) const
{
    SAMPLETYPE m1=(SAMPLETYPE)0;
    SAMPLETYPE m2;
    int i=0;

    for (m2 = (SAMPLETYPE)overlapLength; m2; m2 --)
    {
        for (int c = 0; c < channels; c ++)
        {
            poutput[i] = (input[i] * m1 + pMidBuffer[i] * m2)  >> (overlapDividerBits + 1);
            i++;
        }

        m1++;
    }
}

static unsigned int _CLZ( unsigned int x )
{
	int numZeros;
	
	if (!x)
		return 32;
	
	/* count leading zeros with binary search */
	numZeros = 1;
	if (!((unsigned int)x >> 16))	{ numZeros += 16; x <<= 16; }
	if (!((unsigned int)x >> 24))	{ numZeros +=  8; x <<=  8; }
	if (!((unsigned int)x >> 28))	{ numZeros +=  4; x <<=  4; }
	if (!((unsigned int)x >> 30))	{ numZeros +=  2; x <<=  2; }
	
	numZeros -= ((unsigned int)x >> 31);
	
	return numZeros;
}

// Calculates the x having the closest 2^x value for the given value
static int _getClosest2Power(int value)
{
    int n = 0; 
	int tmp = value;

	n = 31 - _CLZ(value);

	if(((value + (1 << (n -1))) >> n) > 1)
		n++;
	
	return n;
}


static unsigned int _Sqrt(unsigned int  in)
{
    unsigned int  tmp;
    unsigned int  out = 0;
    unsigned int  i;
    unsigned int  j;


    if (in == 0) return 0;

    if (in >= 0x10000000)
    {
        out = 0x4000;
        in -= 0x10000000;
    }

    j = 32 - _CLZ(in);

    if (j & 1) j++;
    j >>= 1;

    for (i = j; i > 0; i--) {
        tmp = (out << i) + (1 << ((i - 1)*2));
        if (in >= tmp)
        {
            out += 1 << (i-1);
            in -= tmp;
        }
    }

    return out;
}



/// Calculates overlap period length in samples.
/// Integer version rounds overlap length to closest power of 2
/// for a divide scaling operation.
void TDStretch::calculateOverlapLength(int aoverlapMs)
{
    int newOvl;

    // calculate overlap length so that it's power of 2 - thus it's easy to do
    // integer division by right-shifting. Term "-1" at end is to account for 
    // the extra most significatnt bit left unused in result by signed multiplication 
    overlapDividerBits = _getClosest2Power((sampleRate * aoverlapMs) / 1000) - 1;
    if (overlapDividerBits > 9) overlapDividerBits = 9;
    if (overlapDividerBits < 3) overlapDividerBits = 3;
    newOvl = 1 << (overlapDividerBits + 1);    // +1 => account for -1 above

    acceptNewOverlapLength(newOvl);

    // calculate sloping divider so that crosscorrelation operation won't 
    // overflow 32-bit register. Max. sum of the crosscorrelation sum without 
    // divider would be 2^30*(N^3-N)/3, where N = overlap length
    slopingDivider = (newOvl * newOvl - 1) / 3;
}


int64 TDStretch::calcCrossCorr(const short *mixingPos, const short *compare, unsigned int &norm) const
{
    int64 corr;
    unsigned int lnorm;
    int i;

    corr = lnorm = 0;
    // Same routine for stereo and mono. For stereo, unroll loop for better
    // efficiency and gives slightly better resolution against rounding. 
    // For mono it same routine, just  unrolls loop by factor of 4
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += (mixingPos[i] * compare[i] + 
                 mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBits;  // notice: do intermediate division here to avoid integer overflow
        corr += (mixingPos[i + 2] * compare[i + 2] + 
                 mixingPos[i + 3] * compare[i + 3]) >> overlapDividerBits;
        lnorm += (mixingPos[i] * mixingPos[i] + 
                  mixingPos[i + 1] * mixingPos[i + 1]) >> overlapDividerBits; // notice: do intermediate division here to avoid integer overflow
        lnorm += (mixingPos[i + 2] * mixingPos[i + 2] + 
                  mixingPos[i + 3] * mixingPos[i + 3]) >> overlapDividerBits;
    }

	norm = lnorm;

	return corr*1024 / ((lnorm == 0) ? 1 : _Sqrt(lnorm));
}


/// Update cross-correlation by accumulating "norm" coefficient by previously calculated value
int64 TDStretch::calcCrossCorrAccumulate(const short *mixingPos, const short *compare, unsigned int &norm) const
{
    int64  corr;
    int i;

    // cancel first normalizer tap from previous round
    corr = 0;
    for (i = 1; i <= channels; i ++)
    {
        norm -= (mixingPos[-i] * mixingPos[-i]) >> overlapDividerBits;
    }

    corr = 0;
    // Same routine for stereo and mono. For stereo, unroll loop for better
    // efficiency and gives slightly better resolution against rounding. 
    // For mono it same routine, just  unrolls loop by factor of 4
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += (mixingPos[i] * compare[i] + 
                 mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBits;  // notice: do intermediate division here to avoid integer overflow
        corr += (mixingPos[i + 2] * compare[i + 2] + 
                 mixingPos[i + 3] * compare[i + 3]) >> overlapDividerBits;
    }

    // update normalizer with last samples of this round
    for (int j = 0; j < channels; j ++)
    {
        i --;
        norm += (mixingPos[i] * mixingPos[i]) >> overlapDividerBits;
    }

    // Normalize result by dividing by sqrt(norm) - this step is easiest 
    // done using floating point operation
    return corr*1024 / ((norm == 0) ? 1 : _Sqrt(norm));
}



