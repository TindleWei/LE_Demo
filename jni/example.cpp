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

#define  LOG_TAG    "testjni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static JavaVM *g_VM;
static jobject gActivity;

extern "C" {
JNIEXPORT void JNICALL Java_com_example_ledemo_MainActivity_javaCallJNI(JNIEnv* env, jobject obj);
};

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
		return -1;
	g_VM = vm;
	LOGI("JNI INIT");

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_example_ledemo_MainActivity_javaCallJNI(JNIEnv* env, jobject obj)
{
	LOGI("JNI work !");

	jclass clazz = env->FindClass("com/example/ledemo/MainActivity");
	if (clazz == 0) {
		LOGI("FindClass error");
		return;
	}
	jmethodID javamethod = env->GetMethodID(clazz, "callFromCPP", "()V");
	if (javamethod == 0) {
		LOGI("GetMethodID error");
		return;
	}
	env->CallVoidMethod(obj, javamethod);

	gActivity = (jobject)env->NewGlobalRef(clazz);

}

extern "C" void  Java_com_example_ledemo_MainActivity_setJNI(JNIEnv* env, jobject thiz, jobject assetManager)
{

	using namespace LE;
	LE::Utility::setAppContext(*g_VM, thiz, assetManager);

	LOGI("setContext finish");

}
