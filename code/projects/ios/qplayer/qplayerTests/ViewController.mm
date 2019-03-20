//
//  ViewController.m
//  qplayerTests
//
//  Created by liang on 12/15/16.
//  Copyright © 2016 pili. All rights reserved.
//

#include "qcIO.h"
#include "qcParser.h"
#import "ViewController.h"

@interface ViewController ()
{
    QC_IO_Func _IO;
    QC_Parser_Func _Parser;
}

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    qcCreateIO(&_IO, QC_IOPROTOCOL_HTTP);
    qcCreateParser(&_Parser, QC_PARSER_FLV);
    _Parser.Open(_Parser.hParser, &_IO, "http://baicizhan.8686c.com/baicizhan.201608012000_19463601_262_20160804195820.flv", 0);
    NSLog(@"test");
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
