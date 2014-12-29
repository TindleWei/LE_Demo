////////////////////////////////////////////////////////////////////////////////
///
/// processingExample.cpp
/// ---------------------
///
/// Copyright (c) 2013 - 2014. Little Endian Ltd. All rights reserved.
///
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#include "le/audioio/device.hpp"
#include "le/audioio/file.hpp"
#include "le/audioio/outputWaveFile.hpp"

#include "le/melodify/melodifyer.hpp"

#include "le/utility/entryPoint.hpp"
#include "le/utility/filesystem.hpp"
#include "le/utility/trace.hpp"
#include "le/utility/sleep.hpp"

#include <assert.h>
#include <jni.h>
#include <string.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <new>

// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
#include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

//------------------------------------------------------------------------------
// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
		SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// URI player interfaces
static SLObjectItf uriPlayerObject = NULL;
static SLPlayItf uriPlayerPlay;
static SLSeekItf uriPlayerSeek;
static SLMuteSoloItf uriPlayerMuteSolo;
static SLVolumeItf uriPlayerVolume;

// file descriptor player interfaces
static SLObjectItf fdPlayerObject = NULL;
static SLPlayItf fdPlayerPlay;
static SLSeekItf fdPlayerSeek;
static SLMuteSoloItf fdPlayerMuteSolo;
static SLVolumeItf fdPlayerVolume;

// recorder interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

// synthesized sawtooth clip
#define SAWTOOTH_FRAMES 8000
static short sawtoothBuffer[SAWTOOTH_FRAMES];

// 5 seconds of recorded audio at 16 kHz mono, 16-bit signed little endian
#define RECORDER_FRAMES (16000 * 5)
static short recorderBuffer[RECORDER_FRAMES];
static unsigned recorderSize = 0;
static SLmilliHertz recorderSR;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

