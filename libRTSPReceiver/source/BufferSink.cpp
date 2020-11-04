#include "BufferSink.h"
#include <GroupsockHelper.hh>

RTSPReceiver::BufferSink::BufferSink(RTSPReceiver::Core * front, int32_t mt, int32_t codec, UsageEnvironment & env, unsigned buffer_size)
    : MediaSink(env)
	, _front(front)
    , _buffer_size(buffer_size)
	, _mt(mt)
{
	if (_mt == RTSPReceiver::MEDIA_TYPE_T::VIDEO)
		_vcodec = codec;
	if (_mt == RTSPReceiver::MEDIA_TYPE_T::AUDIO)
		_acodec = codec;
    _buffer = new unsigned char[buffer_size];
}

RTSPReceiver::BufferSink::~BufferSink(void)
{
	if (_buffer)
	{
		delete[] _buffer;
		_buffer = 0;
	}
}

RTSPReceiver::BufferSink * RTSPReceiver::BufferSink::createNew(RTSPReceiver::Core * front, int32_t mt, int32_t codec, UsageEnvironment & env, unsigned buffer_size)
{
	return new BufferSink(front, mt, codec, env, buffer_size);
}

Boolean RTSPReceiver::BufferSink::continuePlaying(void)
{
    if( !fSource )
        return False;

    fSource->getNextFrame(_buffer, _buffer_size, after_getting_frame, this, onSourceClosure, this);
    return True;
}

void RTSPReceiver::BufferSink::after_getting_frame(void * param, unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec)
{
	RTSPReceiver::BufferSink * sink = static_cast<RTSPReceiver::BufferSink*>(param);
	sink->after_getting_frame(frame_size, truncated_bytes, presentation_time, duration_msec);
}

void RTSPReceiver::BufferSink::add_data(unsigned char * data, unsigned data_size, struct timeval presentation_time, unsigned duration_msec)
{
	long long pts = (presentation_time.tv_sec * 10000000i64) + (presentation_time.tv_usec * 10i64);
	if (_front)
	{
		if (_mt == RTSPReceiver::MEDIA_TYPE_T::VIDEO)
		{
			_front->put_video_sample(_vcodec, data, data_size, pts);
		}
		else if (_mt == RTSPReceiver::MEDIA_TYPE_T::AUDIO)
		{
			_front->put_audio_sample(_acodec, data, data_size, pts);
		}
	}
}

void RTSPReceiver::BufferSink::after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec)
{
	if (_front)
	{
		if (_mt == RTSPReceiver::MEDIA_TYPE_T::VIDEO)
		{
			if (_vcodec == RTSPReceiver::VIDEO_CODEC_T::AVC)
			{
				const unsigned char start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
				if ((_buffer[0] == start_code[0]) && (_buffer[1] == start_code[1]) && (_buffer[2] == start_code[2]) && (_buffer[3] == start_code[3]))
					add_data(_buffer, frame_size, presentation_time, duration_msec);
				else
				{
					if (truncated_bytes > 0)
						::memmove(_buffer + 4, _buffer, frame_size - 4);
					else
					{
						truncated_bytes = (frame_size + 4) - _buffer_size;
						if (truncated_bytes > 0 && (frame_size + 4) > _buffer_size)
							::memmove(_buffer + 4, _buffer, frame_size - truncated_bytes);
						else
							::memmove(_buffer + 4, _buffer, frame_size);
					}
					::memmove(_buffer, start_code, sizeof(start_code));
					add_data(_buffer, frame_size + sizeof(start_code), presentation_time, duration_msec);
				}
			} 
			else if (_vcodec == RTSPReceiver::VIDEO_CODEC_T::HEVC)
			{
				const unsigned char start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
				if ((_buffer[0] == start_code[0]) && (_buffer[1] == start_code[1]) && (_buffer[2] == start_code[2]) && (_buffer[3] == start_code[3]))
					add_data(_buffer, frame_size, presentation_time, duration_msec);
				else
				{
					if (truncated_bytes > 0)
						::memmove(_buffer + 4, _buffer, frame_size - 4);
					else
					{
						truncated_bytes = (frame_size + 4) - _buffer_size;
						if (truncated_bytes > 0 && (frame_size + 4) > _buffer_size)
							::memmove(_buffer + 4, _buffer, frame_size - truncated_bytes);
						else
							::memmove(_buffer + 4, _buffer, frame_size);
					}
					::memmove(_buffer, start_code, sizeof(start_code));
					add_data(_buffer, frame_size + sizeof(start_code), presentation_time, duration_msec);
				}
			}
		}
		else if (_mt == RTSPReceiver::MEDIA_TYPE_T::AUDIO)
		{
			add_data(_buffer, frame_size, presentation_time, duration_msec);
		}
	}
    continuePlaying();
}