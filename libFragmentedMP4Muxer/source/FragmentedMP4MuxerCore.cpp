#include "FragmentedMP4MuxerCore.h"
#include <MiniNVRLocks.h>
#include <MiniNVRStringHelper.h>
#include <process.h>

FragmentedMP4Muxer::Core::Core(FragmentedMP4Muxer* front)
	: _front(front)
	, _context(nullptr)
	, _is_initialized(FALSE)
	, _thread(INVALID_HANDLE_VALUE)
	, _run(FALSE)
{
	_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	::InitializeCriticalSection(&_lock);
}

FragmentedMP4Muxer::Core::~Core(void)
{
	::DeleteCriticalSection(&_lock);
	::CloseHandle(_event);
}

BOOL FragmentedMP4Muxer::Core::is_initialized(void)
{
	return _is_initialized;
}

int32_t FragmentedMP4Muxer::Core::initialize(FragmentedMP4Muxer::CONTEXT_T* context)
{
	if (!(context->option & FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO) && !(context->option & FragmentedMP4Muxer::MEDIA_TYPE_T::AUDIO))
		return FragmentedMP4Muxer::ERR_CODE_T::GENERIC_FAIL;

	Autolock lock(&_lock);

	_timestamp_generator.BeginElapsedTime();
	_context = context;
	_run = TRUE;
	_thread = (HANDLE)::_beginthreadex(NULL, 0, FragmentedMP4Muxer::Core::process_cb, this, 0, NULL);
	::SetEvent(_event);
	_is_initialized = TRUE;
	return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;
}

int32_t FragmentedMP4Muxer::Core::release(void)
{
	Autolock lock(&_lock);

	_is_initialized = FALSE;
	_run = FALSE;
	::SetEvent(_event);
	if (_thread != NULL && _thread != INVALID_HANDLE_VALUE)
	{
		if (::WaitForSingleObject(_thread, INFINITE) == WAIT_OBJECT_0)
		{
			::CloseHandle(_thread);
			_thread = INVALID_HANDLE_VALUE;
		}
	}
	_queue.clear();
	return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;
}

int32_t FragmentedMP4Muxer::Core::put_video_stream(uint8_t* bytes, int32_t nbytes, int64_t timestamp)
{
	if (!_is_initialized || !bytes || nbytes < 1)
		return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;

	Autolock lock(&_lock);

	std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t> videoQueueElem = std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t>(new FragmentedMP4Muxer::Core::queue_elem_t(nbytes));
	videoQueueElem->type = FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO;
	::memmove(videoQueueElem->data, bytes, nbytes);
	videoQueueElem->size = nbytes;
	if (timestamp == -1)
		videoQueueElem->timestamp = _timestamp_generator.ElapsedMicroseconds();
	else
		videoQueueElem->timestamp = timestamp;

	_queue.push(videoQueueElem);

	return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;
}

int32_t FragmentedMP4Muxer::Core::put_audio_stream(uint8_t* bytes, int32_t nbytes, int64_t timestamp)
{
	if (!_is_initialized || !bytes || nbytes < 1)
		return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;

	Autolock lock(&_lock);

	std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t> audioQueueElem = std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t>(new FragmentedMP4Muxer::Core::queue_elem_t(nbytes));
	audioQueueElem->type = FragmentedMP4Muxer::MEDIA_TYPE_T::AUDIO;
	::memmove(audioQueueElem->data, bytes, nbytes);
	audioQueueElem->size = nbytes;
	if (timestamp == -1)
		audioQueueElem->timestamp = _timestamp_generator.ElapsedMicroseconds();
	else
		audioQueueElem->timestamp = timestamp;

	_queue.push(audioQueueElem);
	return FragmentedMP4Muxer::ERR_CODE_T::SUCCESS;
}