// i add this
#ifdef __ANDROID__
#endif // __ANDROID__
bool processingExample() {
	////////////////////////////////////////////////////////////////////////////
	// Important note: for improved readability, some error handling (memory
	// allocation failures in particular) has been omitted from this example
	// code.
	////////////////////////////////////////////////////////////////////////////

	using namespace LE;

#define LE_SAMPLE_DIRECTORY "samples/"

#if defined( __ANDROID__ ) || defined( __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ )
	Utility::SpecialLocations const resourcesLocation = Utility::Resources;
#else // Windows & OSX
	Utility::SpecialLocations const resourcesLocation = Utility::CWD;
#ifdef __APPLE__
#undef LE_SAMPLE_DIRECTORY
#define LE_SAMPLE_DIRECTORY "../samples/"
#endif // __APPLE__
#endif // OS
#if defined( __ANDROID__ )
	// On devices with an installed SD card, Utility::ExternalStorage could also
	// be used for more convenience...
	Utility::SpecialLocations const resultsLocation = Utility::AppData;
#elif defined( __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ )
	Utility::SpecialLocations const resultsLocation = Utility::Documents;
#else // Windows & OSX
	Utility::SpecialLocations const resultsLocation = Utility::CWD;
#endif // OS

	char const inputVoiceFileName[] = LE_SAMPLE_DIRECTORY "Speech.mp3";
	char const inputBackgroundFileName[] = LE_SAMPLE_DIRECTORY "Background.mp3";
	char const inputMIDIFileName[] = LE_SAMPLE_DIRECTORY "Melody.mid";
	char const outputFileName[] = "MelodifyedSpeech.wav";

	typedef float sample_t;
	typedef std::auto_ptr<sample_t> Buffer;
	__android_log_print(ANDROID_LOG_ERROR, "bad", "1");
	AudioIO::File backgroundFile;
	char const * pErrorMessage(
			backgroundFile.open < resourcesLocation
					> (inputBackgroundFileName));
	if (pErrorMessage) {
		__android_log_print(ANDROID_LOG_ERROR, "bad", "1.5");
		Utility::Tracer::formattedError(
				"Failed to open background input file: %s (%s, errno: %d).",
				inputBackgroundFileName, pErrorMessage, errno);
		__android_log_print(ANDROID_LOG_ERROR, "bad", "1.6");
		return false;
	}

	__android_log_print(ANDROID_LOG_ERROR, "bad", "2");

	auto const sampleRate(backgroundFile.sampleRate());
	auto const numberOfChannels(backgroundFile.numberOfChannels());
	auto numberOfBackgroundSamples(backgroundFile.lengthInSamples());

	SW::Melodifyer melodifyer;
	melodifyer.setup(sampleRate, numberOfChannels);

	// We are doing offline processing so we can use the latency information to
	// compensate for the inherent delay.
	auto const latency(melodifyer.latencyInSamples());
	Buffer const pBackground(
			new sample_t[(latency + numberOfBackgroundSamples)
					* numberOfChannels]);
	std::fill_n(pBackground.get(), latency, 0.0f);
	numberOfBackgroundSamples = backgroundFile.read(&pBackground.get()[latency],
			numberOfBackgroundSamples);

	unsigned int const melodyTrack(1);
	unsigned int const melodyChannel(0);
	pErrorMessage = melodifyer.setMelodyMIDIFile < resourcesLocation
			> (inputMIDIFileName, melodyTrack, melodyChannel);
	if (pErrorMessage) {
		Utility::Tracer::error(pErrorMessage);
		return false;
	}

	AudioIO::File inputFile;
	pErrorMessage = inputFile.open < resourcesLocation > (inputVoiceFileName);
	if (pErrorMessage) {
		Utility::Tracer::error(pErrorMessage);
		return false;
	}
	if (inputFile.sampleRate() != sampleRate) {
		Utility::Tracer::error(
				"Voice and background inputs have mismatched samplerates.");
		return false;
	}
	if (inputFile.numberOfChannels() != numberOfChannels) {
		Utility::Tracer::error(
				"Voice and background inputs have mismatched number of channels.");
		return false;
	}
	if (numberOfChannels != 1 && numberOfChannels != 2) {
		Utility::Tracer::error("Only mono and stereo input is supported.");
		return false;
	}

	auto numberOfInputSamples(inputFile.lengthInSamples());
	Buffer const pMainInput(
			new sample_t[numberOfChannels * numberOfInputSamples]);
	numberOfInputSamples = inputFile.read(pMainInput.get(),
			numberOfInputSamples);

	Buffer const pOutput(
			new sample_t[numberOfChannels * numberOfBackgroundSamples]);

	Utility::Tracer::message("Processing input data...");

	std::clock_t const startTime(std::clock());

	unsigned int const numberOfOutputSamples(
			std::min(numberOfInputSamples, numberOfBackgroundSamples));
	unsigned int processSize(1024);
	for (unsigned sample(0); sample < numberOfOutputSamples; sample +=
			processSize) {
		processSize = std::min<unsigned int>(processSize,
				numberOfOutputSamples - sample);
		melodifyer.process(&pMainInput.get()[sample * numberOfChannels],
				&pBackground.get()[sample * numberOfChannels],
				&pOutput.get()[sample * numberOfChannels], processSize);
	}

	float const elapsedMilliseconds(
			(std::clock() - startTime) * 1000.0f / CLOCKS_PER_SEC);
	float const dataMilliseconds(numberOfOutputSamples * 1000.0f / sampleRate);
	float const processingSpeedRatio(dataMilliseconds / elapsedMilliseconds);
	unsigned int const totalProcessedSamples(
			numberOfOutputSamples * numberOfChannels
					* (pBackground.get() ? 2 : 1));
	Utility::Tracer::formattedMessage(
			"Done: %.2f ms of data, %.2f ms processing time (%.2f ratio, %.0f ksamples/second).\n",
			dataMilliseconds, elapsedMilliseconds, processingSpeedRatio,
			totalProcessedSamples / elapsedMilliseconds);

	////////////////////////////////////////////////////////////////////////////
	// Save the processed data:
	////////////////////////////////////////////////////////////////////////////
	{
		AudioIO::OutputWaveFile outputFile;
		pErrorMessage = outputFile.create < resultsLocation
				> (outputFileName, numberOfChannels, sampleRate);
		if (pErrorMessage) {
			Utility::Tracer::formattedError(
					"Failed to create output file: %s (%s, errno: %d).",
					Utility::fullPath < resultsLocation > (outputFileName),
					pErrorMessage, errno);
			return false;
		}

		Utility::Tracer::formattedMessage("Writing processed data to %s...\n",
				Utility::fullPath < resultsLocation > (outputFileName));
		pErrorMessage = outputFile.write(
				&pOutput.get()[numberOfChannels * latency],
				numberOfChannels * (numberOfOutputSamples - latency));
		if (pErrorMessage) {
			Utility::Tracer::formattedError(
					"Failed to write output file (%s, errno: %d).",
					pErrorMessage, errno);
			return false;
		}
	}

	Utility::Tracer::message(
			" * real time rendering through the hardware audio device...");

	AudioIO::Device device;
	if (auto const err = device.setup(numberOfChannels, sampleRate, 0)) {
		Utility::Tracer::error(err);
		return false;
	}

	bool const slowPreset(processingSpeedRatio < 1.5f);
	if (slowPreset) {
		Utility::Tracer::message(
				"\t...device too slow for realtime processing, playing preprocessed data and skipping full duplex rendering...");

		struct PreprocessedOutputContext {
			AudioIO::BlockingDevice blockingDevice;
			AudioIO::Device::InterleavedInputData pPreprocessedData;
			unsigned int numberOfSamples;
			unsigned int numberOfChannels;

			static void callback(void * const pContext,
					AudioIO::Device::InterleavedOutputData const pOutputBuffers,
					unsigned int numberOfSamples) {
				PreprocessedOutputContext & context(
						*static_cast<PreprocessedOutputContext *>(pContext));

				numberOfSamples = std::min(context.numberOfSamples,
						numberOfSamples);

				unsigned int const interleavedSamples(
						numberOfSamples * context.numberOfChannels);

				std::copy(context.pPreprocessedData,
						context.pPreprocessedData + interleavedSamples,
						pOutputBuffers);

				context.numberOfSamples -= numberOfSamples;
				if (!context.numberOfSamples) {
					context.blockingDevice.stop();
					return;
				}

				context.pPreprocessedData += interleavedSamples;
			}
		};
		// struct PreprocessedOutputContext
		PreprocessedOutputContext context = { device, pOutput.get(),
				numberOfOutputSamples, numberOfChannels };
		if (auto err = device.setCallback(&PreprocessedOutputContext::callback,
				&context)) {
			Utility::Tracer::error(err);
			return false;
		}
		melodifyer.reset();
		context.blockingDevice.startAndWait();
	} else {
		struct RealTimeInputOutputContext {
			AudioIO::BlockingDevice blockingDevice;

			SW::Melodifyer & processor;

			AudioIO::Device::InterleavedInputData pBackgroundData;

			unsigned int numberOfSamples;
			unsigned int const numberOfChannels;

			static void callback(void * const pContext,
					AudioIO::Device::InterleavedInputData const pInputBuffers,
					AudioIO::Device::InterleavedOutputData const pOutputBuffers,
					unsigned int numberOfSamples) {
				RealTimeInputOutputContext & context(
						*static_cast<RealTimeInputOutputContext *>(pContext));

				numberOfSamples = std::min(context.numberOfSamples,
						numberOfSamples);

				unsigned int const interleavedSamples(
						numberOfSamples * context.numberOfChannels);

				context.processor.process(pInputBuffers,
						context.pBackgroundData, pOutputBuffers,
						numberOfSamples);

				context.numberOfSamples -= numberOfSamples;
				if (!context.numberOfSamples) {
					context.blockingDevice.stop();
					return;
				}

				if (context.pBackgroundData)
					context.pBackgroundData += interleavedSamples;
			}
		};
		// struct RealTimeInputOutputContext

		Utility::Tracer::message(
				" * full duplex real time rendering - please speak into the microphone - and listen yourself sing :)");
		RealTimeInputOutputContext context = { device, melodifyer,
				pBackground.get(), numberOfBackgroundSamples, numberOfChannels };
		if (auto err = device.setCallback(&RealTimeInputOutputContext::callback,
				&context)) {
			Utility::Tracer::error(err);
			return false;
		}
		melodifyer.reset();
		context.blockingDevice.startAndWait();
	}

	Utility::Tracer::message("Done.");

	return true;
}

