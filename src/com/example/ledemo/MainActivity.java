package com.example.ledemo;


import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity implements LELib.onMelodifyListener{

	Button btn;
	Button btn2;

	static AssetManager assetManager;

	String pathDir;
	Activity activity;
	LELib leLib;
	
	@Override
	public void onMelodifyFinished() {
		Log.e("MainActivity","back success");
	}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        assetManager = getAssets();
        leLib = LELib.getInstance();
        
        init();
    }

	private void init() {
		btn = (Button) findViewById(R.id.btn);
		btn2 = (Button) findViewById(R.id.btn2);

		btn.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View view) {
				leLib.callback();
			}
		});

		btn2.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View view) {

				try {
					leLib.initLE(assetManager);
//					leLib.setMelodifyFile(pathDir + "Speech.mp3", pathDir
//					+ "myvoice.wav", pathDir + "Background.mp3",
//							pathDir + "Melody.mid");
				} catch (Exception e) {
				}
			}
		});

	}

	

}
