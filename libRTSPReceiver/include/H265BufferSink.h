#ifndef _H265_BUFFER_SINK_H_
#define _H265_BUFFER_SINK_H_

#include "H2645BufferSink.h"
#include "RTSPReceiverCore.h"

class RTSPReceiver::H265BufferSink
	: public RTSPReceiver::H2645BufferSink
{
public:
	static RTSPReceiver::H265BufferSink * createNew(RTSPReceiver::Core * front, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size);

protected:
	H265BufferSink(RTSPReceiver::Core* front, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size);
	virtual ~H265BufferSink(void);
};

#endif // H265_BUFFER_SINK_H

