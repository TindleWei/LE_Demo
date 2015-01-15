
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

#define  LOG_TAG    "melodify"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static JavaVM *g_VM;
static jobject gActivity;

using namespace std;

//extern "C" {
//JNIEXPORT void JNICALL Java_com_example_ledemo_LELib_javaCallJNI(JNIEnv* env, jobject obj);
//};

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
		return -1;
	g_VM = vm;

	return JNI_VERSION_1_6;
}

extern "C" void Java_com_example_ledemo_LELib_callback(JNIEnv* env, jobject jobj)
{
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
	gActivity = (jobject)env->NewGlobalRef(clazz);
}

extern "C" void Java_com_example_ledemo_LELib_initLE(JNIEnv* env, jobject jobj, jobject assetManager)
{
	jclass objectClass = (env)->FindClass("com/example/ledemo/LELib");

	if(objectClass == NULL) {
			LOGI("objClass is null");
		} else {
			LOGI("objClass is not null");
		}

	using namespace LE;
	LE::Utility::setAppContext(*g_VM, objectClass, assetManager);

}

extern "C" void Java_com_example_ledemo_LELib_initLE2(JNIEnv* env, jobject jobj, jobject assetManager)
{
	jclass objectClass = (env)->FindClass("com/example/ledemo/LELib");

	if(objectClass == NULL) {
			LOGI("objClass is null");
		} else {
			LOGI("objClass is not null");
		}

	using namespace LE;
	LE::Utility::setAppContext(*g_VM, objectClass, assetManager);

}

extern "C" void Java_com_example_ledemo_LELib_initLE3(JNIEnv* env, jobject jobj, jobject assetManager)
{
	jclass objectClass = (env)->FindClass("com/example/ledemo/LELib");

	if(objectClass == NULL) {
			LOGI("objClass is null");
		} else {
			LOGI("objClass is not null");
		}

	using namespace LE;
	LE::Utility::setAppContext(*g_VM, objectClass, assetManager);

}
