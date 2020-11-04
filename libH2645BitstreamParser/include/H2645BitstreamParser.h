#pragma once

#if defined(EXPORT_H2645_BITSTREAM_PARSER_LIB)
#  define EXP_H2645_BITSTREAM_PARSER_CLASS __declspec(dllexport)
#else
#  define EXP_H2645_BITSTREAM_PARSER_CLASS __declspec(dllimport)
#endif

#include <MiniNVR.h>

class EXP_H2645_BITSTREAM_PARSER_CLASS H2645BitstreamParser
	: public Base
{
	class Core;
public:
	H2645BitstreamParser(void);
	~H2645BitstreamParser(void);

	static void ParseVideoParameterSet(int32_t video_codec, const uint8_t * vps, int32_t vpssize);
	static void ParseSequenceParameterSet(int32_t video_codec, const uint8_t * sps, int32_t spssize, int32_t & width, int32_t & height);
};
