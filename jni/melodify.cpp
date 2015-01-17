#include "le/audioio/device.hpp"
#include "le/audioio/file.hpp"
#include "le/audioio/outputWaveFile.hpp"

#include "le/melodify/melodifyer.hpp"

#include "le/utility/entryPoint.hpp"
#include "le/utility/filesystem.hpp"
#include "le/utility/trace.hpp"
#include "le/utility/sleep.hpp"

#include <jni.h>
#include <android/log.h>
#include <assert.h>
// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <stdio.h>
#include <linux/threads.h>
#include <pthread.h>

#define  LOG_TAG    "Melodify"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static JavaVM *g_VM;
static jobject gActivity;
static jobject gAssetManager;

static pthread_mutex_t thread_mutex;
static pthread_t thread;
static JNIEnv* jniENV;

extern "C" {
void *threadLoop();

void start_thread();
}
;

//extern "C" {
//JNIEXPORT void JNICALL Java_com_example_ledemo_LELib_javaCallJNI(JNIEnv* env, jobject obj);
//};

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	g_VM = vm;
	LOGI("JNI_OnLoad run");
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGI("vm not okay");
		return -1;
	}
	return JNI_VERSION_1_6;
}

extern "C" void Java_com_example_ledemo_LELib_callback(JNIEnv* env,
		jobject jobj) {
	jclass clazz = env->FindClass("com/example/ledemo/LELib");
	if (clazz == 0) {
		LOGI("FindClass error");
		return;
	}
	//	java myCallback
	jmethodID javamethod = env->GetMethodID(clazz, "myCallback", "()V");
	if (javamethod == 0) {
		LOGI("GetMethodID error");
		return;
	}
	env->CallVoidMethod(jobj, javamethod);
	gActivity = (jobject) env->NewGlobalRef(clazz);
}

extern "C" void Java_com_example_ledemo_LELib_initLE(JNIEnv* env, jobject jobj,
		jobject assetManager) {
	LOGI("init method 1");

	jclass objectClass = (env)->FindClass("com/example/ledemo/MainActivity");

	if (objectClass == NULL) {
		LOGI("objClass is null");
	} else {
		LOGI("objClass is not null");
	}

	using namespace LE;
	LE::Utility::setAppContext(*g_VM, objectClass, assetManager);

}

extern "C" void Java_com_example_ledemo_LELib_initLE2(JNIEnv* env, jobject jobj,
		jobject activityObj, jobject assetManager) {
	LOGI("init method 2");

	jniENV = env;


	gActivity = env->NewGlobalRef(activityObj);
	env->GetJavaVM(&g_VM);
	g_VM->AttachCurrentThread(&env, NULL);
	gAssetManager = env->NewGlobalRef(assetManager);

	start_thread();

//	using namespace LE;
//	LE::Utility::setAppContext(*g_VM, gActivity, assetManager);
//
//	LOGI("init method 2 run end");
//	return;
}

extern "C" void Java_com_example_ledemo_LELib_initLE3(JNIEnv* env, jobject jobj,
		jobject activityObj, jobject assetManager) {
	LOGI("init method 3");

	using namespace LE;
	jclass obj_class = env->GetObjectClass(activityObj);

	LE::Utility::setAppContext(*g_VM, obj_class, assetManager);

}

extern "C" void Java_com_example_ledemo_LELib_initLE4(JNIEnv* env, jobject jobj,
		jobject contextObj, jobject assetManager) {
	LOGI("init method 4");

	using namespace LE;
	jclass obj_class = env->GetObjectClass(contextObj);

	LE::Utility::setAppContext(*g_VM, obj_class, assetManager);

}

void *threadLoop(void*) {
	int exiting;
	LOGI("Got JVM:");
	//LOGI("Got JVM: %s", (gotVM ? "false" : "true"));
//	jclass javaClass;
//	jmethodID javaMethodId;
//	int attached = g_VM->AttachCurrentThread(&jniENV, NULL);
//	if (attached > 0) {
//		LOGE("Failed to attach thread to JavaVM");
//		exiting = 1;
//	} else {
//		javaClass = jniENV->FindClass("com/example/ledemo/LELib");
//	}
//	while (!exiting) {
//		pthread_mutex_lock(&thread_mutex);
//		jniENV->CallStaticVoidMethod(javaClass, javaMethodId);
//		pthread_mutex_unlock(&thread_mutex);
//	}
	using namespace LE;
	LE::Utility::setAppContext(*g_VM, gActivity, gAssetManager);
	LOGI("Got 1:");
//	void* retval;
//	pthread_exit(retval);
//	LOGI("Got 2:");
	//detach ..
	g_VM->DetachCurrentThread();
	LOGI("Got 3:");
//	return retval;
}

void start_thread() {
	if (thread < 1) {
		LOGI("start_thread:");
		//pthread_mutex_init(&thread_mutex, NULL);

		pthread_create(&thread, NULL, threadLoop, NULL);

	}
}
