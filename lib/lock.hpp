#pragma once

#include <atomic>
#include <mutex>

#include "folly/RWSpinLock.h"

class rwlock_base
{
public:
	virtual void LockRead() = 0;
	virtual void UnlockRead() = 0;
	virtual void LockWrite() = 0;
	virtual void UnlockWrite() = 0;
};

class SpinLock
{
private:
	enum LockState
	{
		Locked,
		Unlocked
	};
	
	std::atomic<LockState> _state;

public:
	SpinLock() : _state(Unlocked)
	{
	
	}
	
	void lock()
	{
		while (_state.exchange(Locked, std::memory_order_acquire) == Locked)
		{
		
		}
	}
	
	void unlock()
	{
		_state.store(Unlocked, std::memory_order_release);
	}
};

class RWLock : rwlock_base
{
public:
	void LockRead() override
	{
		std::unique_lock<std::mutex> lock(_m);
		while (writerUsed == true)
		{
			_cv.wait(lock);
		}
		++readerCount;
	}
	
	void LockWrite() override
	{
		std::unique_lock<std::mutex> lock(_m);
		while (readerCount != 0 || writerUsed == true)
		{
			_cv.wait(lock);
		}
		writerUsed = true;
	}
	
	void UnlockRead() override
	{
		std::unique_lock<std::mutex> lock(_m);
		--readerCount;
		if (readerCount == 0)
		{
			_cv.notify_all();
		}
	}
	
	void UnlockWrite() override
	{
		std::unique_lock<std::mutex> lock(_m);
		writerUsed = false;
		_cv.notify_all();
	}

private:
	int readerCount = 0;
	bool writerUsed = false;
	std::mutex _m;
	std::condition_variable _cv;
};

class SpinRWLock : rwlock_base
{
public:
	void LockRead() override
	{
		_lock.lock_shared();
	}
	
	void UnlockRead() override
	{
		_lock.unlock_shared();
	}
	
	void LockWrite() override
	{
		_lock.lock();
	}
	
	void UnlockWrite() override
	{
		_lock.unlock();
	}

private:
	folly::RWSpinLock _lock;
};

template <class T_Mutex>
class lock_guard_read
{
public:
	using mutex_type = T_Mutex;
	
	explicit lock_guard_read(T_Mutex& mutex) : _mutex(mutex)
	{
		_mutex.LockRead();
	}
	
	~lock_guard_read() noexcept
	{
		_mutex.UnlockRead();
	}
	
	lock_guard_read(const lock_guard_read&) = delete;
	lock_guard_read& operator=(const lock_guard_read&) = delete;

private:
	T_Mutex& _mutex;
};

template <class T_Mutex>
class lock_guard_write
{
public:
	using mutex_type = T_Mutex;
	
	explicit lock_guard_write(T_Mutex& mutex) : _mutex(mutex)
	{
		_mutex.LockWrite();
	}
	
	~lock_guard_write() noexcept
	{
		_mutex.UnlockWrite();
	}
	
	lock_guard_write(const lock_guard_write&) = delete;
	lock_guard_write& operator=(const lock_guard_write&) = delete;

private:
	T_Mutex& _mutex;
};