
#include "H2645BufferSink.h"
#include <H264VideoRTPSource.hh>

RTSPReceiver::H2645BufferSink::H2645BufferSink(RTSPReceiver::Core* front, int32_t codec, UsageEnvironment & env, const char * vps, unsigned vps_size, const char * sps, unsigned sps_size, const char * pps, unsigned pps_size, unsigned buffer_size)
	: RTSPReceiver::BufferSink(front, RTSPReceiver::MEDIA_TYPE_T::VIDEO, codec, env, buffer_size)
	, _receive_first_frame(false)
{
	if (vps != nullptr && vps_size > 0)
	{
		_vspps[0] = static_cast<uint8_t*>(malloc(vps_size));
		::memmove(_vspps[0], vps, vps_size);
		_vspps_size[0] = vps_size;
	}
	else
	{
		_vspps[0] = nullptr;
		_vspps_size[0] = 0;
	}
	if (sps != nullptr && sps_size > 0)
	{
		_vspps[1] = static_cast<uint8_t*>(malloc(sps_size));
		::memmove(_vspps[1], sps, sps_size);
		_vspps_size[1] = sps_size;
	}
	else
	{
		_vspps[1] = nullptr;
		_vspps_size[1] = 0;
	}
	if (pps != nullptr && pps_size > 0)
	{
		_vspps[2] = static_cast<uint8_t*>(malloc(pps_size));
		::memmove(_vspps[2], pps, pps_size);
		_vspps_size[2] = pps_size;
	}
	else
	{
		_vspps[2] = nullptr;
		_vspps_size[2] = 0;
	}
	::memset(_vspps_buffer, 0x00, sizeof(_vspps_buffer));
}

RTSPReceiver::H2645BufferSink::~H2645BufferSink(void)
{
	if (_vspps[0] != nullptr)
		::free((void*)_vspps[0]);
	if (_vspps[1] != nullptr)
		::free((void*)_vspps[1]);
	if (_vspps[2] != nullptr)
		::free((void*)_vspps[2]);
}

void RTSPReceiver::H2645BufferSink::after_getting_frame(unsigned frame_size, unsigned truncated_bytes, struct timeval presentation_time, unsigned duration_msec)
{
    const unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};
	//if (!_front->ignore_sdp())
	{
		if (!_receive_first_frame)
		{
			/*
			for (unsigned i = 0; i < 3; ++i)
			{
				if (_vspps_size[i] > 0)
				{
					::memmove(_vspps_buffer, start_code, sizeof(start_code));
					::memmove(_vspps_buffer + sizeof(start_code), _vspps[i], _vspps_size[i]);
					add_data(_vspps_buffer, sizeof(start_code) + _vspps_size[i], presentation_time, duration_msec);
				}
			}
			*/
			_receive_first_frame = true;
		}
    }
	RTSPReceiver::BufferSink::after_getting_frame(frame_size, truncated_bytes, presentation_time, duration_msec);
}