void FragmentedMP4Muxer::Core::process(void)
{
	std::wstring str_muxing_dst_path = _context->muxingDstPath;
	if (wcslen(_context->muxingDstPath) > 0 && !is_directory_exist(_context->muxingDstPath))
		create_directory_recursively(str_muxing_dst_path);

	AVFormatContext*	format_ctx = NULL;
	AVOutputFormat*		format = NULL;
	AVStream*			video_stream = NULL;
	AVStream*			audio_stream = NULL;

	int32_t				status = 0;

	wchar_t				muxing_dst_path_begin[MAX_PATH] = { 0 };
	wchar_t				muxing_dst_path_end[MAX_PATH] = { 0 };

	AVRational			bq;
	uint8_t*			vbytes = NULL;
	int32_t				nvbytes = 0;
	int64_t				vpts = 0;
	int64_t				vdts = 0;
	int64_t				vduration = 0;

	uint8_t*			abytes = NULL;
	int32_t				nabytes = 0;
	int64_t				apts = 0;
	int64_t				adts = 0;
	int64_t				aduration = 0;

	long long			start_time = -1;
	long long			begin_time = -1;
	long long			end_time = -1;
	int64_t				frame_count = 0;

	std::shared_ptr<FragmentedMP4Muxer::Core::queue_elem_t> queueElem = NULL;
	while (_run)
	{
		if (::WaitForSingleObject(_event, INFINITE) == WAIT_OBJECT_0)
		{
			while (_queue.try_pop(queueElem))
			{
				end_time = queueElem->timestamp;

				if (_context->generationRule == FragmentedMP4Muxer::GENERATION_RULE_T::FULL)
				{
					if (start_time == -1)
					{
						start_time = queueElem->timestamp;
						begin_time = start_time;
						_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);

						char* muxingDstPath1 = NULL;
						StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
						if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
						{
							create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
							free(muxingDstPath1);
						}
					}
				}
				else if (_context->generationRule == FragmentedMP4Muxer::GENERATION_RULE_T::TIME)
				{
					if (start_time == -1)
					{
						start_time = queueElem->timestamp;
						begin_time = start_time;
						_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
						char* muxingDstPath1 = NULL;
						StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
						if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
						{
							create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
							free(muxingDstPath1);
						}
					}
					else
					{
						if (format_ctx == NULL)
						{
							begin_time = end_time;
							_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
							char* muxingDstPath1 = NULL;
							StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
							if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
							{
								create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
								free(muxingDstPath1);
							}
						}
						else
						{
							int64_t diff = queueElem->timestamp - start_time;
							if (diff > (_context->generationThreshold * 1000))
							{
								destroy_container_context(&format_ctx, &format, &video_stream, &audio_stream);
								_snwprintf_s(muxing_dst_path_end, sizeof(muxing_dst_path_end), L"%s\\%lld_%lld.mp4", _context->muxingDstPath, begin_time, end_time);
								::MoveFile(muxing_dst_path_begin, muxing_dst_path_end);

								begin_time = end_time;
								_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
								char* muxingDstPath1 = NULL;
								StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
								if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
								{
									create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
									free(muxingDstPath1);
								}
							}
						}
					}

				}
				else if (_context->generationRule == FragmentedMP4Muxer::GENERATION_RULE_T::FRAME)
				{
					if (queueElem->type == FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO)
					{
						if (frame_count == 0)
						{
							start_time = queueElem->timestamp;
							begin_time = start_time;
							_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
							char* muxingDstPath1 = NULL;
							StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
							if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
							{
								create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
								free(muxingDstPath1);
							}
						}
						else
						{
							if (format_ctx == NULL)
							{
								begin_time = end_time;
								_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
								char* muxingDstPath1 = NULL;
								StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
								if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
								{
									create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
									free(muxingDstPath1);
								}
							}
							else
							{
								if ((frame_count+1) % (_context->generationThreshold) == 0)
								{
									destroy_container_context(&format_ctx, &format, &video_stream, &audio_stream);
									_snwprintf_s(muxing_dst_path_end, sizeof(muxing_dst_path_end), L"%s\\%lld_%lld.mp4", _context->muxingDstPath, begin_time, end_time);
									::MoveFile(muxing_dst_path_begin, muxing_dst_path_end);

									begin_time = end_time;
									_snwprintf_s(muxing_dst_path_begin, sizeof(muxing_dst_path_begin), L"%s\\%lld.mp4", _context->muxingDstPath, begin_time);
									char* muxingDstPath1 = NULL;
									StringHelper::ConvertWide2Multibyte(muxing_dst_path_begin, &muxingDstPath1);
									if (muxingDstPath1 && strlen(muxingDstPath1) > 0)
									{
										create_container_context(format_ctx, muxingDstPath1, _context->option, &format, &video_stream, &audio_stream);
										free(muxingDstPath1);
									}
								}
							}
						}
					}
				}
				else if (_context->generationRule == FragmentedMP4Muxer::GENERATION_RULE_T::SIZE)
				{

				}


				switch (queueElem->type)
				{
				case FragmentedMP4Muxer::MEDIA_TYPE_T::VIDEO:
				{
					uint8_t* bitstream = NULL;
					int32_t bitstreamSize = 0;

					vbytes = queueElem->data;
					nvbytes = queueElem->size;

					bq.num = 1;
					bq.den = 1000000;
					vpts = av_rescale_q(queueElem->timestamp - start_time, bq, video_stream->time_base);
					vdts = vpts;
					vduration = av_rescale_q(1, bq, video_stream->time_base);

					AVPacket pkt;
					av_init_packet(&pkt);
					pkt.pts = vpts;
					pkt.dts = pkt.pts;
					pkt.duration = vduration;

					pkt.pos = -1;
					pkt.stream_index = video_stream->index;
					pkt.data = vbytes;
					pkt.size = nvbytes;
					status = av_write_frame(format_ctx, &pkt);
					av_packet_unref(&pkt);

					frame_count++;
					queueElem = NULL;

					if (bitstream && bitstreamSize > 0)
					{
						free(bitstream);
						bitstream = NULL;
						bitstreamSize = 0;
					}
					break;
				}
				case FragmentedMP4Muxer::MEDIA_TYPE_T::AUDIO:
				{
					abytes = queueElem->data;
					nabytes = queueElem->size;

					bq.num = 1;
					bq.den = 1000000;
					apts = av_rescale_q(queueElem->timestamp - start_time, bq, audio_stream->time_base);
					adts = apts;
					aduration = av_rescale_q(1, bq, audio_stream->time_base);

					AVPacket pkt;
					av_init_packet(&pkt);
					pkt.pts = apts;
					pkt.dts = adts;
					pkt.duration = aduration;

					pkt.pos = -1;
					pkt.stream_index = audio_stream->index;
					pkt.data = abytes;
					pkt.size = nabytes;
					status = av_write_frame(format_ctx, &pkt);
					av_packet_unref(&pkt);

					queueElem = NULL;
					break;
				}
				}
			}
		}
	}

	if (queueElem)
		queueElem = NULL;

	destroy_container_context(&format_ctx, &format, &video_stream, &audio_stream);
	_snwprintf_s(muxing_dst_path_end, sizeof(muxing_dst_path_end), L"%s\\%lld_%lld.mp4", _context->muxingDstPath, begin_time, end_time);
	::MoveFile(muxing_dst_path_begin, muxing_dst_path_end);
}

