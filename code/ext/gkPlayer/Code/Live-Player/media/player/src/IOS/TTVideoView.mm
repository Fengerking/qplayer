//
//  TTVideoView.m
//

#import <QuartzCore/QuartzCore.h>

#import "TTVideoView.h"

#define MAX_OVERRATE 8.0
#define MIN_OVERRATE 2.2
#define DEFAULT_OVERTURE 2.5

float doubleRectPosition[16] = {-1, 1, -1, -1, 0, 1, 0, -1, 0, 1, 0, -1, 1, 1, 1, -1};
float doubleRectTexCoord[16] = {0.5, 1, 0.5, 0, 0, 1, 0, 0, 0.5, 1, 0.5, 0, 0, 1, 0, 0};
GLushort doubleRectIndice[8] = {0, 1, 2, 3, 4, 5, 6, 7};
int doubleRectVerticeNum = 8;

/*Top and Bottom*/
float doubleRectTopBottomTexCoord[16] = {0, 0, 0, 0.5, 1, 0, 1, 0.5, 0, 0.5, 0, 1, 1, 0.5, 1, 1};

float singleRectPosition[8] = {-1, -1, 1, -1, -1, 1, 1, 1};
float singleRectTexCoord[8] = {0, 1, 1, 1, 0, 0, 1, 0};
GLushort singleRectIndice[4] = {0, 1, 2, 3};
int singleRectVerticeNum = 4;

@implementation TTVideoView

+(Class) layerClass
{
	return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    
    if (nil != self) {
        self.pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinchGesture:)];
        [self addGestureRecognizer:self.pinchRecognizer];
        
        self.overRate = DEFAULT_OVERTURE;
        self.isTouchEnable = false;
        
        self.fingerRotationX = 0;
        self.fingerRotationY = 0;
    }

    return self;
}


- (void)drawRect:(CGRect)rect {
    // Drawing code
}

- (void)setTouchEnable:(bool) bEnable {
    self.isTouchEnable = bEnable;
}


- (void)dealloc {
    [self.pinchRecognizer release];
    [super dealloc];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self.currentTouches addObject:touch];
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    //NSLog(@"move");
    UITouch *touch = [touches anyObject];
    float distX = [touch locationInView:touch.view].x - [touch previousLocationInView:touch.view].x;
    float distY = [touch locationInView:touch.view].y - [touch previousLocationInView:touch.view].y;
    distX *= 0.008;
    distY *= 0.004;
    if(self.isTouchEnable) {
        self.fingerRotationX -= distY;
        self.fingerRotationY -= distX;
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self.currentTouches removeObject:touch];
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        [self.currentTouches removeObject:touch];
    }
}

- (void)handleSingleTapGesture:(UITapGestureRecognizer *)recognizer {
/*    switch (self.currentVideoType) {
        case DoublePano:
            //NSLog(@"DoublePano->SinglePano");
            self.currentVideoType = SinglePano;
            self.squareVertices = singleRectPosition;
            self.textureVertices = singleRectTexCoord;
            self.verticeNum = 8;
            break;
        case SinglePano:
            //NSLog(@"SinglePano->DoublePano");
            self.currentVideoType = DoublePano;
            self.squareVertices = doubleRectPosition;
            self.textureVertices = doubleRectTexCoord;
            self.verticeNum = 16;
            break;
        default:
            //NSLog(@"General");
            self.currentVideoType = General;
            self.squareVertices = singleRectPosition;
            self.textureVertices = singleRectTexCoord;
            self.verticeNum = 8;
            break;
    } */
}

- (void)handlePinchGesture:(UIPinchGestureRecognizer *)recognizer {
    float currentScale = [[recognizer.view.layer valueForKeyPath:@"transform.scale.x"] floatValue];
    float deltaScale = recognizer.scale;
    
    float zoomSpeed = 0.5;
    
    if(deltaScale >= currentScale) {
        zoomSpeed = 0.5;
        self.overRate = ((deltaScale - currentScale) * zoomSpeed) + self.overRate;
    } else {
        zoomSpeed = 0.5;
        self.overRate = ((deltaScale - currentScale) * zoomSpeed) + self.overRate;
    }
    
    self.overRate = MIN(self.overRate, MAX_OVERRATE);
    self.overRate = MAX(self.overRate, MIN_OVERRATE);
}

@end
