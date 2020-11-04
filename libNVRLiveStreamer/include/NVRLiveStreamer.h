#pragma once

#if defined(EXPORT_LIVE_STREAMER_LIB)
#define EXP_LIVE_STREAMER_CLASS __declspec(dllexport)
#else
#define EXP_LIVE_STREAMER_CLASS __declspec(dllimport)
#endif

#include <MiniNVR.h>

class EXP_LIVE_STREAMER_CLASS NVRLiveStreamer
	: public Base
{
	class Core;
public:
	NVRLiveStreamer(void);
	~NVRLiveStreamer(void);

	int32_t AddStream(const char* name, const char* url);
	int32_t RemoveStream(const char* name);

	int32_t StartStream(const char* name);
	int32_t StopStream(const char* name);

private:
	NVRLiveStreamer::Core* _core;

};