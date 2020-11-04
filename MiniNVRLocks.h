#pragma once

class Autolock
{
public:
	Autolock(CRITICAL_SECTION* lock)
		: _lock(lock)
	{
		::EnterCriticalSection(_lock);
	}

	~Autolock(void)
	{
		::LeaveCriticalSection(_lock);
	}
private:
	CRITICAL_SECTION* _lock;
};


class Scopedlock
{
public:
	Scopedlock(HANDLE lock)
		: _lock(lock)
	{
		if (_lock == NULL || _lock == INVALID_HANDLE_VALUE)
			return;
		::WaitForSingleObject(_lock, INFINITE);
	}

	~Scopedlock(void)
	{
		if (_lock == NULL || _lock == INVALID_HANDLE_VALUE)
			return;

		::SetEvent(_lock);
	}

private:
	HANDLE _lock;

private:
	Scopedlock(void);
	Scopedlock(const Scopedlock & clone);

};

class ExclusiveScopedlock
{
public:
	ExclusiveScopedlock(SRWLOCK* lock)
		: _lock(lock)
	{
		::AcquireSRWLockExclusive(_lock);
	}

	~ExclusiveScopedlock(void)
	{
		ReleaseSRWLockExclusive(_lock);
	}

private:
	SRWLOCK* _lock;

private:
	ExclusiveScopedlock(void);
	ExclusiveScopedlock(const ExclusiveScopedlock & clone);
};

class SharedScopedlock
{
public:
	SharedScopedlock(SRWLOCK* lock)
		: _lock(lock)
	{
		::AcquireSRWLockShared(_lock);
	}

	~SharedScopedlock(void)
	{
		::ReleaseSRWLockShared(_lock);
	}

private:
	SRWLOCK* _lock;

private:
	SharedScopedlock(void);
	SharedScopedlock(const SharedScopedlock& clone);
};