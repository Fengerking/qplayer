//
//  ViewControllerLive.m
//  TestPlayer
//
//  Created by Jun Lin on 07/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "ViewControllerPlayerViews.h"
#import "CollectionViewCellPlayer.h"
#import "corePlayer.h"
#import "UIPlayerView.h"

static NSString *const liveCellId = @"liveCellId";
static NSString *const liveCellFooterId = @"liveCellFooterId";
static NSString *const liveCellHeaderId = @"liveCellHeaderId";

@interface ViewControllerPlayerViews ()
{
    UICollectionView*	_collectionView;
    UICollectionViewFlowLayout*	_layout;
    
    //UITapGestureRecognizer*		_singleTapTwo;
    CollectionViewCellPlayer* 	_currCell;
    CollectionViewCellPlayer* 	_lastCell;
}
@end

@implementation ViewControllerPlayerViews

- (void)viewDidLoad
{
    _lastCell = nil;
    _currCell = nil;
    
    //
    _layout = [[UICollectionViewFlowLayout alloc] init];
    _layout.minimumLineSpacing = 0;
    _layout.itemSize = CGSizeMake(self.view.frame.size.width, self.view.frame.size.height);
    _layout.scrollDirection = UICollectionViewScrollDirectionVertical;
    
    //
    _collectionView = [[UICollectionView alloc]initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height) collectionViewLayout:_layout];
    _collectionView.backgroundColor = [UIColor blackColor];
    _collectionView.delegate = self;
    _collectionView.dataSource = self;
    _collectionView.scrollsToTop = NO;
    _collectionView.showsVerticalScrollIndicator = NO;
    _collectionView.showsHorizontalScrollIndicator = NO;
    _collectionView.pagingEnabled = YES;
    _collectionView.userInteractionEnabled = YES;
    [_collectionView registerClass:[CollectionViewCellPlayer class] forCellWithReuseIdentifier:liveCellId];
    [_collectionView registerClass:[UICollectionReusableView class] forSupplementaryViewOfKind:UICollectionElementKindSectionHeader withReuseIdentifier:liveCellHeaderId];
    [_collectionView registerClass:[UICollectionReusableView class] forSupplementaryViewOfKind:UICollectionElementKindSectionFooter withReuseIdentifier:liveCellFooterId];
    [self.view addSubview:_collectionView];
    [_collectionView release];
    
    [super viewDidLoad];
    
    
    //
//    _singleTapTwo = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
//    _singleTapTwo.numberOfTouchesRequired = 2;
//    _singleTapTwo.numberOfTapsRequired = 1;
//    _singleTapTwo.delegate = self;
}

//- (void)handleSingleTap:(UITapGestureRecognizer *)sender
//{
//    if (sender.numberOfTouchesRequired == 1) {
//        NSLog(@"Single Tap with a finger.");
//    }
//    else if (sender.numberOfTouchesRequired == 2) {
//        NSLog(@"Single Tap with two finger.");
//    }
//}



-(void)stop
{
    if(_currCell)
        [_currCell stop];
    if(_singleView)
        [_singleView stop];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return [_urlList count];
}

- (void)collectionView:(UICollectionView *)collectionView didEndDisplayingCell:(UICollectionViewCell *)cell forItemAtIndexPath:(NSIndexPath *)indexPath
{
    // use multiple instance, so stop the last playback
    if(!_corePlayer)
    {
        NSLog(@"[K]+Stop");
        int time = [basePlayer getSysTime];
        [(CollectionViewCellPlayer*)cell stop];
        NSLog(@"[K]-Stop %ld, %p, use time %d", indexPath.row, cell, (int)([basePlayer getSysTime] - time));
    }
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
    //NSLog(@"[K]1 scrollViewWillBeginDragging");
}

- ( UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    CollectionViewCellPlayer* cell = [collectionView dequeueReusableCellWithReuseIdentifier:liveCellId forIndexPath:indexPath];
    [cell setBackgroundColor:indexPath.row%2?[UIColor redColor]:[UIColor blueColor]];
    cell.tag = indexPath.row;
    cell.userInteractionEnabled = YES;
    //[_currCell setPlayer:_corePlayer];
    //[cell preCache:[_urlList objectAtIndex:indexPath.row]];
    return cell;
}

- (void)collectionView:(UICollectionView *)collectionView willDisplayCell:(UICollectionViewCell *)cell forItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(_currCell)
    {
        _lastCell = _currCell;
    }
    
    _currCell = (CollectionViewCellPlayer*)cell;
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint *)targetContentOffset
{
    if(_corePlayer) // 使用单实例player的话需要stop,因为view发生了变化
    {
        if(_lastCell)
        {
            [_lastCell stop];
        }
    }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate
{
    NSLog(@"[K]+scrollViewDidEndDragging");
    if(_currCell)
    {
        [_currCell setPlayer:_corePlayer];
        NSLog(@"[K]-scrollViewDidEndDragging 0");
        int time = [basePlayer getSysTime];
        [_currCell open:[_urlList objectAtIndex:_currCell.tag]];
        NSLog(@"[K]-scrollViewDidEndDragging 1");
        NSLog(@"[K]Open %ld, %p, use time %d", _currCell.tag, _currCell, (int)([basePlayer getSysTime] - time));
    }
    NSLog(@"[K]-scrollViewDidEndDragging");
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    //NSLog(@"[K]scrollViewDidScroll");
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldShowMenuForItemAtIndexPath:(NSIndexPath *)indexPath
{
    return YES;
}

- (BOOL)collectionView:(UICollectionView *)collectionView canPerformAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(nullable id)sender
{
    if([NSStringFromSelector(action) isEqualToString:@"cut:"])
        return NO;
    return YES;
}

- (void)collectionView:(UICollectionView *)collectionView performAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(nullable id)sender
{
    if([NSStringFromSelector(action) isEqualToString:@"copy:"])
    {
        [UIPasteboard generalPasteboard].string = [_urlList objectAtIndex:_currCell.tag];
        if(_singleView)
            [_singleView setHidden:NO];
    }
    else if([NSStringFromSelector(action) isEqualToString:@"paste:"])
    {
        if(_singleView && ![_singleView isHidden])
        {
            [_singleView open:[UIPasteboard generalPasteboard].string];
            //[_singleView run];
        }
        else
        {
            if(_currCell)
            {
                [_currCell setPlayer:_corePlayer];
                [_currCell stop];
                [_currCell open:[UIPasteboard generalPasteboard].string];
                //[_currCell run];
            }
        }
    }
}


-(void)dealloc
{
    if(_layout)
    {
        [_layout release];
        _layout = nil;
    }
    if(_currCell)
    {
        [_currCell release];
        _currCell = nil;
    }
    
    [super dealloc];
}

@end