bool entryPoint() {
	using namespace LE::Utility;
	Tracer::pTagString = "Melodify example app";
	bool const success(processingExample());
	if (success)
		Tracer::message(
				"The LE Melodify SDK sample application finished successfully.");
	else
		Tracer::error(
				"The LE Melodify SDK sample application finished unsuccessfully.");
	return success;
}

//LE_UTILITY_ENTRY_POINT( entryPoint )

//------------------------------------------------------------------------------

static JavaVM *g_VM;

//extern "C" {
// JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved){
//	 g_VM = jvm;
//	 return JNI_VERSION_1_6;
// }
//};
extern "C" jstring Java_com_example_ledemo_MainActivity_modify(JNIEnv* env,
		jclass clazz, jobject assetManager, jstring filename) {

	using namespace LE;

	__android_log_print(ANDROID_LOG_ERROR, "melodify", "start");

	// convert Java string to UTF-8
//	const char *utf8 = env->GetStringUTFChars(filename, NULL);
//	assert(NULL != utf8);
//
	// use asset manager to open asset by filename
//	AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
//	assert(NULL != mgr);
	//AAsset* asset = AAssetManager_open(mgr, utf8, AASSET_MODE_UNKNOWN);

//	// release the Java string and UTF-8
//	env->ReleaseStringUTFChars(filename, utf8);

	env->GetJavaVM(&g_VM);

	g_VM->AttachCurrentThread(&env, NULL);

	if (g_VM != NULL)
		__android_log_print(ANDROID_LOG_ERROR, "melodify", "not null");
	else
		__android_log_print(ANDROID_LOG_ERROR, "melodify", "null");

	jclass jj = env->FindClass("com/example/ledemo/MainActivity");

	if (jj != NULL)
		__android_log_print(ANDROID_LOG_ERROR, "melodify", "jClass not null");
	else
		__android_log_print(ANDROID_LOG_ERROR, "melodify", "jClass null");

	sleep(3);
	LE::Utility::setAppContext(*g_VM, jj, assetManager);

	//entryPoint();

	return env->NewStringUTF("Hi");
}

