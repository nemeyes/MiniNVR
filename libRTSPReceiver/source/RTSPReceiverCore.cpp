#include <winsock2.h>
#include <windows.h>
#include "RTSPReceiverCore.h"
#include "RTSPReceiver.h"
#include "H264BufferSink.h"
#include "H265BufferSink.h"
#include "AACBufferSink.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <MiniNVRLocks.h>
#include <H2645BitstreamParser.h>

#define AV_RB32(x)  \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) | \
               (((const uint8_t*)(x))[1] << 16) | \
               (((const uint8_t*)(x))[2] <<  8) | \
                ((const uint8_t*)(x))[3])

#define MIN(a,b) ((a) > (b) ? (b) : (a))

RTSPReceiver::Core * RTSPReceiver::Core::createNew(RTSPReceiver * front, UsageEnvironment & env, const char * url, const char * username, const char * password, int transport_option, int recv_option, int recv_timeout, float scale, unsigned int http_port_number, bool * kill_flag)
{
	return new RTSPReceiver::Core(front, env, url, username, password, transport_option, recv_option, recv_timeout, scale, http_port_number, kill_flag);
}

RTSPReceiver::Core::Core(RTSPReceiver* front, UsageEnvironment& env, const char* url, const char* username, const char* password, int transport_option, int recv_option, int recv_timeout, float scale, unsigned int http_port_number, bool* kill_flag)
	: RTSPClient(env, url, 1, "RTP/RTSP Client", http_port_number, -1)
	, _front(front)
	, _kill_flag(kill_flag)
	, _kill_trigger(0)
	, _auth(0)
	, _media_session(0)
	, _iter(0)
	, _session_timer_task(0)
	, _arrival_check_timer_task(0)
	, _inter_packet_gap_check_timer_task(0)
	, _session_timeout_broken_server_task(0)
	, _socket_input_buffer_size(0)
	, _made_progress(false)
	, _session_timeout_parameter(60)
	, _inter_packet_gap_max_time(recv_timeout)
	, _total_packets_received(~0)
	, _duration(0.0)
	, _duration_slot(-1.0)
	, _init_seek_time(0.0f)
	, _init_abs_seek_time(0)
	, _scale(scale)
	, _end_time(0)
	, _shutting_down(false)
	, _wait_teardown_response(false)
	, _send_keepalives_to_broken_servers(true)
	, _video_run(FALSE)
	, _audio_run(FALSE)
	, _video_thread(INVALID_HANDLE_VALUE)
	, _audio_thread(INVALID_HANDLE_VALUE)
	, _video_event(INVALID_HANDLE_VALUE)
	, _audio_event(INVALID_HANDLE_VALUE)
	, _start_time(-1)
	, _width(0)
	, _height(0)
{
    _transport_option = transport_option;
    _recv_option = recv_option;

	_kill_trigger = envir().taskScheduler().createEventTrigger((TaskFunc*)&(RTSPReceiver::Core::kill_trigger));
	//_task_sched = &_env->taskScheduler();
	if (username && password && strlen(username)>0 && strlen(password)>0)
        _auth = new Authenticator(username, password);

	::InitializeCriticalSection(&_video_lock);
	::InitializeCriticalSection(&_audio_lock);
	::InitializeCriticalSection(&_time_lock);
	_video_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	_audio_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	_video_run = TRUE;
	_video_thread = (HANDLE)::_beginthreadex(NULL, 0, RTSPReceiver::Core::process_video_cb, this, 0, NULL);
	_audio_run = TRUE;
	_audio_thread = (HANDLE)::_beginthreadex(NULL, 0, RTSPReceiver::Core::process_audio_cb, this, 0, NULL);
}

RTSPReceiver::Core::~Core(void)
{
	_video_run = FALSE;
	::SetEvent(_video_event);
	if (_video_thread != NULL && _video_thread != INVALID_HANDLE_VALUE)
	{
		if (::WaitForSingleObject(_video_thread, INFINITE) == WAIT_OBJECT_0)
		{
			::CloseHandle(_video_thread);
			_video_thread = INVALID_HANDLE_VALUE;
		}
	}

	_audio_run = FALSE;
	::SetEvent(_audio_event);
	if (_audio_thread != NULL && _audio_thread != INVALID_HANDLE_VALUE)
	{
		if (::WaitForSingleObject(_audio_thread, INFINITE) == WAIT_OBJECT_0)
		{
			::CloseHandle(_audio_thread);
			_audio_thread = INVALID_HANDLE_VALUE;
		}
	}

	::CloseHandle(_audio_event);
	::CloseHandle(_video_event);
	::DeleteCriticalSection(&_time_lock);
	::DeleteCriticalSection(&_video_lock);
	::DeleteCriticalSection(&_audio_lock);
	if (_auth)
	{
		delete _auth;
		_auth = 0;
	}
}

void RTSPReceiver::Core::get_options(RTSPClient::responseHandler * after_func)
{
    sendOptionsCommand(after_func, _auth);
}

void RTSPReceiver::Core::get_description(RTSPClient::responseHandler * after_func)
{
    sendDescribeCommand(after_func, _auth);
}

