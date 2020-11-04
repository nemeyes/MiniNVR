#pragma once

#if defined(EXPORT_FRAGMENTED_MP4_MUXER_LIB)
# define EXP_FRAGMENTED_MP4_MUXER_CLASS __declspec(dllexport)
#else
# define EXP_FRAGMENTED_MP4_MUXER_CLASS __declspec(dllimport)
#endif

#include <MiniNVR.h>

class EXP_FRAGMENTED_MP4_MUXER_CLASS FragmentedMP4Muxer
	: public Base
{
public:
	class Core;
public:
	static const int64_t no_value_pts = ((int64_t)UINT64_C(0x8000000000000000));

	typedef struct _GENERATION_RULE_T
	{
		static const int32_t FULL = 0;
		static const int32_t TIME = 1;
		static const int32_t FRAME = 2;
		static const int32_t SIZE = 3;
	} GENERATION_RULE_T;

	typedef struct EXP_FRAGMENTED_MP4_MUXER_CLASS _CONTEXT_T
	{
		int32_t		option;
		char		path[MAX_PATH];

		int32_t		generationRule;
		int32_t		generationThreshold;//size(MB) or time(millisecond)

		int32_t		videoCodec;
		uint8_t		videoExtradata[500];
		int32_t		videoExtradataSize;
		BOOL		ignoreVideoExtradata;
		int32_t		videoWidth;
		int32_t		videoHeight;

		int32_t		audioCodec;
		uint8_t		audioExtradata[500];
		int32_t		audioExtradataSize;
		BOOL		ignoreAudioExtradata;
		int32_t		audioSamplerate;
		int32_t		audioSampleFormat;
		int32_t		audioChannels;
		int32_t		audioFrameSize;

		wchar_t		muxingDstPath[MAX_PATH];
		_CONTEXT_T(void)
			: option(FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO | FragmentedMP4Muxer::MEDIA_TYPE_T::AUDIO)
			, generationRule(FragmentedMP4Muxer::GENERATION_RULE_T::FRAME)
			, generationThreshold(1000)
			, videoCodec(FragmentedMP4Muxer::VIDEO_CODEC_T::UNKNOWN)
			, videoExtradataSize(0)
			, ignoreVideoExtradata(FALSE)
			, videoWidth(1280)
			, videoHeight(720)
			, audioCodec(FragmentedMP4Muxer::AUDIO_CODEC_T::UNKNOWN)
			, audioExtradataSize(0)
			, ignoreAudioExtradata(FALSE)
			, audioSamplerate(44100)
			, audioSampleFormat(FragmentedMP4Muxer::AUDIO_SAMPLE_T::FLTP)
			, audioChannels(2)
			, audioFrameSize(1024)
		{
			::memset(path, 0x00, sizeof(path));
			::memset(videoExtradata, 0x00, sizeof(videoExtradata));
			::memset(audioExtradata, 0x00, sizeof(audioExtradata));
		}
	} CONTEXT_T;

	FragmentedMP4Muxer(void);
	virtual ~FragmentedMP4Muxer(void);

	BOOL	IsInitialized(void);

	int32_t Initialize(FragmentedMP4Muxer::CONTEXT_T* context);
	int32_t Release(void);

	int32_t PutVideoStream(uint8_t* bytes, int32_t nbytes, int64_t timestamp = -1);
	int32_t PutAudioStream(uint8_t* bytes, int32_t nbytes, int64_t timestamp = -1);

private:
	FragmentedMP4Muxer::Core* _core;

};