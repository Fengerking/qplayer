/*******************************************************************************
	File:		CVideoRndFactory.h

	Contains:	The Video render factory header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#ifndef __CVideoRndFactory_H__
#define __CVideoRndFactory_H__

/*
    This factory is just used to support .cpp and .mm compiling together on iOS
    It can't expose COpenGLRnd.h header file in other .cpp, otherwise it will fail while compiling
    Because COpenGLRnd.h contains header files which is related with iOS platform
*/

class CBaseVideoRnd;
class CBaseAudioRnd;
class CBaseInst;

class CVideoRndFactory
{
private:
    CVideoRndFactory();
	virtual ~CVideoRndFactory(void);
    
public:
    static CBaseVideoRnd*   Create(CBaseInst* pBaseInst);
    static void             Destroy(CBaseVideoRnd** ppRender);
};

class CAudioRndFactory
{
private:
    CAudioRndFactory();
    virtual ~CAudioRndFactory(void);
    
public:
    static CBaseAudioRnd*   Create(CBaseInst* pBaseInst);
    static void             Destroy(CBaseAudioRnd** ppRender);
};

#endif // __CVideoRndFactory_H__
