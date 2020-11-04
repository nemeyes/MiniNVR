
#include "H264BufferSink.h"

RTSPReceiver::H264BufferSink::H264BufferSink(RTSPReceiver::Core * front, UsageEnvironment & env, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size)
	: RTSPReceiver::H2645BufferSink(front, RTSPReceiver::VIDEO_CODEC_T::AVC, env, nullptr, 0, sps, sps_size, pps, pps_size, buffer_size)
{

}

RTSPReceiver::H264BufferSink::~H264BufferSink(void)
{

}

RTSPReceiver::H264BufferSink* RTSPReceiver::H264BufferSink::createNew(RTSPReceiver::Core * front, UsageEnvironment & env, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size)
{
	return new RTSPReceiver::H264BufferSink(front, env, sps, sps_size, pps, pps_size, buffer_size);
}
