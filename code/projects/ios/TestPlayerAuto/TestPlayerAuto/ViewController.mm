//
//  ViewController.m
//  TestPlayerAuto
//
//  Created by Jun Lin on 24/04/2018.
//  Copyright © 2018 Jun Lin. All rights reserved.
//

#import "ViewController.h"
#include "CTestMng.h"
#include "CTestAdp.h"

@interface ViewController () <UITextViewDelegate>
{
    CTestMng*	_testMng;
    CTestAdp*	_testAdp;
}
@property (nonatomic, strong) UIView* viewVideo;
@property (nonatomic, strong) UILabel* labelPlayingTime;
@property (nonatomic, strong) UITextView* tvItem;
@property (nonatomic, strong) UITextView* tvFunc;
@property (nonatomic, strong) UITextView* tvMsg;
@property (nonatomic, strong) UITextView* tvErr;
@property (nonatomic, strong) NSMutableArray* testFiles;

@property (nonatomic, retain) UISwipeGestureRecognizer* recognizer;
@end



@implementation ViewController

- (void)setupTestCase
{
    _testFiles = [[NSMutableArray alloc] init];
    
    //
#if 0
    //[_testFiles addObject:@"http://opb95n9bi.bkt.clouddn.com/qplayer/autotest/baseTest.tst"];
    [_testFiles addObject:[[NSBundle mainBundle] pathForResource:@"qaTest" ofType:@"tst"]];
#else
    [_testFiles addObject:[[NSBundle mainBundle] pathForResource:@"iosTest" ofType:@"tst"]];
#endif
    
    //
    _testAdp = new CTestAdp;
    _testAdp->m_viewItem = (__bridge void*)_tvItem;
    _testAdp->m_viewFunc = (__bridge void*)_tvFunc;
    _testAdp->m_viewMsg = (__bridge void*)_tvMsg;
    _testAdp->m_viewErr = (__bridge void*)_tvErr;
    _testAdp->m_viewPlayingTime = (__bridge void*)_labelPlayingTime;
    
    _testMng = new CTestMng;
    _testMng->GetInst()->m_pAdp = _testAdp;
    _testMng->GetInst()->m_hWndVideo = (__bridge void*)_viewVideo;
    CTestPlayer* pPlayer = new CTestPlayer(_testMng->GetInst());
    pPlayer->Create();
    pPlayer->SetView((__bridge void*)_viewVideo, NULL);
    _testMng->SetPlayer(pPlayer);
}