void RTSPReceiver::Core::setup_media_subsession(MediaSubsession * media_subsession, bool rtp_over_tcp, bool force_multicast_unspecified, RTSPClient::responseHandler * after_func)
{
    sendSetupCommand(*media_subsession, after_func, False, rtp_over_tcp?True:False, force_multicast_unspecified?True:False, _auth);
}

void RTSPReceiver::Core::start_playing_session(MediaSession * media_session, double start, double end, float scale, RTSPClient::responseHandler * after_func)
{
    sendPlayCommand(*media_session, after_func, start, end, scale, _auth);
}

void RTSPReceiver::Core::start_playing_session(MediaSession * media_session, const char * abs_start_time, const char * abs_end_time, float scale, RTSPClient::responseHandler * after_func)
{
    sendPlayCommand(*media_session, after_func, abs_start_time, abs_end_time, scale, _auth);
}

void RTSPReceiver::Core::start_pausing_session(void)
{
	sendPauseCommand(*_media_session, continue_after_pause, _auth);
}

void RTSPReceiver::Core::teardown_session(MediaSession * media_session, RTSPClient::responseHandler * after_func)
{
    sendTeardownCommand(*media_session, after_func, _auth);
}

void RTSPReceiver::Core::set_user_agent_string(const char * user_agent)
{
    setUserAgentString(user_agent);
}

void RTSPReceiver::Core::continue_after_client_creation(RTSPClient * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    self->set_user_agent_string("Magnetar.AI RTSP Client");
    self->get_options(continue_after_options);
}

void RTSPReceiver::Core::continue_after_options(RTSPClient * param, int result_code, char * result_string)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    delete [] result_string;
    self->get_description(continue_after_describe);
}

void RTSPReceiver::Core::continue_after_describe(RTSPClient * param, int result_code, char * result_string)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);

	do
	{
		if (result_code != 0)
		{
			delete[] result_string;
			break;
		}

		char * sdp_description = result_string;
		self->_media_session = MediaSession::createNew(self->envir(), sdp_description);
		delete[] sdp_description;

		if (!self->_media_session)
			break;
		if (!self->_media_session->hasSubsessions())
			break;

		MediaSubsessionIterator iter(*self->_media_session);
		MediaSubsession * media_subsession;

		bool made_progress = false;
		while ((media_subsession = iter.next()) != 0)
		{
			if ((self->_recv_option & RTSPReceiver::MEDIA_TYPE_T::AUDIO) && !(self->_recv_option & RTSPReceiver::MEDIA_TYPE_T::VIDEO))
			{
				if (strcmp(media_subsession->mediumName(), "audio")!=0)
					continue;
			}
			if ((self->_recv_option & RTSPReceiver::MEDIA_TYPE_T::VIDEO) && !(self->_recv_option & RTSPReceiver::MEDIA_TYPE_T::AUDIO))
			{
				if (strcmp(media_subsession->mediumName(), "video") != 0)
					continue;
			}
			if (media_subsession->initiate(-1))
			{
				//if( media_subsession->rtcpIsMuxed() )
				made_progress = true;
				if (media_subsession->rtpSource())
				{
					unsigned const threshold = 1000000; //1sec
					media_subsession->rtpSource()->setPacketReorderingThresholdTime(threshold);
					int fd = media_subsession->rtpSource()->RTPgs()->socketNum();
					if (self->_socket_input_buffer_size>0)
					{
						setReceiveBufferTo(self->envir(), fd, self->_socket_input_buffer_size);
					}
				}
			}
		}

		if (!made_progress)
			break;

		self->setup_streams();
		return;
	} while (0);

#if 0
	self->shutdown();
#else
	self->close();
#endif
}

void RTSPReceiver::Core::continue_after_setup(RTSPClient * param, int result_code, char * result_string)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);

    if( result_code==0 )
        self->_made_progress = true;

    delete [] result_string;

    self->_session_timeout_parameter = self->sessionTimeoutParameter();
    self->setup_streams();
}

void RTSPReceiver::Core::continue_after_play(RTSPClient * param, int result_code, char * result_string)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);

	do
	{
		if (result_code)
		{
			delete[] result_string;
			break;
		}
		delete[] result_string;

		// double delay_secs = self->_duration;
		if (self->_duration>0.0)
		{
			// First, adjust "duration" based on any change to the play range (that was specified in the "PLAY" response):
			double range_adjustment = (self->_media_session->playEndTime() - self->_media_session->playStartTime()) - (self->_end_time - self->_init_seek_time);
			if ((self->_duration + range_adjustment)>0.0)
			{
				self->_duration += range_adjustment;
			}

			double abs_scale = self->_scale>0 ? self->_scale : -(self->_scale);
			double delay_in_second = self->_duration / abs_scale + self->_duration_slot;

			long long delay_in_millisec = (long long)(delay_in_second*1000000.0);
			self->_session_timer_task = self->envir().taskScheduler().scheduleDelayedTask(delay_in_millisec, (TaskFunc*)session_timer_handler, (void*)0);
		}

		self->_session_timeout_broken_server_task = 0;
		//watch for incoming packet
		self->check_packet_arrival(self);

		if (self->_inter_packet_gap_max_time>0)
			self->check_inter_packet_gaps(self);
		self->check_session_timeout_broken_server(self);
		return;

	} while (0);

