#include "NVRLiveStreamer.h"
#include "NVRLiveStreamerCore.h"

NVRLiveStreamer::NVRLiveStreamer(void)
{
	_core = new NVRLiveStreamer::Core(this);
}

NVRLiveStreamer::~NVRLiveStreamer(void)
{
	if (_core)
		delete _core;
	_core = NULL;
}

int32_t NVRLiveStreamer::AddStream(const char* name, const char* url)
{
	return _core->AddStream(name, url);
}

int32_t NVRLiveStreamer::RemoveStream(const char* name)
{
	return _core->RemoveStream(name);
}

int32_t NVRLiveStreamer::StartStream(const char* name)
{
	return _core->StartStream(name);
}

int32_t NVRLiveStreamer::StopStream(const char* name)
{
	return _core->StopStream(name);
}