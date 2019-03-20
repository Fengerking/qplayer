/**
 * File : AVRecordControl.h
 * Created on : 2015-12-12
 * Copyright : Copyright (c) 2015 All rights reserved.
 * Description : RecordControl
 */

#ifndef _AVRecordControl_h
#define _AVRecordControl_h
#import <UIKit/UIKit.h>

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1
@interface CAVRecordControl : NSObject
{

}

@property(nonatomic,assign) bool   mIsRunning;

- (id)initWithCameratype:(int) type;
-(void)startcapture;
-(int)stopRecord;
- (void)setPreviewView:(UIView *)previewView;
- (bool) setvideoparameter: (int)width andhi:(int)hight;

- (void) setfilepath:(NSString*)apath;
- (void) setCameraType:(int)type;
- (void) swap;
- (void) opencamera: (UIView *)previewView;
- (void) startRecord;
- (UIImage*)getfirstpic;

- (void) recordpause;
- (void) recordresume;
- (void) resetRecord;
-(void) manualFocus:(GLfloat) x andy:(GLfloat)y;//对焦
-(void) zoomInOut:(GLfloat) level;//
- (float)GetMaxZoomFactor;
-(long) getRecordTime;
-(long) getPeriodTime;
@end


#endif /* RecordControl_h */