int32_t FragmentedMP4Muxer::Core::create_container_context(AVFormatContext* ctx, char* path, int32_t option, AVOutputFormat** format, AVStream** vstream, AVStream** astream)
{
	int32_t status = avformat_alloc_output_context2(&ctx, NULL, NULL, path);
	if (status < 0 || !ctx)
		return status;

	int32_t stream_size = 0;
	if ((option & MEDIA_TYPE_T::VIDEO) && (option & MEDIA_TYPE_T::AUDIO))
		stream_size = 2;
	else
		stream_size = 1;

	(*format) = ctx->oformat;
	for (int32_t i = 0; i < stream_size; i++)
	{
		if ((option & MEDIA_TYPE_T::VIDEO) && (option & MEDIA_TYPE_T::AUDIO))
		{
			if (i == 0)
			{
				(*vstream) = avformat_new_stream(ctx, NULL);
				fill_video_stream((*vstream), i);
			}
			else
			{
				(*astream) = avformat_new_stream(ctx, NULL);
				fill_audio_stream((*astream), i);
			}
		}
		else if (_context->option & MEDIA_TYPE_T::VIDEO)
		{
			(*vstream) = avformat_new_stream(ctx, NULL);
			fill_video_stream((*vstream), i);
		}
		else
		{
			(*astream) = avformat_new_stream(ctx, NULL);
			fill_audio_stream((*astream), i);
		}
	}
	av_dump_format(ctx, 0, path, 1);
	status = avio_open(&ctx->pb, path, AVIO_FLAG_WRITE);
	status = avformat_write_header(ctx, NULL);

	return status;
}

