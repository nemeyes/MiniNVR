#include "TimestampGenerator.h"
#include "TimestampGeneratorCore.h"

TimestampGenerator::TimestampGenerator(void)
{
	_generator = new TimestampGenerator::Core();
}

TimestampGenerator::~TimestampGenerator(void)
{
	if (_generator)
		delete _generator;
	_generator = nullptr;
}

void TimestampGenerator::BeginElapsedTime(void)
{
	if (_generator)
		_generator->begin_elapsed_time();
}

long long TimestampGenerator::ElapsedMicroseconds(void)
{
	if (_generator)
		return _generator->elapsed_microseconds();
	return 0;
}

long long TimestampGenerator::ElapsedMilliseconds(void)
{
	if (_generator)
		return _generator->elapsed_milliseconds();
	return 0;
}

