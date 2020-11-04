
#include "AACBufferSink.h"
#include <H264VideoRTPSource.hh>

RTSPReceiver::AACBufferSink::AACBufferSink(RTSPReceiver::Core * front, UsageEnvironment & env, unsigned buffer_size, int32_t channels, int32_t samplerate, char * configstr, int32_t configstr_size)
	: RTSPReceiver::BufferSink(front, RTSPReceiver::MEDIA_TYPE_T::AUDIO, RTSPReceiver::AUDIO_CODEC_T::AAC, env, buffer_size)
{
	if (_front)
	{
		_front->set_audio_channels(channels);
		_front->set_audio_samplerate(samplerate);
		_front->set_audio_extradata((uint8_t*)configstr, configstr_size);
	}
}

RTSPReceiver::AACBufferSink::~AACBufferSink(void)
{
}

RTSPReceiver::AACBufferSink* RTSPReceiver::AACBufferSink::createNew(RTSPReceiver::Core * front, UsageEnvironment & env, unsigned buffer_size, int32_t channels, int32_t samplerate, char * configstr, int32_t configstr_size)
{
	return new RTSPReceiver::AACBufferSink(front, env, buffer_size, channels, samplerate, configstr, configstr_size);
}


void RTSPReceiver::AACBufferSink::after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec)
{
	RTSPReceiver::BufferSink::after_getting_frame(frame_size, truncated_bytes, presentation_time, duration_msec);
}
