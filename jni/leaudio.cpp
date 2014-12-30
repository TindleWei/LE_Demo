////////////////////////////////////////////////////////////////////////////////
///
/// leaudio.cpp
/// ---------------------
///
/// Copyright (c) 2014 - 2015. Menkey League. All rights reserved.
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

#include <android/log.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <new>

#define  LOG_TAG    "jniAudio"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
// can log anything like: LOGI("JNI INIT");

#define JNI_VERSION_1_6 0x00010006

extern "C" {
JNIEXPORT void JNICALL Java_com_example_ledemo_MainActivity_setActivity(JNIEnv * env, jobject obj, jobject activity);
};

JavaVM *gJavaVm = NULL;
jobject gActivity;

//jint JNI_OnLoad(JavaVM *javaVm, void *reserved) {
//	gJavaVm = javaVm;
//	return JNI_VERSION_1_6;
//}

JNIEXPORT void JNICALL Java_com_example_ledemo_MainActivity_setActivity(JNIEnv * env, jobject obj, jobject activity) {

	int check = gJavaVm->GetEnv((void **) &env, JNI_VERSION_1_6);
//	LOGE("Detach destructor ==: %d", check);
//	if (check != JNI_EDETACHED)
//	int detach = gJavaVm->DetachCurrentThread();

	//gJavaVm->AttachCurrentThread(&env, NULL);

	gActivity = env->NewGlobalRef(obj);

	//LE::Utility::setAppContext(*gJavaVm, gActivity);

	//gJavaVm->DetachCurrentThread();
}

extern "C" bool Java_com_example_ledemo_MainActivity_my(JNIEnv* env,
		jclass clazz, jstring inputPath, jstring backMusic, jstring midMusic,
		jstring outputPath) {

	using namespace LE;

	Utility::SpecialLocations const resourcesLocation = Utility::AbsolutePath;

	Utility::SpecialLocations const resultsLocation = Utility::AbsolutePath;

	const char *input = env->GetStringUTFChars(inputPath, NULL);
	const char *back = env->GetStringUTFChars(backMusic, NULL);
	const char *mid = env->GetStringUTFChars(midMusic, NULL);
	const char *output = env->GetStringUTFChars(outputPath, NULL);

	char inputVoiceFileName[] = "";
	char inputBackgroundFileName[] = "";
	char inputMIDIFileName[] = "";
	char outputFileName[] = "";

	strcpy(inputVoiceFileName, input);
	strcpy(inputBackgroundFileName, back);
	strcpy(inputMIDIFileName, mid);
	strcpy(outputFileName, output);

	env->ReleaseStringUTFChars(inputPath, input);
	env->ReleaseStringUTFChars(backMusic, back);
	env->ReleaseStringUTFChars(midMusic, mid);
	env->ReleaseStringUTFChars(outputPath, output);

	typedef float sample_t;
	typedef std::auto_ptr<sample_t> Buffer;
	AudioIO::File backgroundFile;

	char const * pErrorMessage(
			backgroundFile.open < resourcesLocation
					> (inputBackgroundFileName));

	if (pErrorMessage) {
		Utility::Tracer::formattedError(
				"Failed to open background input file: %s (%s, errno: %d).",
				inputBackgroundFileName, pErrorMessage, errno);
		return false;
	}

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

	//return true;

}

