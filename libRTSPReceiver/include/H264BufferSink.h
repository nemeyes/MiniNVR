#ifndef _H264_BUFFER_SINK_H_
#define _H264_BUFFER_SINK_H_

#include "H2645BufferSink.h"
#include "RTSPReceiverCore.h"

class RTSPReceiver::H264BufferSink
	: public RTSPReceiver::H2645BufferSink
{
public:
	static RTSPReceiver::H264BufferSink* createNew(RTSPReceiver::Core * front, UsageEnvironment & env, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size = 100000);

protected:
	H264BufferSink(RTSPReceiver::Core * front, UsageEnvironment & env, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size);
	virtual ~H264BufferSink(void);
};


#endif // H264_BUFFER_SINK_H

