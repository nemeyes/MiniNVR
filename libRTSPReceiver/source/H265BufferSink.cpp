
#include "H265BufferSink.h"

RTSPReceiver::H265BufferSink::H265BufferSink(RTSPReceiver::Core * front, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size)
	: RTSPReceiver::H2645BufferSink(front, RTSPReceiver::VIDEO_CODEC_T::HEVC, env, vps, vps_size, sps, sps_size, pps, pps_size, buffer_size)
{

}

RTSPReceiver::H265BufferSink::~H265BufferSink(void)
{

}

RTSPReceiver::H265BufferSink * RTSPReceiver::H265BufferSink::createNew(RTSPReceiver::Core * front, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size)
{
	return new RTSPReceiver::H265BufferSink(front, env, vps, vps_size, sps, sps_size, pps, pps_size, buffer_size);
}
