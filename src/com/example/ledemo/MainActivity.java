package com.example.ledemo;

import java.io.File;
import java.io.IOException;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {

	Button btn;
	Button btn2;

	static AssetManager assetManager;

	static boolean isPlayingAsset = false;

	String pathDir;
	Activity activity;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		activity = this;

		assetManager = getAssets();

//		createEngine();
//		createBufferQueueAudioPlayer();

		String path = Environment.getExternalStorageDirectory()
				.getAbsolutePath();
		pathDir = path + "/LEAUDIO/";
		
//		File file=new File(pathDir+"myvoice.wav"); 
//		if(!file.exists())
//			try {
//				file.createNewFile();
//			} catch (IOException e) {
//				e.printStackTrace();
//			} 

		setActivity(this);//, assetManager);
		init();
	}
	
	public native void setActivity(Activity mActivity);//, AssetManager mAsset);

//	/** Native methods, implemented in jni folder */
//	public static native void createEngine();
//
//	public static native void createBufferQueueAudioPlayer();
//
//	public static native boolean createAssetAudioPlayer(
//			AssetManager assetManager, String filename);
//
//	public static native void setPlayingAssetAudioPlayer(boolean isPlaying);
//
//	public static native String modify(AssetManager assetManager,
//			String fileName);
//
	public native boolean my(String input, String background,
			String mid, String output);

	static {
		System.loadLibrary("little-effect");
	}

	private void init() {
		btn = (Button) findViewById(R.id.btn);
		btn2 = (Button) findViewById(R.id.btn2);

		btn.setOnClickListener(new View.OnClickListener() {

			boolean created = false;

			@Override
			public void onClick(View view) {

				//String a = modify(assetManager, "background.mp3");

				// if (!created) {
				// created = createAssetAudioPlayer(assetManager,
				// "background.mp3");
				// }
				// if (created) {
				// isPlayingAsset = !isPlayingAsset;
				// setPlayingAssetAudioPlayer(isPlayingAsset);
				// }
			}
		});

		btn2.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View view) {

				try {
					
					
					my(pathDir + "Speech.mp3", pathDir + "Background.mp3",
							pathDir + "Melody.mid", pathDir
									+ "myvoice.wav");
				} catch (Exception e) {
					Log.e("OOOOch", "!!!");
				}
			}
		});

	}

}
