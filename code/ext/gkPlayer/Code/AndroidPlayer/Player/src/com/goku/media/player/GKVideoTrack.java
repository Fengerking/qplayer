package com.goku.media.player;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.view.Surface;
import android.view.Surface.OutOfResourcesException;
import android.view.ViewGroup.LayoutParams;
import android.view.SurfaceHolder;
import android.util.Log;

/**
 * @(#)GKVideoTrack.java 2011-11-20
 *  yongping.lin
 */

public class GKVideoTrack {
	private final String	TAG = "TTVideoTrack";
	private Surface       	mSurface;
	private Bitmap        	mBitmap;

	private int 	       	mWidthBitmap = 0;
	private int 			mHeightBitmap = 0;
	
	private int 			mViewWidth = 0;
	private int				mViewHeight = 0;
	
	private Rect 			mDst = null;
	
	public GKVideoTrack()
	{
		mSurface = null;
		mBitmap = null;	
		
		mWidthBitmap = 0;
		mHeightBitmap = 0;	
	}			
	
	private void setViewSize(int nWidth, int nHeight)
	{
		mViewWidth = nWidth;
		mViewHeight = nHeight;
	}
	
	private void setSurface (Surface pSurface)
	{
		mSurface = pSurface;
	}
	
	private void updateRect() 
	{
		if(mWidthBitmap == 0 || mHeightBitmap == 0)
			return;
		
		int nMaxOutW = mViewWidth;
		int nMaxOutH = mViewHeight;
		int w = 0, h = 0;
		int offset = 0;

		if (nMaxOutW * mHeightBitmap > mWidthBitmap * nMaxOutH)
		{
			h = nMaxOutH;
			w = nMaxOutH * mWidthBitmap / mHeightBitmap;
			w &= ~0x3;
			h &= ~0x3;
			offset = ((nMaxOutW - w)/2) & ~0x3;
			
			mDst = new Rect(offset, 0, w + offset, h);			
		}
		else
		{
			w = nMaxOutW;
			h = nMaxOutW * mHeightBitmap / mWidthBitmap;
			
			w &= ~0x3;
			h &= ~0x3;
			offset = ((nMaxOutH - h)/2) & ~0x3;
			
			mDst = new Rect(0, offset, w, h + offset);
		}
	}
		
	private int init(int nWidth, int nHeight) 
	{
		Log.v(TAG, "Init " + nWidth + "X" + nHeight);
		
		if (nWidth == 0 || nHeight == 0)
			return -1;
		
		if (mWidthBitmap == nWidth && mHeightBitmap == nHeight)
			return 0;
		
		if (mBitmap != null) 
		{
			mBitmap.recycle();
			mBitmap = null;
		}	
		
		try
		{
			mBitmap = Bitmap.createBitmap(nWidth, nHeight, Bitmap.Config.ARGB_8888);				
			if (mBitmap == null) 
			{
				Log.e(TAG, "Failed to Create Bitmap buffer");
				return -1;
			}
			
			mWidthBitmap  = nWidth;
			mHeightBitmap = nHeight;
	
			updateRect();
		}catch(Exception e)
		{
			Log.e(TAG, "Failed to Create Bitmap buffer on catch! " + e.getMessage());
			return -1;
		}
		
		Log.v(TAG, "Init OK ");
			    
		return 0;			
	}
	
	private int render() 
	{	
		if (mSurface == null || mBitmap == null)
			return -1;
	
		try 
		{		
			Canvas canvas = mSurface.lockCanvas(null);
			if (canvas == null)
			{
				mSurface.unlockCanvasAndPost(canvas);				
				return -1;
			}
			
			canvas.drawBitmap(mBitmap, null, mDst, null); 				
			mSurface.unlockCanvasAndPost(canvas);	
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (OutOfResourcesException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return 0;
	}		
}
