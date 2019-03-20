package com.example.playerdemo;

import android.os.FileObserver;     
import android.util.Log;     
    
public class SDCardListener extends FileObserver {
	private static final String TAG = "@@@FileObserver";
   
       public SDCardListener(String path) {   
              super(path);     
       }     
     
       @Override    
       public void onEvent(int event, String path) {            
    		 //Log.i(TAG, "File event:" + event + " path:"+ path);
    		 switch(event) {
                     case FileObserver.CREATE:     
                            Log.i(TAG, "File Create " + "path:"+ path);     
                            break;
                     case FileObserver.MODIFY:
                    	 	Log.i(TAG, "File MODIFY " + "path:"+ path);
                    	 	break;
                     case FileObserver.OPEN:
                 	 		Log.i(TAG, "File OPEN " + "path:"+ path);
                 	 		break;	 	
              }     
      }     
}  
