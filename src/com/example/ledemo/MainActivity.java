package com.example.ledemo;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity implements LELib.onMelodifyListener,
		View.OnClickListener {

	Button btn;
	Button btn2;
	Button btn3;
	Button btn4;

	AssetManager assetManager;

	Context context;

	Activity activity;

	LELib leLib;

	String pathDir;

	@Override
	public void onMelodifyFinished() {
		Log.e("MainActivity", "back success");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		context = this;
		activity = this;

		assetManager = getAssets();
		leLib = LELib.getInstance();

		init();
	}

	private void init() {
		btn = (Button) findViewById(R.id.btn);
		btn2 = (Button) findViewById(R.id.btn2);
		btn3 = (Button) findViewById(R.id.btn3);
		btn4 = (Button) findViewById(R.id.btn4);

		btn.setOnClickListener(this);
		btn2.setOnClickListener(this);
		btn3.setOnClickListener(this);
		btn4.setOnClickListener(this);

	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn:
			leLib.initLE(assetManager);
			break;

		case R.id.btn2:


			leLib.initLE2(activity, assetManager);
//			new Thread() {
//				@Override
//				public void run() {
//				}
//			}.start();

			break;

		case R.id.btn3:
			leLib.initLE3(activity, assetManager);
			break;

		case R.id.btn4:
			leLib.initLE4(context, assetManager);
			break;
		}

	}

}