#if 0
	self->shutdown();
#else
	self->close();
#endif
}

void RTSPReceiver::Core::continue_after_pause(RTSPClient * param, int result_code, char * result_string)
{

}

void RTSPReceiver::Core::continue_after_teardown(RTSPClient * param, int result_code, char * result_string)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);

    if( result_string )
        delete [] result_string;

	if (!self->_media_session)
		return;

	MediaSubsessionIterator iter(*self->_media_session);
	MediaSubsession * media_subsession;
	while ((media_subsession = iter.next()) != 0)
	{
		Medium::close(media_subsession->sink);
		media_subsession->sink = 0;
	}

    Medium::close( self->_media_session );
	
	self->_shutting_down = false;
	self->close();
}

void RTSPReceiver::Core::close(void)
{
	//envir().taskScheduler();
	envir().taskScheduler().triggerEvent(_kill_trigger, this);
}

void RTSPReceiver::Core::subsession_after_playing(void * param)
{
	MediaSubsession * media_subsession = static_cast<MediaSubsession*>(param);
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(media_subsession->miscPtr);

    Medium::close( media_subsession->sink );
    media_subsession->sink = 0;

    MediaSession & media_session = media_subsession->parentSession();
    MediaSubsessionIterator iter( media_session );
    while( (media_subsession=iter.next())!=0 )
    {
        if( media_subsession->sink ) //this subsession is still active
            return;
    }
	self->session_after_playing(self);
}

void RTSPReceiver::Core::subsession_bye_handler(void * param)
{
	MediaSubsession * media_subsession = static_cast<MediaSubsession*>(param);
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(media_subsession->miscPtr);

    //struct timeval time_now;
    //gettimeofday( &time_now, 0 );
    //unsigned diff_secs = time_now.tv_sec - start_time.tv_sec;
	self->subsession_after_playing(media_subsession);
}

void RTSPReceiver::Core::session_after_playing(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
#if 0
	self->shutdown();
#else
	self->close();
#endif
}

