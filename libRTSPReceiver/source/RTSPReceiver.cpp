#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include "RTSPReceiver.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "RTSPReceiverCore.h"

RTSPReceiver::RTSPReceiver(void)
	: _ignore_sdp(TRUE)
{
	WSADATA wsd;
	WSAStartup( MAKEWORD(2,2), &wsd );
}

RTSPReceiver::~RTSPReceiver(void)
{
	OnEndVideo();
	OnEndAudio();

	WSACleanup();
}

int32_t RTSPReceiver::Play(const char * url, const char * username, const char * password, int32_t transport_option, int32_t recv_option, int32_t recv_timeout, float scale, bool repeat)
{
    if( !url || strlen(url)<1 )
		return RTSPReceiver::ERR_CODE_T::GENERIC_FAIL;

	memset(_url, 0x00, sizeof(_url));
	memset(_username, 0x00, sizeof(_username));
	memset(_password, 0x00, sizeof(_password));
	if (strlen(url)>0)
		strcpy_s(_url, url);
	if (username && strlen(username)>0)
		strcpy_s(_username, username);
	if (password && strlen(password)>0)
		strcpy_s(_password, password);
	_transport_option = transport_option;
	_recv_option = recv_option;
	_recv_timeout = recv_timeout;
	_scale = scale;
	_repeat = repeat;

	unsigned int thread_id;
	_thread = (HANDLE)::_beginthreadex(0, 0, RTSPReceiver::ProcessCB, this, 0, &thread_id);
	return RTSPReceiver::ERR_CODE_T::SUCCESS;
}

int32_t RTSPReceiver::Stop(void)
{
	if (!_kill )
	{
		_repeat = false;
		_kill = true;
		if (_live)
			_live->close();
	}
	return RTSPReceiver::ERR_CODE_T::SUCCESS;
}

int32_t RTSPReceiver::Pause(void)
{
	if (!_kill)
	{
		if (_live)
		{
			_live->start_pausing_session();
		}
	}
	return RTSPReceiver::ERR_CODE_T::SUCCESS;
}

int32_t RTSPReceiver::Width(void)
{
	if (_live)
		return _live->width();
	return 0;
}

int32_t RTSPReceiver::Height(void)
{
	if (_live)
		return _live->height();
	return 0;
}

void RTSPReceiver::OnBeginVideo(int32_t codec, uint8_t * extradata, int32_t extradata_size, int32_t width, int32_t height, int32_t fps)
{

}

void RTSPReceiver::OnRecvVideo(uint8_t* bytes, int32_t nbytes, long long pts, long long duration)
{

}

void RTSPReceiver::OnEndVideo(void)
{

}

void RTSPReceiver::OnBeginAudio(int32_t codec, uint8_t* extradata, int32_t extradata_size, int32_t samplerate, int32_t channels)
{

}

void RTSPReceiver::OnRecvAudio(uint8_t * bytes, int32_t nbytes, long long pts, long long duration)
{

}

void RTSPReceiver::OnEndAudio(void)
{

}

void RTSPReceiver::Process(void)
{
	do
	{
		TaskScheduler * sched = BasicTaskScheduler::createNew();
		UsageEnvironment * env = BasicUsageEnvironment::createNew(*sched);
		if (strlen(_username) > 0 && strlen(_password) > 0)
			_live = RTSPReceiver::Core::createNew(this, *env, _url, _username, _password, _transport_option, _recv_option, _recv_timeout, _scale, 0, &_kill);
		else
			_live = RTSPReceiver::Core::createNew(this, *env, _url, 0, 0, _transport_option, _recv_option, _recv_timeout, _scale, 0, &_kill);

		_kill = false;
		RTSPReceiver::Core::continue_after_client_creation(_live);
		env->taskScheduler().doEventLoop((char*)&_kill);

		if (env)
		{
			env->reclaim();
			env = 0;
		}
		if (sched)
		{
			delete sched;
			sched = 0;
		}
	} while (_repeat);
}

unsigned __stdcall RTSPReceiver::ProcessCB(void * param)
{
	RTSPReceiver* self = static_cast<RTSPReceiver*>(param);
	self->Process();
	return 0;
}