-(void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer
{
    if(recognizer.direction==UISwipeGestureRecognizerDirectionLeft)
    {
        NSLog(@"swipe left");
//        ViewController * player = [[ViewController alloc] init];
//        [self.navigationController pushViewController:player animated:YES];
        if(_testMng)
        	_testMng->OpenTestFile([[_testFiles objectAtIndex:1] UTF8String]);
    }
    
    if(recognizer.direction==UISwipeGestureRecognizerDirectionRight)
    {
        NSLog(@"swipe right");
        //执行程序
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];

    [self setupUI];
    [self setupTestCase];
    
    for(NSString* file in _testFiles)
    	_testMng->AddTestFile([file UTF8String]);
    _testMng->StartTest();
}

- (void)setupUI
{
    _recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
    [_recognizer setDirection:(UISwipeGestureRecognizerDirectionLeft)];
    [self.view addGestureRecognizer:_recognizer];
    
    int infoCount = 3;
    self.view.backgroundColor = [UIColor redColor];
    
    int totalWidth = self.view.frame.size.width;
    int totalHeight = self.view.frame.size.height-infoCount;
    
    // video view
    CGRect rectSmallScreen = self.view.frame;
    rectSmallScreen.size.height = self.view.bounds.size.width*9/16;
    rectSmallScreen.origin.y = 0;
    _viewVideo = [[UIView alloc] initWithFrame:rectSmallScreen];
    _viewVideo.backgroundColor = [UIColor blackColor];
    _viewVideo.contentMode =  UIViewContentModeScaleAspectFit;
    [self.view insertSubview:_viewVideo atIndex:0];
    
    // playing time
    _labelPlayingTime = [[UILabel alloc] initWithFrame:CGRectMake(10, rectSmallScreen.origin.y+rectSmallScreen.size.height - 20, 120, 20)];
    _labelPlayingTime.text = @"00:00:00 / 00:00:00";
    _labelPlayingTime.font = [UIFont systemFontOfSize:8];
    _labelPlayingTime.textColor = [UIColor redColor];
    [_viewVideo addSubview:_labelPlayingTime];
    
    
    //
    CGRect rItem = CGRectMake(0, rectSmallScreen.size.height+1, totalWidth, (totalHeight-rectSmallScreen.size.height)/infoCount);
    self.tvItem = [[UITextView alloc] initWithFrame:rItem];
    self.tvItem.textColor = [UIColor whiteColor];
    self.tvItem.font = [UIFont fontWithName:@"Arial" size:10.0];
    self.tvItem.delegate = self;
    [self.tvItem setEditable:NO];
    self.tvItem.backgroundColor = [UIColor grayColor];
    self.tvItem.text = @"";
    self.tvItem.scrollEnabled = YES;
    self.tvItem.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    self.tvItem.layoutManager.allowsNonContiguousLayout = NO;
    [self.view addSubview: self.tvItem];
    
    //
    CGRect rFunc = CGRectMake(0, rItem.origin.y+rItem.size.height+1, totalWidth, (totalHeight-rectSmallScreen.size.height)/infoCount);
    self.tvFunc = [[UITextView alloc] initWithFrame:rFunc];
    self.tvFunc.textColor = [UIColor whiteColor];
    self.tvFunc.font = [UIFont fontWithName:@"Arial" size:10.0];
    self.tvFunc.delegate = self;
    [self.tvFunc setEditable:NO];
    self.tvFunc.backgroundColor = [UIColor lightGrayColor];
    self.tvFunc.text = @"";
    self.tvFunc.scrollEnabled = YES;
    self.tvFunc.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    self.tvFunc.layoutManager.allowsNonContiguousLayout = NO;
    [self.view addSubview: self.tvFunc];
    
    //
//    CGRect rMsg = CGRectMake(0, rFunc.origin.y+rFunc.size.height+1, totalWidth, (totalHeight-rectSmallScreen.size.height)/infoCount);
//    self.tvMsg = [[UITextView alloc] initWithFrame:rMsg];
//    self.tvMsg.textColor = [UIColor whiteColor];
//    self.tvMsg.font = [UIFont fontWithName:@"Arial" size:10.0];
//    self.tvMsg.delegate = self;
//    [self.tvMsg setEditable:NO];
//    self.tvMsg.backgroundColor = [UIColor grayColor];
//    self.tvMsg.text = @"";
//    self.tvMsg.scrollEnabled = YES;
//    self.tvMsg.autoresizingMask = UIViewAutoresizingFlexibleHeight;
//    self.tvMsg.layoutManager.allowsNonContiguousLayout = NO;
//    [self.view addSubview: self.tvMsg];
    
    //
    CGRect rErr = CGRectMake(0, rFunc.origin.y+rFunc.size.height+1, totalWidth, (totalHeight-rectSmallScreen.size.height)/infoCount);
    self.tvErr = [[UITextView alloc] initWithFrame:rErr];
    self.tvErr.textColor = [UIColor whiteColor];
    self.tvErr.font = [UIFont fontWithName:@"Arial" size:8.0];
    self.tvErr.delegate = self;
    [self.tvErr setEditable:NO];
    self.tvErr.backgroundColor = [UIColor grayColor];
    self.tvErr.text = @"";
    self.tvErr.scrollEnabled = YES;
    self.tvErr.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    self.tvErr.layoutManager.allowsNonContiguousLayout = NO;
    [self.view addSubview: self.tvErr];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)dealloc {
    if(_testMng)
    {
        delete _testMng;
        _testMng = NULL;
    }
    if(_testAdp)
    {
        delete _testAdp;
        _testAdp = NULL;
    }
}


@end
