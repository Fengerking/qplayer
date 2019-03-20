#ifndef __HEX_CODING_H__
#define __HEX_CODING_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	int HexEncode(unsigned char*  pInput, int iInputSize, char* pOutput, int* pOutputSize, int iOutputMaxSize);
	int HexDecode(char*  pInput, int iInputSize, char* pOutput, int* pOutputSize, int iOutputMaxSize);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif