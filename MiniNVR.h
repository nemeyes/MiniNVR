#pragma once

#include <winsock2.h>
#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <algorithm>
#include <array>
#include <map>
#include <stack>
#include <vector>
#include <tuple>
#include <codecvt>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <d3d11_4.h>


//#if defined(_DEBUG) && !defined(WITH_DISABLE_VLD)
//#include <vld.h>
//#endif

#define MN_MAX(A, B)                           (((A) > (B)) ? (A) : (B))
#define MN_MIN(A, B)                           (((A) < (B)) ? (A) : (B))

#define MN_ALIGN16(value)                      (((value + 15) >> 4) << 4) // round up to a multiple of 16
#define MN_ALIGN32(value)                      (((value + 31) >> 5) << 5) // round up to a multiple of 32
#define MN_ALIGN(value, alignment)             (alignment) * ( (value) / (alignment) + (((value) % (alignment)) ? 1 : 0))

#define MN_BREAK_ON_ERROR(P)                   {if (MFX_ERR_NONE != (P)) break;}
#define MN_SAFE_DELETE_ARRAY(P)                {if (P) {delete[] P; P = NULL;}}
#define MN_SAFE_RELEASE(X)                     {if (X) { X->Release(); X = NULL; }}
#define MN_SAFE_FREE(X)                        {if (X) { free(X); X = NULL; }}
#define MN_SAFE_DELETE(P)                      {if (P) {delete P; P = NULL;}}

template <class T> void SafeRelease(T*& pt)
{
	if (pt != NULL)
	{
		pt->Release();
		pt = NULL;
	}
}

class Base
{
public:
	typedef struct _ERR_CODE_T
	{
		static const int32_t UNKNOWN = -1;
		static const int32_t SUCCESS = 0;
		static const int32_t GENERIC_FAIL = 1;
		static const int32_t INVALID_PARAMETER = 2;
		static const int32_t INVALID_FILE_PATH = 3;
		static const int32_t UNSUPPORTED_MEDIA_FILE = 4;
	} ERR_CODE_T;

	typedef struct _MEDIA_TYPE_T
	{
		static const int32_t UNKNOWN = 0x00;
		static const int32_t VIDEO = 0x01;
		static const int32_t AUDIO = 0x02;
	} MEDIA_TYPE_T;

	typedef struct _VIDEO_CODEC_T
	{
		static const int32_t UNKNOWN = -1;
		static const int32_t VP6 = 0;
		static const int32_t AVC = 1;
		static const int32_t MP4V = 2;
		static const int32_t HEVC = 3;
		static const int32_t VP8 = 4;
		static const int32_t VP9 = 5;
	} VIDEO_CODEC_T;

	typedef struct _COLORSPACE_T
	{
		static const int32_t NV12 = 0;
		static const int32_t BGRA = 1;
		static const int32_t YV12 = 2;
		static const int32_t I420 = 3;
	} COLORSPACE_T;

	typedef struct _AUDIO_CODEC_T
	{
		static const int32_t UNKNOWN = -1;
		static const int32_t MP3 = 0;
		static const int32_t ALAW = 1;
		static const int32_t MLAW = 2;
		static const int32_t AAC = 3;
		static const int32_t AC3 = 4;
		static const int32_t OPUS = 5;
	} AUDIO_CODEC_T;

	typedef struct _AUDIO_SAMPLE_T
	{
		static const int32_t UNKNOWN = -1;
		static const int32_t U8 = 0;
		static const int32_t S16 = 1;
		static const int32_t S32 = 2;
		static const int32_t FLT = 3;
		static const int32_t DBL = 4;
		static const int32_t S64 = 5;
		static const int32_t U8P = 6;
		static const int32_t S16P = 7;
		static const int32_t S32P = 8;
		static const int32_t FLTP = 9;
		static const int32_t DBLP = 10;
		static const int32_t S64P = 11;
	} AUDIO_SAMPLE_T;
};
