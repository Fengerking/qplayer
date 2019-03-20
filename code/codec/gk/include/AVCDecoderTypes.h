#ifndef AVCDECODERTYPE_H
#define AVCDECODERTYPE_H


typedef struct _TTAVCDecoderSpecificInfo {
	unsigned char *	iData;
	unsigned int	iSize;
	unsigned char *	iConfigData;
	unsigned int	iConfigSize;
	unsigned char *	iSpsData;
	unsigned int	iSpsSize;
	unsigned char *	iPpsData;
	unsigned int	iPpsSize;
}TTAVCDecoderSpecificInfo;


#endif