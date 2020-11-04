#pragma once

#include <MiniNVR.h>

# if defined(EXPORT_TIMESTAMP_GENERATOR_LIB)
#  define EXP_TIMESTAMP_GENERATOR_CLASS __declspec(dllexport)
# else
#  define EXP_TIMESTAMP_GENERATOR_CLASS __declspec(dllimport)
# endif

class EXP_TIMESTAMP_GENERATOR_CLASS TimestampGenerator
{
	class Core;
public:
	TimestampGenerator(void);
	~TimestampGenerator(void);

	void		BeginElapsedTime(void);
	long long	ElapsedMicroseconds(void);
	long long	ElapsedMilliseconds(void);


private:
	TimestampGenerator::Core * _generator;

};