void RTSPReceiver::Core::setup_streams(void)
{
	//static MediaSubsessionIterator * iter = new MediaSubsessionIterator(*_media_session);
	if (!_iter)
		_iter = new MediaSubsessionIterator(*_media_session);
    MediaSubsession * media_subsession = 0;
	while ((media_subsession = _iter->next()) != 0)
    {
        if( media_subsession->clientPortNum()==0 )
            continue;
        if(_transport_option== RTSPReceiver::TRANSPORT_OPTION_T::RTP_OVER_TCP)
			setup_media_subsession(media_subsession, true, false, RTSPReceiver::Core::continue_after_setup);
        else
			setup_media_subsession(media_subsession, false, false, RTSPReceiver::Core::continue_after_setup);

		return;
    }

	if (_iter)
	{
		delete _iter;
		_iter = 0;
	}

	if(!_made_progress)
	{
		shutdown();
		return;
	}

	_made_progress = false;
	MediaSubsessionIterator iter2(*_media_session);
	while((media_subsession=iter2.next())!=0)
	{
		if(media_subsession->readSource()==0) // was not initiated
			continue;

		RTSPReceiver::BufferSink * bs = nullptr;
		if (!strcmp(media_subsession->mediumName(), "video"))
		{
			if(!strcmp(media_subsession->codecName(), "H265"))
			{
				char*			vpsspspps[3]		= { 0 };
				int32_t			vpsspspps_size[3]	= { 0 };
				uint8_t			extradata[MAX_PATH] = { 0 };
				const uint8_t	start_code[4]		= { 0x00, 0x00, 0x00, 0x01 };
				int32_t			extradata_size		= 0;

				uint32_t		num					= 0;
				SPropRecord * vpsRecord				= parseSPropParameterSets(media_subsession->fmtp_spropvps(), num);
				SPropRecord * spsRecord				= parseSPropParameterSets(media_subsession->fmtp_spropsps(), num);
				SPropRecord * ppsRecord				= parseSPropParameterSets(media_subsession->fmtp_sproppps(), num);

				if (vpsRecord && (vpsRecord->sPropLength > 0))
				{
					vpsspspps[0]		= (char*)vpsRecord->sPropBytes;
					vpsspspps_size[0]	= vpsRecord->sPropLength;

					::memmove(extradata + extradata_size, start_code, sizeof(start_code));
					::memmove(extradata + extradata_size + sizeof(start_code), vpsspspps[0], vpsspspps_size[0]);
					extradata_size += sizeof(start_code) + vpsspspps_size[0];
				}
				if (spsRecord && (spsRecord->sPropLength > 0))
				{
					vpsspspps[1] = (char*)spsRecord->sPropBytes;
					vpsspspps_size[1] = spsRecord->sPropLength;

					::memmove(extradata + extradata_size, start_code, sizeof(start_code));
					::memmove(extradata + extradata_size + sizeof(start_code), vpsspspps[1], vpsspspps_size[1]);
					extradata_size += sizeof(start_code) + vpsspspps_size[1];
				}
				if (ppsRecord && (ppsRecord->sPropLength > 0))
				{
					vpsspspps[2] = (char*)ppsRecord->sPropBytes;
					vpsspspps_size[2] = ppsRecord->sPropLength;

					::memmove(extradata + extradata_size, start_code, sizeof(start_code));
					::memmove(extradata + extradata_size + sizeof(start_code), vpsspspps[2], vpsspspps_size[2]);
					extradata_size += sizeof(start_code) + vpsspspps_size[2];
				}
				
				bs = H265BufferSink::createNew(this, envir(), vpsspspps[0], vpsspspps_size[0], vpsspspps[1], vpsspspps_size[1], vpsspspps[2], vpsspspps_size[2], 8 * 1024 * 1024);

				int32_t wdth = 0, hght = 0;
				H2645BitstreamParser::ParseSequenceParameterSet(RTSPReceiver::VIDEO_CODEC_T::HEVC, (const uint8_t*)vpsspspps[1], vpsspspps_size[1], wdth, hght);
				width(wdth);
				height(hght);
				on_begin_video(RTSPReceiver::VIDEO_CODEC_T::HEVC, extradata, extradata_size, wdth, hght, 60);

				delete [] vpsRecord;
				delete [] spsRecord;
				delete [] ppsRecord;

			}
			else if (!strcmp(media_subsession->codecName(), "H264"))
			{
				uint8_t *		spspps[2]			= { 0 };
				int32_t			spspps_size[2]		= { 0 };
				uint32_t		num_spspps			= 0;
				uint8_t			extradata[MAX_PATH] = { 0 };
				const uint8_t	start_code[4]		= { 0x00, 0x00, 0x00, 0x01 };
				int32_t			extradata_size		= 0;
				SPropRecord *	sPropRecord			= parseSPropParameterSets(media_subsession->fmtp_spropparametersets(), num_spspps);

				for (uint32_t i = 0; i < num_spspps; i++)
				{
					spspps[i] = sPropRecord[i].sPropBytes;
					spspps_size[i] = sPropRecord[i].sPropLength;

					if (spspps_size[i] > 0)
					{
						::memmove(extradata + extradata_size, start_code, sizeof(start_code));
						::memmove(extradata + extradata_size + sizeof(start_code), spspps[i], spspps_size[i]);
						extradata_size += sizeof(start_code) + spspps_size[i];
					}
				}

				bs = H264BufferSink::createNew(this, envir(), (const char*)spspps[0], spspps_size[0], (const char*)spspps[1], spspps_size[1], 4 * 1024 * 1024);
				delete[] sPropRecord;
				
				int32_t wdth = 0, hght = 0;
				H2645BitstreamParser::ParseSequenceParameterSet(RTSPReceiver::VIDEO_CODEC_T::AVC, (const uint8_t*)spspps[1], spspps_size[1], wdth, hght);
				width(wdth);
				height(hght);
				on_begin_video(RTSPReceiver::VIDEO_CODEC_T::AVC, extradata, extradata_size, wdth, hght, 60);
			}
			else if (!strcmp(media_subsession->codecName(), "MP4V-ES"))
			{

			}
		}
		else if(!strcmp(media_subsession->mediumName(), "audio"))
		{
			if (!strcmp(media_subsession->codecName(), "AMR") || !strcmp(media_subsession->codecName(), "AMR-WB"))
			{
				//bs = amr_buffer_sink::createNew(_front, envir(), media_subsession->fmtp_spropparametersets(), 512 * 1024);
			}
			else if (!strcmp(media_subsession->codecName(), "MPEG4-GENERIC") || !strcmp(media_subsession->codecName(), "MP4A-LATM")) //aac
			{
				int32_t frequencyFromconfig = (int32_t)samplingFrequencyFromAudioSpecificConfig(media_subsession->fmtp_config());
				uint32_t configStrSize = 0;
				uint8_t * configStr = parseGeneralConfigStr(media_subsession->fmtp_config(), configStrSize);
				bs = AACBufferSink::createNew(this, envir(), 512 * 1024, media_subsession->numChannels(), frequencyFromconfig, (char*)configStr, configStrSize);

				on_begin_audio(RTSPReceiver::AUDIO_CODEC_T::AAC, configStr, configStrSize, frequencyFromconfig, media_subsession->numChannels());
				if (configStr)
					delete configStr;
				configStr = NULL;
			}
		}

		media_subsession->sink = bs;
		if(media_subsession->sink)
		{
			media_subsession->miscPtr = (void*)this;
			media_subsession->sink->startPlaying(*(media_subsession->readSource()), subsession_after_playing, media_subsession);

			// also set a handler to be called if a RTCP "BYE" arrives
			// for this subsession:

			if (media_subsession->rtcpInstance() != NULL)
				media_subsession->rtcpInstance()->setByeHandler(subsession_bye_handler, media_subsession);
			_made_progress = true;
		}
	}

    if( _duration==0 )
    {
        if( _scale>0 )
            _duration = _media_session->playEndTime() - _init_seek_time;
        else if( _scale<0 )
            _duration = _init_seek_time;
    }
    if( _duration<0 )
        _duration = 0.0;

    _end_time = _init_seek_time;
    if( _scale>0 )
    {
        if( _duration<=0 )
            _end_time = -1.0f;
        else
            _end_time = _init_seek_time + _duration;
    }
    else
    {
        _end_time = _init_seek_time - _duration;
        if( _end_time<0 )
            _end_time = 0.0f;
    }

    const char * abs_start_time = _init_abs_seek_time?_init_abs_seek_time:_media_session->absStartTime();
    if( abs_start_time )
        start_playing_session( _media_session, abs_start_time, _media_session->absEndTime(), _scale, continue_after_play );
    else
        start_playing_session( _media_session, _init_seek_time, _end_time, _scale, continue_after_play );
}

