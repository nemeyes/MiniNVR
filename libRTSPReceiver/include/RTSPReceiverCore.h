#pragma once

#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include <signal.h>
#include <memory>
#include <vector>
#include <ppl.h>
#include <concurrent_queue.h>

#include "RTSPReceiver.h"

class RTSPReceiver::Core : public RTSPClient
{
	friend class RTSPReceiver::BufferSink;
public:
	typedef struct _sample_t
	{
		int32_t		codec;
		uint8_t *	bytes;
		int32_t		nbytes;
		long long	pts;
		_sample_t(int32_t codec, uint8_t* bytes, int32_t nbytes, long long pts)
			: codec(-1)
			, bytes(NULL)
			, nbytes(0)
			, pts(0)
		{
			if (bytes != NULL && nbytes > 0)
			{
				this->codec = codec;
				this->nbytes = nbytes;
				this->pts = pts;
				this->bytes = static_cast<uint8_t*>(malloc(this->nbytes));
				::memmove(this->bytes, bytes, this->nbytes);
			}
		}

		~_sample_t(void)
		{
			if (bytes != NULL && nbytes > 0)
			{
				free(bytes);
				bytes = NULL;
			}
			nbytes = 0;
			pts = 0;
			codec = -1;
		}
	} sample_t;

	static RTSPReceiver::Core * createNew(RTSPReceiver* front, UsageEnvironment & env, const char * url, const char * username, const char * password, int transport_option, int recv_option, int recv_timeout, float scale, unsigned int http_port_number, bool * kill_flag);

	static void continue_after_client_creation(RTSPClient * param);
	static void continue_after_options(RTSPClient * param, int result_code, char * result_string);
	static void continue_after_describe(RTSPClient * param, int result_code, char * result_string);
	static void continue_after_setup(RTSPClient * param, int result_code, char * result_string);
	static void continue_after_play(RTSPClient * param, int result_code, char * result_string);
	static void continue_after_pause(RTSPClient * param, int result_code, char * result_string);
	static void continue_after_teardown(RTSPClient * param, int result_code, char * result_string);

	void start_pausing_session(void);
	void close(void);


	void		set_vps(uint8_t* vps, int32_t vps_size);
	void		set_sps(uint8_t* sps, int32_t sps_size);
	void		set_pps(uint8_t* pps, int32_t pps_size);
	uint8_t *	get_vps(int32_t& vps_size);
	uint8_t *	get_sps(int32_t& sps_size);
	uint8_t *	get_pps(int32_t& pps_size);

	void		set_audio_extradata(uint8_t* extradata, int32_t size);
	uint8_t *	get_audio_extradata(int32_t& size);
	void		set_audio_channels(int32_t channels);
	int32_t		get_audio_channels(void);
	void		set_audio_samplerate(int32_t samplerate);
	int32_t		get_audio_samplerate(void);

	static BOOL is_vps(int32_t codec, uint8_t nalu_type);
	static BOOL is_sps(int32_t codec, uint8_t nalu_type);
	static BOOL is_pps(int32_t codec, uint8_t nalu_type);
	static BOOL is_idr(int32_t codec, uint8_t nalu_type);
	static BOOL is_vlc(int32_t codec, uint8_t nalu_type);

	void		width(int32_t wdth);
	void		height(int32_t hght);
	int32_t		width(void);
	int32_t		height(void);

protected:
	Core(RTSPReceiver* front, UsageEnvironment & env, const char * url, const char * username, const char * password, int transport_option, int recv_option, int recv_timeout, float scale, unsigned int http_port_number, bool * kill_flag);
	~Core(void);

private:
	void		get_options(RTSPClient::responseHandler * after_func);
	void		get_description(RTSPClient::responseHandler * after_func);
	void		setup_media_subsession(MediaSubsession * media_subsession, bool rtp_over_tcp, bool force_multicast_unspecified, RTSPClient::responseHandler * after_func);
	void		start_playing_session(MediaSession * media_session, double start, double end, float scale, RTSPClient::responseHandler * after_func);
	void		start_playing_session(MediaSession * media_session, const char * abs_start_time, const char * abs_end_time, float scale, RTSPClient::responseHandler * after_func);
	void		teardown_session(MediaSession * media_session, RTSPClient::responseHandler * after_func);
	void		set_user_agent_string(const char * user_agent);

	void		setup_streams(void);
	void		shutdown(void);

	static void subsession_after_playing(void * param);
	static void subsession_bye_handler(void * param);
	static void session_after_playing(void * param = 0);

	static void session_timer_handler(void * param);
	static void check_packet_arrival(void * param);
	static void check_inter_packet_gaps(void * param);
	static void check_session_timeout_broken_server(void * param);

	static void kill_trigger(void * param);

	void		on_begin_video(int32_t codec, uint8_t * extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps);
	void		on_recv_video(uint8_t* bytes, int32_t nbytes, long long pts, long long duration);
	void		on_end_video(void);

	void		on_begin_audio(int32_t codec, uint8_t * extradata, int32_t extradata_size, int32_t samplerate, int32_t channels);
	void		on_recv_audio(uint8_t* bytes, int32_t nbytes, long long pts, long long duration);
	void		on_end_audio(void);


	void		put_video_sample(int32_t codec, uint8_t * bytes, int32_t nbytes, long long pts);
	void		put_audio_sample(int32_t codec, uint8_t * bytes, int32_t nbytes, long long pts);

	static unsigned __stdcall process_video_cb(void * param);
	static unsigned __stdcall process_audio_cb(void * param);
	void		process_video(void);
	void		process_audio(void);


private:
	RTSPReceiver*				_front;
	int32_t						_transport_option;
	int32_t						_recv_option;

	bool *						_kill_flag;
	int32_t						_kill_trigger;

	Authenticator *				_auth;
	MediaSession *				_media_session;

	MediaSubsessionIterator *	_iter;

	TaskToken					_session_timer_task;
	TaskToken					_arrival_check_timer_task;
	TaskToken					_inter_packet_gap_check_timer_task;
	TaskToken					_session_timeout_broken_server_task;

	int32_t						_socket_input_buffer_size;
	bool						_made_progress;
	unsigned					_session_timeout_parameter;
	//bool _repeat;

	unsigned					_inter_packet_gap_max_time;
	unsigned					_total_packets_received;

	double						_duration;
	double						_duration_slot;
	double						_init_seek_time;
	char *						_init_abs_seek_time;
	float						_scale;
	double						_end_time;


	bool						_shutting_down;
	bool						_wait_teardown_response;
	bool						_send_keepalives_to_broken_servers;

	uint8_t						_vps[100];
	uint8_t						_sps[100];
	uint8_t						_pps[100];	
	int32_t						_vps_size;
	int32_t						_sps_size;
	int32_t						_pps_size;

	int32_t						_width;
	int32_t						_height;

	uint8_t						_audio_extradata[100];
	int32_t						_audio_extradata_size;
	int32_t						_audio_channels;
	int32_t						_audio_samplerate;

	BOOL						_video_run;
	BOOL						_audio_run;
	HANDLE						_video_thread;
	HANDLE						_audio_thread;
	HANDLE						_video_event;
	HANDLE						_audio_event;

	CRITICAL_SECTION			_video_lock;
	CRITICAL_SECTION			_audio_lock;

	CRITICAL_SECTION			_time_lock;
	long long					_start_time;

	std::vector<std::shared_ptr<RTSPReceiver::Core::sample_t>> _vqueue;
	std::vector<std::shared_ptr<RTSPReceiver::Core::sample_t>> _aqueue;
};
