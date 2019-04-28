package com.qiniu.sampleplayer;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.VelocityTracker;
import android.view.ViewConfiguration;
import android.widget.Scroller;

public class MoveSlider extends ViewGroup {
    private int                 mLastX;
    private Scroller            mScroller;
    private VelocityTracker     mVelocityTracker;
    private int                 mMaxVelocity;
    private int                 mCurrentPage = 0;

    public MoveSlider(Context context) {
        super(context);
        init(context);
    }
    public MoveSlider(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }
    public MoveSlider(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    private void init(Context context) {
        mScroller = new Scroller(context);
        ViewConfiguration config = ViewConfiguration.get(context);
        mMaxVelocity = config.getScaledMinimumFlingVelocity();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        int count = getChildCount();
        for(int i = 0; i < count; i++){
            View child = getChildAt(i);
            child.measure(widthMeasureSpec, heightMeasureSpec);
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        int count = getChildCount();
        for(int i = 0; i < count; i++){
            View child = getChildAt(i);
            child.layout(i * getWidth(), t, (i + 1) * getWidth(), b);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        initVelocityTrackerIfNotExists();
        mVelocityTracker.addMovement(ev);
        int x = (int) ev.getX();
        switch (ev.getAction()){
            case MotionEvent.ACTION_DOWN:
                if(!mScroller.isFinished()){
                    mScroller.abortAnimation();
                }
                mLastX = x;
                break;

            case MotionEvent.ACTION_MOVE:
                int dx = mLastX - x;  
                scrollBy(dx,0);
                mLastX = x;
                break;

            case MotionEvent.ACTION_UP:
                final VelocityTracker velocityTracker = mVelocityTracker;
                velocityTracker.computeCurrentVelocity(1000);
                int initVelocity = (int) velocityTracker.getXVelocity();
                if(initVelocity > mMaxVelocity && mCurrentPage > 0){
                     scrollToPage(mCurrentPage - 1);
                }else if(initVelocity < -mMaxVelocity && mCurrentPage < (getChildCount() - 1)){
                    scrollToPage(mCurrentPage + 1);
                }else{
                    slowScrollToPage();
                }
                recycleVelocityTracker();
                break;
        }
        return true;
    }

    private void slowScrollToPage() {
        //当前的偏移位置  
        int scrollX = getScrollX();
        int scrollY = getScrollY();
        //判断是停留在本Page还是往前一个page滑动或者是往后一个page滑动  
        int whichPage = (getScrollX() + getWidth() / 2 ) / getWidth() ;
        scrollToPage(whichPage);
    }

    private void scrollToPage(int indexPage) {
        mCurrentPage = indexPage;
        if(mCurrentPage > getChildCount() - 1){
            mCurrentPage = getChildCount() - 1;
        }

        int dx = mCurrentPage * getWidth() - getScrollX();
        mScroller.startScroll(getScrollX(),0,dx,0,Math.abs(dx) * 2);

        invalidate();
    }

    @Override
    public void computeScroll() {
        super.computeScroll();
        if(mScroller.computeScrollOffset()){
            scrollTo(mScroller.getCurrX(),mScroller.getCurrY());
            invalidate();
        }
    }

    private void recycleVelocityTracker() {
        if (mVelocityTracker != null) {
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }
    }

    private void initVelocityTrackerIfNotExists() {
        if(mVelocityTracker == null){
            mVelocityTracker = VelocityTracker.obtain();
        }
    }
}  