// create asset audio player
extern "C" jboolean Java_com_example_ledemo_MainActivity_createAssetAudioPlayer(
		JNIEnv* env, jclass clazz, jobject assetManager, jstring filename) {
	SLresult result;

	// convert Java string to UTF-8
	const char *utf8 = env->GetStringUTFChars(filename, NULL);
	assert(NULL != utf8);

	// use asset manager to open asset by filename
	AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
	assert(NULL != mgr);
	AAsset* asset = AAssetManager_open(mgr, utf8, AASSET_MODE_UNKNOWN);

	// release the Java string and UTF-8
	env->ReleaseStringUTFChars(filename, utf8);

	// the asset might not be found
	if (NULL == asset) {
		return JNI_FALSE;
	}

	// open asset as file descriptor
	off_t start, length;
	int fd = AAsset_openFileDescriptor(asset, &start, &length);
	assert(0 <= fd);
	AAsset_close(asset);

	// configure audio source
	SLDataLocator_AndroidFD loc_fd = { SL_DATALOCATOR_ANDROIDFD, fd, start,
			length };
	SLDataFormat_MIME format_mime = { SL_DATAFORMAT_MIME, NULL,
			SL_CONTAINERTYPE_UNSPECIFIED };
	SLDataSource audioSrc = { &loc_fd, &format_mime };

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX,
			outputMixObject };
	SLDataSink audioSnk = { &loc_outmix, NULL };

	// create audio player
	const SLInterfaceID ids[3] = { SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME };
	const SLboolean req[3] =
			{ SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &fdPlayerObject,
			&audioSrc, &audioSnk, 3, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// realize the player
	result = (*fdPlayerObject)->Realize(fdPlayerObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the play interface
	result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_PLAY,
			&fdPlayerPlay);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the seek interface
	result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_SEEK,
			&fdPlayerSeek);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the mute/solo interface
	result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_MUTESOLO,
			&fdPlayerMuteSolo);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the volume interface
	result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_VOLUME,
			&fdPlayerVolume);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// enable whole file looping
	result = (*fdPlayerSeek)->SetLoop(fdPlayerSeek, SL_BOOLEAN_TRUE, 0,
			SL_TIME_UNKNOWN);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	return JNI_TRUE;
}

// set the playing state for the asset audio player
extern "C" void Java_com_example_ledemo_MainActivity_setPlayingAssetAudioPlayer(
		JNIEnv* env, jclass clazz, jboolean isPlaying) {
	SLresult result;

	// make sure the asset audio player was created
	if (NULL != fdPlayerPlay) {

		// set the player's state
		result = (*fdPlayerPlay)->SetPlayState(fdPlayerPlay,
				isPlaying ? SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
		assert(SL_RESULT_SUCCESS == result);
		(void) result;
	}

}

// create the engine and output mix objects
extern "C" void Java_com_example_ledemo_MainActivity_createEngine(JNIEnv* env,
		jclass clazz) {
	SLresult result;

	// create engine
	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
			&engineEngine);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean req[1] = { SL_BOOLEAN_FALSE };
	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1,
			ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	result = (*outputMixObject)->GetInterface(outputMixObject,
			SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result) {
		result =
				(*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
						outputMixEnvironmentalReverb, &reverbSettings);
		(void) result;
	}
	// ignore unsuccessful result codes for environmental reverb, as it is optional for this example

}

// create buffer queue audio player
extern "C" void Java_com_example_ledemo_MainActivity_createBufferQueueAudioPlayer(
		JNIEnv* env, jclass clazz) {
	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
			SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
			SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
			SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
	SLDataSource audioSrc = { &loc_bufq, &format_pcm };

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX,
			outputMixObject };
	SLDataSink audioSnk = { &loc_outmix, NULL };

	// create audio player
	const SLInterfaceID ids[3] = { SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
	/*SL_IID_MUTESOLO,*/SL_IID_VOLUME };
	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
	/*SL_BOOLEAN_TRUE,*/SL_BOOLEAN_TRUE };
	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject,
			&audioSrc, &audioSnk, 3, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// realize the player
	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the play interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY,
			&bqPlayerPlay);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the buffer queue interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
			&bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, NULL,
			NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// get the effect send interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
			&bqPlayerEffectSend);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
	// get the mute/solo interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
#endif

	// get the volume interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME,
			&bqPlayerVolume);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;

	// set the player's state to playing
	result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	assert(SL_RESULT_SUCCESS == result);
	(void) result;
}

