#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
#include <libavutil/avassert.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <FragmentedMP4Muxer.h>
#include <TimestampGenerator.h>

#include <ppl.h>
#include <concurrent_queue.h>

class FragmentedMP4Muxer::Core
{
public:
	typedef struct _queue_elem_t
	{
		int32_t		type;
		uint8_t*	data;
		int32_t		size;
		int64_t		timestamp;
		_queue_elem_t(int32_t size)
			: type(FragmentedMP4Muxer::MEDIA_TYPE_T::UNKNOWN)
			, data(nullptr)
			, size(size)
			, timestamp(0)
		{
			data = (uint8_t*)malloc(size);
			if (data != nullptr)
			{
				::memset(data, 0x00, size);
			}
		}
		~_queue_elem_t(void)
		{
			if (data)
			{
				free(data);
				data = nullptr;
			}
			size = 0;
		}
		_queue_elem_t(const _queue_elem_t& clone)
		{
			type = clone.type;
			data = clone.data;
			size = clone.size;
			timestamp = clone.timestamp;
		}
		_queue_elem_t& operator=(const _queue_elem_t& clone)
		{
			type = clone.type;
			data = clone.data;
			size = clone.size;
			timestamp = clone.timestamp;
			return (*this);
		}
	} queue_elem_t;

	typedef struct _cmd_type_t
	{
		static const int put_video = 0;
		static const int put_audio = 1;
	} cmd_type_t;

	typedef struct _msg_t
	{
		int32_t		msg_type;
		char		path[MAX_PATH];
		std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t> msg_elem;
	} msg_t;

	typedef struct _media_thread_context_t
	{
		char		path[MAX_PATH];
		FragmentedMP4Muxer::Core* parent;
	} media_thread_context_t;

	Core(FragmentedMP4Muxer* front);
	~Core(void);

	BOOL	is_initialized(void);

	int32_t initialize(FragmentedMP4Muxer::CONTEXT_T* context);
	int32_t release(void);

	int32_t put_video_stream(uint8_t* bytes, int32_t nbytes, int64_t timestamp);
	int32_t put_audio_stream(uint8_t* bytes, int32_t nbytes, int64_t timestamp);

private:
	void	fill_video_stream(AVStream* vs, int32_t id);
	void	fill_audio_stream(AVStream* as, int32_t id);
	BOOL	is_directory_exist(LPCTSTR path);
	void	create_directory_recursively(std::wstring& path);
	int32_t	create_container_context(AVFormatContext* ctx, char* path, int32_t option, AVOutputFormat** format, AVStream** vstream, AVStream** astream);
	void	destroy_container_context(AVFormatContext** ctx, AVOutputFormat** format, AVStream** vstream, AVStream** astream);

	static unsigned __stdcall process_cb(void* param);
	void	process(void);

private:
	FragmentedMP4Muxer*				_front;
	FragmentedMP4Muxer::CONTEXT_T*	_context;
	BOOL							_is_initialized;
	HANDLE							_thread;
	BOOL							_run;
	CRITICAL_SECTION				_lock;

	Concurrency::concurrent_queue<std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t>>	_queue;

	LARGE_INTEGER					_frequency;
	LARGE_INTEGER					_begin_elapsed_microseconds;

	HANDLE							_event;

	TimestampGenerator				_timestamp_generator;
};
