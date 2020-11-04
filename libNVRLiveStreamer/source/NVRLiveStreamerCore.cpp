#include "NVRLiveStreamerCore.h"
#include <MiniNVRLocks.h>

NVRLiveStreamer::Core::Core(NVRLiveStreamer* front)
	: _front(front)
{
	::InitializeCriticalSection(&_lock);
}

NVRLiveStreamer::Core::~Core(void)
{
	::DeleteCriticalSection(&_lock);
}

int32_t NVRLiveStreamer::Core::AddStream(const char* name, const char* url)
{
	NVRPacketRelayer* relayer = new NVRPacketRelayer(name, url);
	{
		Autolock lock(&_lock);
		_map.insert(std::make_pair(name, relayer));
	}
	return NVRLiveStreamer::ERR_CODE_T::SUCCESS;
}

int32_t NVRLiveStreamer::Core::RemoveStream(const char* name)
{
	Autolock lock(&_lock);
	_map.erase(name);
	return NVRLiveStreamer::ERR_CODE_T::SUCCESS;
}

int32_t NVRLiveStreamer::Core::StartStream(const char* name)
{
	Autolock lock(&_lock);
	std::map<std::string, NVRPacketRelayer*>::iterator iter = _map.find(name);
	if (iter != _map.end())
	{
		NVRPacketRelayer* relayer = iter->second;
		return relayer->Start();
	}
	return NVRLiveStreamer::ERR_CODE_T::SUCCESS;
}

int32_t NVRLiveStreamer::Core::StopStream(const char* name)
{
	Autolock lock(&_lock);
	std::map<std::string, NVRPacketRelayer*>::iterator iter = _map.find(name);
	if (iter != _map.end())
	{
		NVRPacketRelayer* relayer = iter->second;
		return relayer->Stop();
	}
	return NVRLiveStreamer::ERR_CODE_T::SUCCESS;
}