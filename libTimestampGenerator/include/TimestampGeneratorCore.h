#pragma once

#include "TimestampGenerator.h"
# include <windows.h>

class TimestampGenerator::Core
{
public:
	Core(void);
	virtual ~Core(void);

	void		begin_elapsed_time(void);
	long long	elapsed_microseconds(void);
	long long	elapsed_milliseconds(void);

private:
	LARGE_INTEGER	_frequency;
	LARGE_INTEGER	_begin_elapsed_microseconds;
	BOOL			_begin;
};