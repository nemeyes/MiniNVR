#pragma once

#include "NVRLiveStreamer.h"
#include "NVRPacketRelayer.h"

class NVRLiveStreamer::Core
{
public:
	Core(NVRLiveStreamer* front);
	~Core(void);

	int32_t AddStream(const char* name, const char* url);
	int32_t RemoveStream(const char* name);
	int32_t StartStream(const char* name);
	int32_t StopStream(const char* name);

private:
	NVRLiveStreamer* _front;
	CRITICAL_SECTION _lock;
	std::map<std::string, NVRPacketRelayer*> _map;

};