void RTSPReceiver::Core::shutdown(void)
{
    if( _shutting_down )
        return;

    _shutting_down = true;

    envir().taskScheduler().unscheduleDelayedTask(_session_timer_task);
	envir().taskScheduler().unscheduleDelayedTask(_session_timeout_broken_server_task);
	envir().taskScheduler().unscheduleDelayedTask(_arrival_check_timer_task);
	envir().taskScheduler().unscheduleDelayedTask(_inter_packet_gap_check_timer_task);

    bool shutdown_immediately = true;
    if( _media_session )
    {
        RTSPClient::responseHandler * response_handler_for_teardown = 0;
        if( _wait_teardown_response )
        {
            shutdown_immediately = false;
            response_handler_for_teardown = continue_after_teardown;
        }
        teardown_session( _media_session, response_handler_for_teardown );
    }

    if( shutdown_immediately )
        continue_after_teardown(this, 0, 0 );
}

void RTSPReceiver::Core::session_timer_handler(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    self->_session_timer_task = 0;
	self->session_after_playing(self);
}

void RTSPReceiver::Core::check_packet_arrival(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    int delay_usec = 100000; //100 ms
	self->_arrival_check_timer_task = self->envir().taskScheduler().scheduleDelayedTask(delay_usec, (TaskFunc*)&RTSPReceiver::Core::check_packet_arrival, self);
}

void RTSPReceiver::Core::check_inter_packet_gaps(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    if( !self->_inter_packet_gap_max_time )
        return;

    unsigned total_packets_received = 0;

    MediaSubsessionIterator iter( *self->_media_session );
    MediaSubsession * media_subsession;
    while( (media_subsession=iter.next())!=0 )
    {
        RTPSource * src = media_subsession->rtpSource();
        if( !src )
            continue;

        total_packets_received += src->receptionStatsDB().totNumPacketsReceived();
    }

    if( total_packets_received==self->_total_packets_received )
    {
        self->_inter_packet_gap_check_timer_task = 0;
        self->session_after_playing(self);
    }
    else
    {
        self->_total_packets_received = total_packets_received;
		self->_inter_packet_gap_check_timer_task = self->envir().taskScheduler().scheduleDelayedTask(self->_inter_packet_gap_max_time * 1000000, (TaskFunc*)&RTSPReceiver::Core::check_inter_packet_gaps, self);
    }
}

void RTSPReceiver::Core::check_session_timeout_broken_server(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
    if( !self->_send_keepalives_to_broken_servers )
        return;

    if( self->_session_timeout_broken_server_task )
        self->get_options( 0 );

    unsigned session_timeout = self->_session_timeout_parameter;
    unsigned delay_sec_until_next_keepalive = session_timeout<=5?1:session_timeout-5;
    //reduce the interval a liteel, to be on the safe side

	self->_session_timeout_broken_server_task = self->envir().taskScheduler().scheduleDelayedTask(delay_sec_until_next_keepalive * 1000000, (TaskFunc*)&RTSPReceiver::Core::check_session_timeout_broken_server, self);
}

void RTSPReceiver::Core::kill_trigger(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
	self->_shutting_down = false;
	self->shutdown();

	if (self->_kill_flag)
		(*self->_kill_flag) = true;

	Medium::close(self);
}

uint8_t* RTSPReceiver::Core::get_vps(int32_t& vps_size)
{
	vps_size = _vps_size;
	return _sps;
}

uint8_t* RTSPReceiver::Core::get_sps(int32_t& sps_size)
{
	sps_size = _sps_size;
	return _sps;
}

uint8_t* RTSPReceiver::Core::get_pps(int32_t& pps_size)
{
	pps_size = _pps_size;
	return _pps;
}

void RTSPReceiver::Core::set_vps(uint8_t* vps, int32_t vps_size)
{
	::memset(_vps, 0x00, sizeof(_vps));
	::memmove(_vps, vps, vps_size);
	_vps_size = vps_size;
}

void RTSPReceiver::Core::set_sps(uint8_t* sps, int32_t sps_size)
{
	::memset(_sps, 0x00, sizeof(_sps));
	::memmove(_sps, sps, sps_size);
	_sps_size = sps_size;
}

