//
//  CollectionViewCellPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 26/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "CollectionViewCellPlayer.h"
#import "UIPlayerView.h"


@interface CollectionViewCellPlayer()
{
    NSMutableArray* _playerViews;
}

@end

@implementation CollectionViewCellPlayer

-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    
    if(self != nil)
    {
        _playerViews = [[NSMutableArray alloc] init];
        int row = 1;
        int column = 1;
        
        int totalCount = row*column;
        int totalWidth = frame.size.width;
        int totalHeight = frame.size.height;
        
        int left = 0;
        int top = 0;
        int width = totalWidth / column;
        int height = totalHeight / row;
        int offsetWidth = totalWidth % width;
        int offsetHeight = totalHeight % height;
        
        for(int m=0; m<row&&totalCount>0; m++)
        {
            for(int n=0; n<column; n++)
            {
                UIPlayerView* view = [[UIPlayerView alloc] initWithFrame:
                                      CGRectMake(left+n*width, top+m*height,
                                                 (n==(column-1))?width+offsetWidth:width,
                                                 (m==(row-1))?height+offsetHeight:height)];
                if(row*column > 1)
                {
                    view.layer.borderWidth = 1;
                    view.layer.borderColor = [[UIColor orangeColor] CGColor];
                }
                view.backgroundColor = [UIColor blackColor];
                //[self insertSubview:view atIndex:0];
                [self addSubview:view];
                [_playerViews addObject:view];
                
                totalCount--;
                if(totalCount == 0)
                    break;
            }
        }
    }
    return self;
}



-(void)open:(NSString*)url
{
    NSLog(@"\nOpen URL: %@", url);
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
        {
            view.tag = self.tag;
            [view open:url];
        }
    }
}

-(void)run
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view run];
    }
}

-(void)pause
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view pause];
    }
}

-(void)stop
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view stop];
    }
}

-(void)setVolume:(NSInteger)volume
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view setVolume:volume];
    }
}

-(void)setPlayer:(basePlayer*)player
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view setPlayer:player];
    }
}

-(void)preCache:(NSString*)url
{
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view preCache:url];
    }
}


-(void)dealloc
{
    [super dealloc];
    
    for(UIPlayerView* view in _playerViews)
    {
        if(view)
            [view removeFromSuperview];
    }
    
    [_playerViews release];
    _playerViews = nil;
}

@end
