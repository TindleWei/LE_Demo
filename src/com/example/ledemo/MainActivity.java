package com.example.ledemo;

import com.example.ledemo.R;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {

	Button btn;

	static boolean isPlayingAsset = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		assetManager = getAssets();
		// initialize native audio system

        createEngine();
        createBufferQueueAudioPlayer();
		
		init();
	}
	
	/** Native methods, implemented in jni folder */
    public static native void createEngine();
    public static native void createBufferQueueAudioPlayer();

	static AssetManager assetManager;

	public static native String modify(AssetManager assetManager,
			String fileName);

	static {
		System.loadLibrary("little-effect");
	}

	private void init() {
		btn = (Button) findViewById(R.id.btn);
		btn.setOnClickListener(new View.OnClickListener() {

			boolean created = false;

			@Override
			public void onClick(View view) {
				
				String a = modify(assetManager, "background.mp3");
				Log.e("FUCK",a);
//				if (!created) {
//					created = createAssetAudioPlayer(assetManager,
//							"background.mp3");
//				}
//				if (created) {
//					isPlayingAsset = !isPlayingAsset;
//					setPlayingAssetAudioPlayer(isPlayingAsset);
//				}
			}
		});
	}

	public static native boolean createAssetAudioPlayer(
			AssetManager assetManager, String filename);

	// true == PLAYING, false == PAUSED
	public static native void setPlayingAssetAudioPlayer(boolean isPlaying);

}