void RTSPReceiver::Core::set_pps(uint8_t* pps, int32_t pps_size)
{
	::memset(_pps, 0x00, sizeof(_pps));
	::memmove(_pps, pps, pps_size);
	_pps_size = pps_size;
}

void RTSPReceiver::Core::set_audio_extradata(uint8_t* extradata, int32_t size)
{
	::memset(_audio_extradata, 0x00, sizeof(_audio_extradata));
	::memmove(_audio_extradata, extradata, size);
	_audio_extradata_size = size;
}

uint8_t* RTSPReceiver::Core::get_audio_extradata(int32_t& size)
{
	size = _audio_extradata_size;
	return _audio_extradata;
}

void RTSPReceiver::Core::set_audio_channels(int32_t channels)
{
	_audio_channels = channels;
}

int32_t	RTSPReceiver::Core::get_audio_channels(void)
{
	return _audio_channels;
}

void RTSPReceiver::Core::set_audio_samplerate(int32_t samplerate)
{
	_audio_samplerate = samplerate;
}

int32_t RTSPReceiver::Core::get_audio_samplerate(void)
{
	return _audio_samplerate;
}

BOOL RTSPReceiver::Core::is_vps(int32_t smt, uint8_t nal_unit_type)
{
	// VPS NAL units occur in H.265 only:
	return nal_unit_type == 32;
}

BOOL RTSPReceiver::Core::is_sps(int32_t smt, uint8_t nal_unit_type)
{
	if (smt == RTSPReceiver::VIDEO_CODEC_T::AVC)
		return nal_unit_type == 7;
	else
		return nal_unit_type == 33;
}

BOOL RTSPReceiver::Core::is_pps(int32_t smt, uint8_t nal_unit_type)
{
	if (smt == RTSPReceiver::VIDEO_CODEC_T::AVC)
		return nal_unit_type == 8;
	else
		return nal_unit_type == 34;
}

BOOL RTSPReceiver::Core::is_idr(int32_t smt, uint8_t nal_unit_type)
{
	if (smt == RTSPReceiver::VIDEO_CODEC_T::AVC)
		return nal_unit_type == 5;
	else
		return (nal_unit_type == 19) || (nal_unit_type == 20);
}

BOOL RTSPReceiver::Core::is_vlc(int32_t smt, uint8_t nal_unit_type)
{
	if (smt == RTSPReceiver::VIDEO_CODEC_T::AVC)
		return (nal_unit_type <= 5 && nal_unit_type > 0);
	else
		return (nal_unit_type <= 31);
}

void RTSPReceiver::Core::width(int32_t wdth)
{
	_width = wdth;
}

void RTSPReceiver::Core::height(int32_t hght)
{
	_height = hght;
}

int32_t RTSPReceiver::Core::width(void)
{
	return _width;
}

int32_t RTSPReceiver::Core::height(void)
{
	return _height;
}

void RTSPReceiver::Core::on_begin_video(int32_t codec, uint8_t * extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps)
{
	if (_front)
		_front->OnBeginVideo(codec, extradata, extradata_size, width, height, fps);
}

void RTSPReceiver::Core::on_recv_video(uint8_t * bytes, int32_t nbytes, long long pts, long long duration)
{
	if (_front)
		_front->OnRecvVideo(bytes, nbytes, pts, duration);
}

void RTSPReceiver::Core::on_end_video(void)
{
	if (_front)
		_front->OnEndVideo();
}

void RTSPReceiver::Core::on_begin_audio(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t samplerate, int32_t channels)
{
	if (_front)
		_front->OnBeginAudio(codec, extradata, extradata_size, samplerate, channels);
}

void RTSPReceiver::Core::on_recv_audio(uint8_t* bytes, int32_t nbytes, long long pts, long long duration)
{
	if (_front)
		_front->OnRecvAudio(bytes, nbytes, pts, duration);
}

void RTSPReceiver::Core::on_end_audio(void)
{
	if (_front)
		_front->OnEndAudio();
}


unsigned RTSPReceiver::Core::process_video_cb(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
	self->process_video();
	return 0;
}

unsigned RTSPReceiver::Core::process_audio_cb(void * param)
{
	RTSPReceiver::Core* self = static_cast<RTSPReceiver::Core*>(param);
	self->process_audio();
	return 0;
}

void RTSPReceiver::Core::put_video_sample(int32_t codec, uint8_t * bytes, int32_t nbytes, long long pts)
{
	if (!_video_run)
		return;

	std::shared_ptr<RTSPReceiver::Core::sample_t> sample = std::shared_ptr<RTSPReceiver::Core::sample_t>(new RTSPReceiver::Core::sample_t(codec, bytes, nbytes, pts));
	{
		Autolock lock(&_video_lock);
		_vqueue.push_back(sample);
		::SetEvent(_video_event);
	}
}

