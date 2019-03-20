#import <UIKit/UIKit.h>

@interface TTVideoView : UIView <UIGestureRecognizerDelegate> {

}
@property bool isUsingMotion;
@property bool isTouchEnable;
@property float fingerRotationX;
@property float fingerRotationY;
@property (nonatomic, retain) NSMutableArray *currentTouches;
@property float overRate;
@property UIPinchGestureRecognizer* pinchRecognizer;
/**/
@property GLfloat *squareVertices;
@property GLfloat *textureVertices;
@property GLushort *indices;
@property int verticeNum;
@property bool needsRebind;

- (void)setTouchEnable:(bool) bEnable;

@end
