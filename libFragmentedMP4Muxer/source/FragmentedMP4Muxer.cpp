#include "FragmentedMP4Muxer.h"
#include "FragmentedMP4MuxerCore.h"

FragmentedMP4Muxer::FragmentedMP4Muxer(void)
{
	_core = new FragmentedMP4Muxer::Core(this);
}

FragmentedMP4Muxer::~FragmentedMP4Muxer(void)
{
	if (_core)
	{
		delete _core;
		_core = 0;
	}
}

BOOL FragmentedMP4Muxer::IsInitialized(void)
{
	return _core->is_initialized();
}

int32_t FragmentedMP4Muxer::Initialize(FragmentedMP4Muxer::CONTEXT_T* context)
{
	return _core->initialize(context);
}

int32_t FragmentedMP4Muxer::Release(void)
{
	return _core->release();
}

int32_t FragmentedMP4Muxer::PutVideoStream(uint8_t* bytes, int32_t nbytes, int64_t timestamp)
{
	return _core->put_video_stream(bytes, nbytes, timestamp);
}

int32_t FragmentedMP4Muxer::PutAudioStream(uint8_t* bytes, int32_t nbytes, int64_t timestamp)
{
	return _core->put_audio_stream(bytes, nbytes, timestamp);
}