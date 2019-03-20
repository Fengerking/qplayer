/*******************************************************************************
	File:		AudioSessionControl.h
 
	Contains:	Audio session controller header file.
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-14		Jun Lin			Create file
 
 ******************************************************************************/

#import <Foundation/Foundation.h>


enum
{
    ASE_INTERRUPTION_BEGIN,
    ASE_INTERRUPTION_END,
    ASE_MEDIA_SERVICE_RESET,
    ASE_APP_RESUME
    
    
}AUDIO_SESSION_EVENT;

typedef void(* asl)(void* userData, int ID, void *param1, void *param2);

typedef struct
{
    asl   	listener;
    void* 	userData;
}ASL;


@interface AudioSessionControl: NSObject
{
    ASL		_asl;
}

-(bool) enableAudioSession:(BOOL)enable force:(BOOL)force;
-(void) setEventCB:(ASL*)cb;
-(int)	getRefCount;
-(void) uninit;
-(void) onASInterrupt:(int)interruptionState;

    
@end
