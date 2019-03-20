#ifndef TDStretch_H
#define TDStretch_H

typedef short SAMPLETYPE;
typedef	long long int64;


/// Default values for sound processing parameters:
/// Notice that the default parameters are tuned for contemporary popular music 
/// processing. For speech processing applications these parameters suit better:
///     #define DEFAULT_SEQUENCE_MS     40
///     #define DEFAULT_SEEKWINDOW_MS   15
///     #define DEFAULT_OVERLAP_MS      8
///

/// Default length of a single processing sequence, in milliseconds. This determines to how 
/// long sequences the original sound is chopped in the time-stretch algorithm.
///
/// The larger this value is, the lesser sequences are used in processing. In principle
/// a bigger value sounds better when slowing down tempo, but worse when increasing tempo
/// and vice versa.
///
/// Increasing this value reduces computational burden & vice versa.
//#define DEFAULT_SEQUENCE_MS         40
#define DEFAULT_SEQUENCE_MS         USE_AUTO_SEQUENCE_LEN

/// Giving this value for the sequence length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEQUENCE_LEN       0

/// Seeking window default length in milliseconds for algorithm that finds the best possible 
/// overlapping location. This determines from how wide window the algorithm may look for an 
/// optimal joining location when mixing the sound sequences back together. 
///
/// The bigger this window setting is, the higher the possibility to find a better mixing
/// position will become, but at the same time large values may cause a "drifting" artifact
/// because consequent sequences will be taken at more uneven intervals.
///
/// If there's a disturbing artifact that sounds as if a constant frequency was drifting 
/// around, try reducing this setting.
///
/// Increasing this value increases computational burden & vice versa.
//#define DEFAULT_SEEKWINDOW_MS       15
#define DEFAULT_SEEKWINDOW_MS       USE_AUTO_SEEKWINDOW_LEN

/// Giving this value for the seek window length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEEKWINDOW_LEN     0

/// Overlap length in milliseconds. When the chopped sound sequences are mixed back together, 
/// to form a continuous sound stream, this parameter defines over how long period the two 
/// consecutive sequences are let to overlap each other. 
///
/// This shouldn't be that critical parameter. If you reduce the DEFAULT_SEQUENCE_MS setting 
/// by a large amount, you might wish to try a smaller value on this.
///
/// Increasing this value increases computational burden & vice versa.
#define DEFAULT_OVERLAP_MS      8


/// Class that does the time-stretch (tempo change) effect for the processed
/// sound.
class TDStretch
{
protected:
    int channels;
    int sampleReq;
    float tempo;

    SAMPLETYPE *pMidBuffer;
    int overlapLength;
    int seekLength;
    int seekWindowLength;
    int overlapDividerBits;
    int slopingDivider;
    float nominalSkip;
    float skipFract;
	SAMPLETYPE* inputBuffer;
	int  nInOffset;
	int  nLastOffset;
	int  nInSample;
	unsigned int  nInMaxSize;

	int* seekTable;

	bool bQuickSeek;

    int sampleRate;
    int sequenceMs;
    int seekWindowMs;
    int overlapMs;
    bool bAutoSeqSetting;
    bool bAutoSeekSetting;

    void acceptNewOverlapLength(int newOverlapLength);

    void calculateOverlapLength(int overlapMs);

    virtual int64 calcCrossCorr(const SAMPLETYPE *mixingPos, const SAMPLETYPE *compare, unsigned int &norm) const;
    virtual int64 calcCrossCorrAccumulate(const SAMPLETYPE *mixingPos, const SAMPLETYPE *compare, unsigned int &norm) const;

    virtual int seekBestOverlapPositionFull(const SAMPLETYPE *refPos);
    virtual int seekBestOverlapPositionQuick(const SAMPLETYPE *refPos);
    int seekBestOverlapPosition(const SAMPLETYPE *refPos);

    virtual void overlapStereo(SAMPLETYPE *output, const SAMPLETYPE *input) const;
    virtual void overlapMono(SAMPLETYPE *output, const SAMPLETYPE *input) const;
    virtual void overlapMulti(SAMPLETYPE *output, const SAMPLETYPE *input) const;

    void clearMidBuffer();
    void overlap(SAMPLETYPE *output, const SAMPLETYPE *input, unsigned int ovlPos) const;

    void calcSeqParameters();

    /// Changes the tempo of the given sound samples.
    /// Returns amount of samples returned in the "output" buffer.
    /// The maximum amount of samples that can be returned at a time is set by
    /// the 'set_returnBuffer_size' function.
    int processSamples(SAMPLETYPE *Outsamples, 
						unsigned int  numOutSamples);
    
public:
    TDStretch();
    virtual ~TDStretch();

	/// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
    /// tempo, larger faster tempo.
    void setTempo(float newTempo);

    /// Returns nonzero if there aren't any samples available for outputting.
    virtual void clear();

    /// Clears the input buffer
    void clearInput();

    /// Sets the number of channels, 1 = mono, 2 = stereo
    void setChannels(int numChannels);

    /// Enables/disables the quick position seeking algorithm. Zero to disable, 
    /// nonzero to enable
    void enableQuickSeek(bool enable);

    /// Returns nonzero if the quick seeking algorithm is enabled.
    bool isQuickSeekEnabled() const;

    /// Sets routine control parameters. These control are certain time constants
    /// defining how the sound is stretched to the desired duration.
    //
    /// 'sampleRate' = sample rate of the sound
    /// 'sequenceMS' = one processing sequence length in milliseconds
    /// 'seekwindowMS' = seeking window length for scanning the best overlapping 
    ///      position
    /// 'overlapMS' = overlapping length
    void setParameters(int sampleRate,          ///< Samplerate of sound being processed (Hz)
                       int sequenceMS = -1,     ///< Single processing sequence length (ms)
                       int seekwindowMS = -1,   ///< Offset seeking window length (ms)
                       int overlapMS = -1       ///< Sequence overlapping length (ms)
                       );

    /// Get routine control parameters, see setParameters() function.
    /// Any of the parameters to this function can be NULL, in such case corresponding parameter
    /// value isn't returned.
    void getParameters(int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs) const;

    /// Adds 'numsamples' pcs of samples from the 'samples' memory position into
    /// the input of the object.
    virtual int process(
            const SAMPLETYPE *Insamples,  
            unsigned int numInSamples,                         
			SAMPLETYPE *Outsamples, 
            unsigned int  numOutSamples                        
            );

	virtual int getLeftBuffer(SAMPLETYPE *Outsamples, 
						unsigned int  numOutSamples);

    /// return nominal input sample requirement for triggering a processing batch
    int getInputSampleReq() const
    {
        return (int)(nominalSkip + 0.5);
    }

    /// return nominal output sample amount when running a processing batch
    int getOutputBatchSize() const
    {
        return seekWindowLength - overlapLength;
    }
};

#endif  /// TDStretch_H