void RTSPReceiver::Core::put_audio_sample(int32_t codec, uint8_t* bytes, int32_t nbytes, long long pts)
{
	if (!_audio_run)
		return;

	std::shared_ptr<RTSPReceiver::Core::sample_t> sample = std::shared_ptr<RTSPReceiver::Core::sample_t>(new RTSPReceiver::Core::sample_t(codec, bytes, nbytes, pts));
	{
		Autolock lock(&_audio_lock);
		_aqueue.push_back(sample);
		::SetEvent(_audio_event);
	}
}

void RTSPReceiver::Core::process_video(void)
{
	BOOL change_vps			= FALSE;
	BOOL change_sps			= FALSE;
	BOOL change_pps			= FALSE;
	BOOL recvFirstIDR		= FALSE;
	//long long videoStartTime= -1;

	//HANDLE file = ::CreateFile(L"sld.265", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	while (_video_run)
	{
		if (::WaitForSingleObject(_video_event, INFINITE) == WAIT_OBJECT_0)
		{
			Autolock lock(&_video_lock);
			while (_vqueue.size() >= 2)
			{
				int32_t codec = _vqueue[0]->codec;
				uint8_t * bytes = _vqueue[0]->bytes;
				int32_t nbytes = _vqueue[0]->nbytes;
				long long pts = _vqueue[0]->pts;
				long long next_pts = _vqueue[1]->pts;
				long long duration = next_pts - pts;

#if 1
				/*{
					solids::lib::autolock lock(&_time_lock);
					if (_start_time == -1)
						_start_time = pts;
					_front->on_recv_video(bytes, nbytes, pts - _start_time, duration);
				}*/
				{
					Autolock lock(&_time_lock);
					if (_start_time == -1)
						_start_time = pts;
				}

				//uint32_t bytes_written = 0;
				//uint8_t* temp = bytes;
				//do
				//{
				//	uint32_t nb_write = 0;
				//	::WriteFile(file, temp, nbytes, (LPDWORD)&nb_write, 0);
				//	bytes_written += nb_write;
				//	if (nbytes == bytes_written)
				//		break;
				//} while (1);

				//_front->on_recv_video(bytes, nbytes, pts - _start_time, 166777i64);
				_front->OnRecvVideo(bytes, nbytes, pts - _start_time, 166666i64);
#else
				if (codec == RTSPReceiver::video_codec_t::avc)
				{
					int32_t saved_sps_size = 0;
					unsigned char* saved_sps = get_sps(saved_sps_size);
					int32_t saved_pps_size = 0;
					unsigned char* saved_pps = get_pps(saved_pps_size);

					BOOL is_sps = RTSPReceiver::core::is_sps(codec, bytes[4] & 0x1F);
					if (is_sps)
					{
						if (saved_sps_size < 1 || !saved_sps)
						{
							set_sps(bytes, nbytes);
							change_sps = TRUE;
						}
						else
						{
							if (memcmp(saved_sps, bytes, saved_sps_size))
							{
								set_sps(bytes, nbytes);
								change_sps = TRUE;
							}
						}
					}

					BOOL is_pps = RTSPReceiver::core::is_pps(codec, bytes[4] & 0x1F);
					if (is_pps)
					{
						if (saved_pps_size < 1 || !saved_pps)
						{
							set_pps(bytes, nbytes);
							change_pps = TRUE;
						}
						else
						{
							if (memcmp(saved_pps, bytes, saved_pps_size))
							{
								set_pps(bytes, nbytes);
								change_pps = TRUE;
							}
						}
					}

					BOOL is_idr = RTSPReceiver::core::is_idr(codec, bytes[4] & 0x1F);
					if (is_idr)
					{
						uint8_t extradata[MAX_PATH] = { 0 };
						::memmove(extradata, saved_sps, saved_sps_size);
						::memmove(extradata + saved_sps_size, saved_pps, saved_pps_size);
						if (change_sps || change_pps)
						{
							if (!recvFirstIDR)
							{
								{
									solids::lib::autolock lock(&_time_lock);
									if (_start_time == -1)
										_start_time = pts;
								}
								//_front->on_begin_video(codec, extradata, saved_sps_size + saved_pps_size);
								recvFirstIDR = TRUE;
								change_sps = FALSE;
								change_pps = FALSE;
							}
						}
						else
						{
							if (!recvFirstIDR)
							{
								{
									solids::lib::autolock lock(&_time_lock);
									if (_start_time == -1)
										_start_time = pts;
								}
								//_front->on_begin_video(codec, extradata, saved_sps_size + saved_pps_size);
								recvFirstIDR = TRUE;
							}
						}
					}

					if (recvFirstIDR)
					{
						//_front->on_recv_video(bytes, nbytes, pts - _start_time, 416666i64/*166777i64*/);
					}
				}
				else if (codec == RTSPReceiver::video_codec_t::hevc)
				{
					int32_t saved_vps_size = 0;
					int32_t saved_sps_size = 0;
					int32_t saved_pps_size = 0;
					unsigned char* saved_vps = get_vps(saved_vps_size);
					unsigned char* saved_sps = get_sps(saved_sps_size);
					unsigned char* saved_pps = get_pps(saved_pps_size);
					BOOL is_vps = RTSPReceiver::core::is_vps(codec, (bytes[4] >> 1) & 0x3F);
					if (is_vps)
					{
						if (saved_vps_size < 1 || !saved_vps)
						{
							set_vps(bytes, nbytes);
							change_vps = TRUE;
						}
						else
						{
							if (memcmp(saved_vps, bytes, saved_vps_size))
							{
								set_vps(bytes, nbytes);
								change_vps = TRUE;
							}
						}
					}

					BOOL is_sps = RTSPReceiver::core::is_sps(codec, (bytes[4] >> 1) & 0x3F);
					if (is_sps)
					{
						if (saved_sps_size < 1 || !saved_sps)
						{
							set_sps(bytes, nbytes);
							change_sps = TRUE;
						}
						else
						{
							if (memcmp(saved_sps, bytes, saved_sps_size))
							{
								set_sps(bytes, nbytes);
								change_sps = TRUE;
							}
						}
					}

					BOOL is_pps = RTSPReceiver::core::is_pps(codec, (bytes[4] >> 1) & 0x3F);
					if (is_pps)
					{
						if (saved_pps_size < 1 || !saved_pps)
						{
							set_pps(bytes, nbytes);
							change_pps = TRUE;
						}
						else
						{
							if (memcmp(saved_pps, bytes, saved_pps_size))
							{
								set_pps(bytes, nbytes);
								change_pps = TRUE;
							}
						}
					}

					BOOL is_idr = RTSPReceiver::core::is_idr(codec, (bytes[4] >> 1) & 0x3F);
					if (is_idr)
					{
						uint8_t extradata[MAX_PATH] = { 0 };
						::memmove(extradata, saved_vps, saved_vps_size);
						::memmove(extradata + saved_vps_size, saved_sps, saved_sps_size);
						::memmove(extradata + saved_vps_size + saved_sps_size, saved_pps, saved_pps_size);
						if (change_vps || change_sps || change_pps)
						{
							if (!recvFirstIDR)
							{
								{
									solids::lib::autolock lock(&_time_lock);
									if(_start_time==-1)
										_start_time = pts;
								}
								_front->on_begin_video(codec, extradata, saved_vps_size + saved_sps_size + saved_pps_size);
								recvFirstIDR = TRUE;
								change_vps = FALSE;
								change_sps = FALSE;
								change_pps = FALSE;
							}
						}
						else
						{
							if (!recvFirstIDR)
							{
								{
									solids::lib::autolock lock(&_time_lock);
									if (_start_time == -1)
										_start_time = pts;
								}
								_front->on_begin_video(codec, extradata, saved_vps_size + saved_sps_size + saved_pps_size);
								recvFirstIDR = TRUE;
							}
						}
					}

					if (recvFirstIDR)
					{
						_front->on_recv_video(bytes, nbytes, pts - _start_time, duration);
					}
				}
#endif
				_vqueue.erase(_vqueue.begin());
			}
		}
	}

	{
		Autolock lock(&_video_lock);
		_vqueue.clear();
	}

	//::CloseHandle(file);

}

