package com.example.ledemo;

public class LeLib {

	
	public static native boolean entryPoint();
	
	static {
		System.loadLibrary("liblittle-effect");
	}
}
