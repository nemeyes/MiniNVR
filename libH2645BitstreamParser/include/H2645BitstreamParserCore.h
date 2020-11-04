#pragma once

#include "H2645BitstreamParser.h"
#include "bitvector.h"

class H2645BitstreamParser::Core
{
public:
	Core(void);
	~Core(void);

	static void ParseVideoParameterSet(int32_t video_codec, const uint8_t * vps, int32_t vpssize);
	static void ParseSequenceParameterSet(int32_t video_codec, const uint8_t * sps, int32_t spssize, int32_t & width, int32_t & height);


private:
	static void	ProfileTierLevel(BitVector & bv, unsigned max_sub_layers_minus1);
private:
	static uint8_t		_vps[1000];
	static int32_t		_vps_size;
	static uint8_t		_sps[1000];
	static int32_t		_sps_size;
};