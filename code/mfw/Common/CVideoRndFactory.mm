/*******************************************************************************
	File:		CVideoRndFactory.mm

	Contains:	The base Video render factory implement code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#include "CVideoRndFactory.h"
#include "COpenGLRnd.h"
#include "COpenGLRndES1.h"
#include "CAudioUnitRnd.h"

CBaseVideoRnd* CVideoRndFactory::Create(CBaseInst* pBaseInst)
{
    return new COpenGLRnd(pBaseInst, NULL);
    //return new COpenGLRndES1(pBaseInst, NULL);
}

void CVideoRndFactory::Destroy(CBaseVideoRnd** ppRender)
{
    if(*ppRender)
    {
        delete *ppRender;
        *ppRender = NULL;
    }
}

CBaseAudioRnd* CAudioRndFactory::Create(CBaseInst* pBaseInst)
{
    return new CAudioUnitRnd(pBaseInst, NULL);
}

void CAudioRndFactory::Destroy(CBaseAudioRnd** ppRender)
{
    if(*ppRender)
    {
        delete *ppRender;
        *ppRender = NULL;
    }
}


