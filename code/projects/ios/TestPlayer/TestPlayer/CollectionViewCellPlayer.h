//
//  CollectionViewCellPlayer.h
//  TestPlayer
//
//  Created by Jun Lin on 26/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "corePlayer.h"

@interface CollectionViewCellPlayer : UICollectionViewCell
-(void)setPlayer:(basePlayer*)player;
-(void)open:(NSString*)url;
-(void)stop;
-(void)run;
-(void)pause;
-(void)setVolume:(NSInteger)volume;
-(void)preCache:(NSString*)url;
@end
