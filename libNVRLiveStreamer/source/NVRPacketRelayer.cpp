#include "NVRPacketRelayer.h"
#include <wchar.h>

NVRPacketRelayer::NVRPacketRelayer(const char* name, const char* url)
{
	::strncpy_s(_name, name, MAX_PATH);
	::strncpy_s(_url, url, MAX_PATH);

	HINSTANCE module_handle = ::GetModuleHandleA("libNVRLiveStreamer.dll");
	char module_path[MAX_PATH] = { 0 };
	char* module_name = module_path;
	module_name += GetModuleFileNameA(module_handle, module_name, (sizeof(module_path) / sizeof(*module_path)) - (module_name - module_path));
	if (module_name != module_path)
	{
		char* slash = strrchr(module_path, '\\');
		if (slash != NULL)
		{
			module_name = slash + 1;
			_strset_s(module_name, strlen(module_name) + 1, 0);
		}
		else
		{
			_strset_s(module_path, strlen(module_path) + 1, 0);
		}
	}
	_muxer_ctx.generationRule = FragmentedMP4Muxer::GENERATION_RULE_T::FRAME;
	_muxer_ctx.generationThreshold = 10;
	::_snprintf_s(_muxer_ctx.path, MAX_PATH, "%s\\%s", module_path, name);

	_timestamp_generator.BeginElapsedTime();
}

NVRPacketRelayer::~NVRPacketRelayer(void)
{

}

int32_t NVRPacketRelayer::Start(void)
{
	return RTSPReceiver::Play(_url, NULL, NULL, RTSPReceiver::TRANSPORT_OPTION_T::RTP_OVER_TCP, RTSPReceiver::MEDIA_TYPE_T::VIDEO | RTSPReceiver::MEDIA_TYPE_T::AUDIO, 3);
}

int32_t NVRPacketRelayer::Stop(void)
{
	return RTSPReceiver::Stop();
}

void NVRPacketRelayer::OnBeginVideo(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps)
{
	_muxer_ctx.videoCodec = codec;
	_muxer_ctx.videoExtradataSize = extradata_size;
	::memmove(_muxer_ctx.videoExtradata, extradata, _muxer_ctx.videoExtradataSize);
	_muxer_ctx.videoHeight = width;
	_muxer_ctx.videoWidth = width;
	_muxer_ctx.option |= FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO;
}

void NVRPacketRelayer::OnRecvVideo(uint8_t* bytes, int32_t nbytes, long long pts, long long duration)
{
	if (!_muxer.IsInitialized())
		_muxer.Initialize(&_muxer_ctx);
	_muxer.PutVideoStream(bytes, nbytes, _timestamp_generator.ElapsedMicroseconds());
}

void NVRPacketRelayer::OnEndVideo(void)
{
	if (_muxer.IsInitialized())
		_muxer.Release();
}

void NVRPacketRelayer::OnBeginAudio(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t samplerate, int32_t channels)
{
	_muxer_ctx.audioCodec = codec;
	_muxer_ctx.audioExtradataSize = extradata_size;
	::memmove(_muxer_ctx.audioExtradata, extradata, _muxer_ctx.audioExtradataSize);
	_muxer_ctx.audioChannels = channels;
	_muxer_ctx.audioSamplerate = samplerate;
	_muxer_ctx.option |= FragmentedMP4Muxer::MEDIA_TYPE_T::AUDIO;
}

void NVRPacketRelayer::OnRecvAudio(uint8_t* bytes, int32_t nbytes, long long pts, long long duration)
{
	if (!_muxer.IsInitialized())
		_muxer.Initialize(&_muxer_ctx);
	_muxer.PutAudioStream(bytes, nbytes, _timestamp_generator.ElapsedMicroseconds());
}

void NVRPacketRelayer::OnEndAudio(void)
{
	if (_muxer.IsInitialized())
		_muxer.Release();
}