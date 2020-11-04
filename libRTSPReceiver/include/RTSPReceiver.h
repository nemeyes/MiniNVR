#pragma once

#if defined(EXPORT_RTSP_CLIENT_LIB)
#define EXP_RTSP_CLIENT_CLASS __declspec(dllexport)
#else
#define EXP_RTSP_CLIENT_CLASS __declspec(dllimport)
#endif

#include <MiniNVR.h>

class EXP_RTSP_CLIENT_CLASS RTSPReceiver
	: public Base
{
public:
	class Core;
	class BufferSink;
	class H2645BufferSink;
	class H265BufferSink;
	class H264BufferSink;
	class AACBufferSink;
public:
	typedef struct _TRANSPORT_OPTION_T
	{
		static const int32_t RTP_OVER_UDP = 0;
		static const int32_t RTP_OVER_TCP = 1;
		static const int32_t RTP_OVER_HTTP = 2;
	} TRANSPORT_OPTION_T;

	RTSPReceiver(void);
	virtual ~RTSPReceiver(void);

	int32_t Play(const char * url, const char * username, const char * password, int32_t transport_option, int32_t recv_option, int32_t recv_timeout, float scale = 1.f, bool repeat = true);
	int32_t Stop(void);
	int32_t Pause(void);

	int32_t Width(void);
	int32_t Height(void);

	virtual void OnBeginVideo(int32_t codec, uint8_t * extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps);
	virtual void OnRecvVideo(uint8_t * bytes, int32_t nbytes, long long pts, long long duration);
	virtual void OnEndVideo(void);

	virtual void OnBeginAudio(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t samplerate, int32_t channels);
	virtual void OnRecvAudio(uint8_t* bytes, int32_t nbytes, long long pts, long long duration);
	virtual void OnEndAudio(void);

private:
	void Process(void);
	static unsigned __stdcall ProcessCB(void * param);

private:
	RTSPReceiver::Core * _live;

	HANDLE	_thread;
	char	_url[260];
	char	_username[260];
	char	_password[260];
	int32_t _transport_option;
	int32_t _recv_option;
	int32_t _recv_timeout;
	float	_scale;
	BOOL 	_repeat;
	bool	_kill;
	BOOL	_ignore_sdp;
};