void RTSPReceiver::Core::process_audio(void)
{
	//BOOL recvFirstAudio		= FALSE;
	//long long audioStartTime= -1;
	while (_audio_run)
	{
		if (::WaitForSingleObject(_audio_event, INFINITE) == WAIT_OBJECT_0)
		{
			Autolock lock(&_audio_lock);
			while (_aqueue.size() >= 2)
			{
				int32_t codec = _aqueue[0]->codec;
				uint8_t* bytes = _aqueue[0]->bytes;
				int32_t nbytes = _aqueue[0]->nbytes;
				long long pts = _aqueue[0]->pts;
				long long next_pts = _aqueue[1]->pts;
				long long duration = next_pts - pts;

#if 1
				/*{
					solids::lib::autolock lock(&_time_lock);
					if (_start_time == -1)
						_start_time = pts;
				}
				_front->on_recv_audio(bytes, nbytes, pts - _start_time, 232199i64);*/

				{
					
					Autolock lock(&_time_lock);
					if (_start_time == -1)
						_start_time = pts;
				}
				_front->OnRecvAudio(bytes, nbytes, pts - _start_time, 232199i64);
#else
				if (codec == RTSPReceiver::audio_codec_t::aac)
				{
					if (!recvFirstAudio)
					{
						int32_t extradata_size = 0;
						uint8_t* extradata = get_audio_extradata(extradata_size);
						int32_t channels = get_audio_channels();
						int32_t samplerate = get_audio_samplerate();

						{
							solids::lib::autolock lock(&_time_lock);
							if (_start_time == -1)
								_start_time = pts;
						}
						_front->on_begin_audio(codec, extradata, extradata_size, samplerate, channels);
						recvFirstAudio = TRUE;
					}
					
					if(recvFirstAudio)
					{
						_front->on_recv_audio(bytes, nbytes, pts - _start_time, 232199i64);
					}
				}
#endif
				_aqueue.erase(_aqueue.begin());
			}
		}
	}

	{
		Autolock lock(&_audio_lock);
		_aqueue.clear();
	}
}