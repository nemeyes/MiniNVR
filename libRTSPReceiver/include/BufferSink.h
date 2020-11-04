#ifndef _BUFFER_SINK_H_
#define _BUFFER_SINK_H_

#include <MediaSink.hh>
#include <UsageEnvironment.hh>
#include "RTSPReceiverCore.h"

class RTSPReceiver::BufferSink : public MediaSink
{
public:
	static RTSPReceiver::BufferSink* createNew(RTSPReceiver::Core * front, int32_t mt, int32_t codec, UsageEnvironment & env, unsigned buffer_size);

	virtual void add_data(unsigned char * data, unsigned size, struct timeval presentation_time, unsigned duration_msec);

protected:
	BufferSink(RTSPReceiver::Core * front, int32_t mt, int32_t codec, UsageEnvironment & env, unsigned buffer_size);
	virtual ~BufferSink(void);

protected: //redefined virtual functions
	virtual Boolean continuePlaying(void);

protected:
	static void after_getting_frame(void * param, unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec);
	virtual void after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec);


	RTSPReceiver::Core* _front;
	unsigned char *		_buffer;
	unsigned			_buffer_size;
	int32_t				_mt;
	int32_t				_vcodec;
	int32_t				_acodec;
};

#endif // BUFFER_SINK_H

