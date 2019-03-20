package com.ali.music.media.cameracollection;

import com.ali.music.media.cameracollection.CameraActivity.StopListener;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

public class StartActivity extends Activity {

	private Button btnH264;
	private Button btnHEVC;

	EditText 	widthEdit;
	EditText 	HeightEdit;
	EditText 	bitrateEdit;

	private int	   nWidth = 1280;
	private int    nHeight = 720;
	private int	   nBitrate = 12000;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_start);
		
		btnH264 = (Button)findViewById(R.id.button21);
		btnH264.setOnClickListener(new openListenerH264());
		
		btnHEVC = (Button)findViewById(R.id.Button01);
		btnHEVC.setOnClickListener(new openListenerHEVC());

		widthEdit = (EditText)findViewById(R.id.nWidthNum);
		HeightEdit = (EditText)findViewById(R.id.nHeightNum);
		bitrateEdit = (EditText)findViewById(R.id.nBitrateNum);
	}
	
	class openListenerH264 implements OnClickListener {
		public void onClick(View v) {
			Intent intent = new Intent(StartActivity.this, CameraActivity.class);
			
			//��BundleЯ������
		    Bundle bundle=new Bundle();
		    //����name����Ϊtinyphp

			String widthstr = widthEdit.getText().toString();
			nWidth = Integer.parseInt(widthstr);
			String Heightstr = HeightEdit.getText().toString();
			nHeight = Integer.parseInt(Heightstr);
			String bitratestr = bitrateEdit.getText().toString();
			nBitrate = Integer.parseInt(bitratestr);


		    bundle.putString("codec", "H264");
			bundle.putInt("width", nWidth);
			bundle.putInt("height", nHeight);
			bundle.putInt("bitrate", nBitrate);
		    intent.putExtras(bundle);
			
			startActivity(intent);
		}
	};
	
	class openListenerHEVC implements OnClickListener {
		public void onClick(View v) {
			Intent intent = new Intent(StartActivity.this, CameraActivity.class);
			
			//��BundleЯ������
		    Bundle bundle=new Bundle();
		    //����name����Ϊtinyphp

			String widthstr = widthEdit.getText().toString();
			nWidth = Integer.parseInt(widthstr);
			String Heightstr = HeightEdit.getText().toString();
			nHeight = Integer.parseInt(Heightstr);
			String bitratestr = bitrateEdit.getText().toString();
			nBitrate = Integer.parseInt(bitratestr);

		    bundle.putString("codec", "HEVC");
			bundle.putInt("width", nWidth);
			bundle.putInt("height", nHeight);
			bundle.putInt("bitrate", nBitrate);
		    intent.putExtras(bundle);
			
			startActivity(intent);
		}
	};

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.start, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
}
