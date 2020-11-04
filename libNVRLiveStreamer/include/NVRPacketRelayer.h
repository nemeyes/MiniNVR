#pragma once

#include <RTSPReceiver.h>
#include <FragmentedMP4Muxer.h>
#include <TimestampGenerator.h>

class NVRPacketRelayer
	: public RTSPReceiver
{
public:
	NVRPacketRelayer(const char* name, const char* url);
	~NVRPacketRelayer(void);

	int32_t Start(void);
	int32_t Stop(void);

	virtual void OnBeginVideo(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps);
	virtual void OnRecvVideo(uint8_t* bytes, int32_t nbytes, long long pts, long long duration);
	virtual void OnEndVideo(void);

	virtual void OnBeginAudio(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t samplerate, int32_t channels);
	virtual void OnRecvAudio(uint8_t* bytes, int32_t nbytes, long long pts, long long duration);
	virtual void OnEndAudio(void);

private:
	char _name[MAX_PATH];
	char _url[MAX_PATH];

	FragmentedMP4Muxer::CONTEXT_T	_muxer_ctx;
	FragmentedMP4Muxer				_muxer;
	TimestampGenerator				_timestamp_generator;
};