#include "H2645BitstreamParser.h"
#include "H2645BitstreamParserCore.h"

H2645BitstreamParser::H2645BitstreamParser(void)
{

}

H2645BitstreamParser::~H2645BitstreamParser(void)
{

}

void H2645BitstreamParser::ParseVideoParameterSet(int32_t video_codec, const uint8_t * vps, int32_t vpssize)
{
	H2645BitstreamParser::Core::ParseVideoParameterSet(video_codec, vps, vpssize);
}

void H2645BitstreamParser::ParseSequenceParameterSet(int32_t video_codec, const uint8_t * sps, int32_t spssize, int32_t & width, int32_t & height)
{
	H2645BitstreamParser::Core::ParseSequenceParameterSet(video_codec, sps, spssize, width, height);
}