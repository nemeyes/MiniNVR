#ifndef _AAC_BUFFER_SINK_H_
#define _AAC_BUFFER_SINK_H_

#include "BufferSink.h"
#include "RTSPReceiverCore.h"

class RTSPReceiver::AACBufferSink
	: public RTSPReceiver::BufferSink
{
public:
	static RTSPReceiver::AACBufferSink* createNew(RTSPReceiver::Core * front, UsageEnvironment & env, unsigned buffer_size, int32_t channels, int32_t samplerate, char * configstr, int32_t configstr_size);

protected:
	AACBufferSink(RTSPReceiver::Core* front, UsageEnvironment & env, unsigned buffer_size, int32_t channels, int32_t samplerate, char * configstr, int32_t configstr_size);
	virtual ~AACBufferSink(void);

	virtual void after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec);
};

#endif