void FragmentedMP4Muxer::Core::destroy_container_context(AVFormatContext** ctx, AVOutputFormat** format, AVStream** vstream, AVStream** astream)
{
	if ((*vstream) && (*vstream)->codecpar->extradata_size > 0)
	{
		av_free((*vstream)->codecpar->extradata);
		(*vstream)->codecpar->extradata = NULL;
	}
	if ((*astream) && (*astream)->codecpar->extradata_size > 0)
	{
		av_free((*astream)->codecpar->extradata);
		(*astream)->codecpar->extradata = NULL;
	}

	if ((*ctx))
	{
		av_write_trailer((*ctx));
		avio_close((*ctx)->pb);
		avformat_free_context((*ctx));
	}

	(*ctx) = NULL;
	(*format) = NULL;
	(*vstream) = NULL;
	(*astream) = NULL;
}

BOOL FragmentedMP4Muxer::Core::is_directory_exist(LPCTSTR path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void FragmentedMP4Muxer::Core::create_directory_recursively(std::wstring & path)
{
	static const std::wstring separators(L"\\/");

	DWORD fileAttributes = ::GetFileAttributesW(path.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		std::size_t slashIndex = path.find_last_of(separators);
		if (slashIndex != std::wstring::npos)
		{
			std::wstring path2 = path.substr(0, slashIndex);
			create_directory_recursively(path2);
		}
		::CreateDirectoryW(path.c_str(), nullptr);
	}
}

void FragmentedMP4Muxer::Core::fill_video_stream(AVStream* vs, int32_t id)
{
	vs->id = id;
	vs->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	switch (_context->videoCodec)
	{
	case FragmentedMP4Muxer::VIDEO_CODEC_T::AVC:
		vs->codecpar->codec_id = AV_CODEC_ID_H264;
		break;
	case FragmentedMP4Muxer::VIDEO_CODEC_T::HEVC:
		vs->codecpar->codec_id = AV_CODEC_ID_HEVC;
		break;
	}
	if (!_context->ignoreVideoExtradata && _context->videoExtradataSize > 0)
	{
		vs->codecpar->extradata_size = _context->videoExtradataSize;
		vs->codecpar->extradata = static_cast<uint8_t*>(av_malloc(_context->videoExtradataSize));
		::memmove(vs->codecpar->extradata, _context->videoExtradata, vs->codecpar->extradata_size);
	}
	vs->codecpar->width = _context->videoWidth;
	vs->codecpar->height = _context->videoHeight;
	//vs->r_frame_rate.num = _context->videoFPSNum;
	//vs->r_frame_rate.den = _context->videoFPSDen;
	//vs->time_base.num = _context->video_tb_num;
	//vs->time_base.den = _context->video_tb_den;
	vs->time_base.num = 1;
	vs->time_base.den = 1000000;

	//vs->codecpar->bits_per_raw_sample = 8;
	//vs->codecpar->profile = 100;
	//vs->codecpar->level = 40;
	//vs->codecpar->field_order = AV_FIELD_PROGRESSIVE;
	vs->codecpar->codec_tag = 0;
}

void FragmentedMP4Muxer::Core::fill_audio_stream(AVStream* as, int32_t id)
{
	as->id = id;
	as->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
	switch (_context->audioCodec)
	{
	case FragmentedMP4Muxer::AUDIO_CODEC_T::AAC:
		as->codecpar->codec_id = AV_CODEC_ID_AAC;
		break;
	case FragmentedMP4Muxer::AUDIO_CODEC_T::AC3:
		as->codecpar->codec_id = AV_CODEC_ID_AC3;
		break;
	case FragmentedMP4Muxer::AUDIO_CODEC_T::MP3:
		as->codecpar->codec_id = AV_CODEC_ID_MP3;
		break;
	case FragmentedMP4Muxer::AUDIO_CODEC_T::OPUS:
		as->codecpar->codec_id = AV_CODEC_ID_OPUS;
		break;
	}

	if (!_context->ignoreAudioExtradata && (_context->audioExtradataSize > 0))
	{
		as->codecpar->extradata_size = _context->audioExtradataSize;
		as->codecpar->extradata = static_cast<uint8_t*>(av_malloc(_context->audioExtradataSize));
		::memmove(as->codecpar->extradata, _context->audioExtradata, as->codecpar->extradata_size);
	}

	as->codecpar->sample_rate = _context->audioSamplerate;

	switch (_context->audioSampleFormat)
	{
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::U8:
		as->codecpar->format = AV_SAMPLE_FMT_U8;
		as->codecpar->bits_per_coded_sample = 8;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S16:
		as->codecpar->format = AV_SAMPLE_FMT_S16;
		as->codecpar->bits_per_coded_sample = 16;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S32:
		as->codecpar->format = AV_SAMPLE_FMT_S32;
		as->codecpar->bits_per_coded_sample = 32;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::FLT:
		as->codecpar->format = AV_SAMPLE_FMT_FLT;
		as->codecpar->bits_per_coded_sample = 32;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::DBL:
		as->codecpar->format = AV_SAMPLE_FMT_DBL;
		as->codecpar->bits_per_coded_sample = 64;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S64:
		as->codecpar->format = AV_SAMPLE_FMT_S64;
		as->codecpar->bits_per_coded_sample = 64;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::U8P:
		as->codecpar->format = AV_SAMPLE_FMT_U8P;
		as->codecpar->bits_per_coded_sample = 8;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S16P:
		as->codecpar->format = AV_SAMPLE_FMT_S16P;
		as->codecpar->bits_per_coded_sample = 16;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S32P:
		as->codecpar->format = AV_SAMPLE_FMT_S32P;
		as->codecpar->bits_per_coded_sample = 32;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::FLTP:
		as->codecpar->format = AV_SAMPLE_FMT_FLTP;
		as->codecpar->bits_per_coded_sample = 16;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::DBLP:
		as->codecpar->format = AV_SAMPLE_FMT_DBLP;
		as->codecpar->bits_per_coded_sample = 64;
		break;
	case FragmentedMP4Muxer::AUDIO_SAMPLE_T::S64P:
		as->codecpar->format = AV_SAMPLE_FMT_S64P;
		as->codecpar->bits_per_coded_sample = 64;
		break;
	}
	as->codecpar->channels = _context->audioChannels;
	as->codecpar->frame_size = _context->audioFrameSize;
	as->time_base.num = 1;
	as->time_base.den = 1000000;

	//as->time_base.num = _context->audio_tb_num;
	//as->time_base.den = _context->audio_tb_den;
	//as->codecpar->format = 8;
	//as->codecpar->profile = 1;
	as->codecpar->codec_tag = 0;
}

unsigned FragmentedMP4Muxer::Core::process_cb(void* param)
{
	FragmentedMP4Muxer::Core* self = static_cast<FragmentedMP4Muxer::Core*>(param);
	self->process();
	return 0;
}