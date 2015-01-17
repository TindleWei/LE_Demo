package com.example.ledemo;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;

public class LELib {
	public static LELib le;

	public LELib() {
	}

	public static LELib getInstance() {
		if (le == null)
			le = new LELib();
		return le;
	}

	static {
		System.loadLibrary("little-effect");
	}
	
	public native void initLE(AssetManager assetManager);
	
	public native void initLE2(Activity activity, AssetManager assetManager);
	
	public native void initLE3(Activity activity, AssetManager assetManager);
	
	public native void initLE4(Context context, AssetManager assetManager);
	
	public native void callback();

	
	public void myCallback(){
		listener.onMelodifyFinished();
	}
	
	onMelodifyListener listener;

	public interface onMelodifyListener {
		
		public void onMelodifyFinished();

	}
}


