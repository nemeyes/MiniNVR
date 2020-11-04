#ifndef _H2645_BUFFER_SINK_H_
#define _H2645_BUFFER_SINK_H_

#include "BufferSink.h"
#include "RTSPReceiverCore.h"

class RTSPReceiver::H2645BufferSink
	: public RTSPReceiver::BufferSink
{
public:
	H2645BufferSink(RTSPReceiver::Core * front, int32_t codec, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size);
	virtual ~H2645BufferSink(void);

protected:
	virtual void after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec);

private:
	uint8_t *		_vspps[3];
	int32_t			_vspps_size[3];
	bool			_receive_first_frame;
	unsigned char	_vspps_buffer[MAX_PATH];

};

#endif // H264_BUFFER_SINK_H

