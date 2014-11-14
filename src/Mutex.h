/*
 * Mutex.h
 *
 *  Created on: 2013. 1. 5.
 *      Author: oskwon
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>
//----------------------------------------------------------------------

class Mutex
{
protected:
	pthread_mutex_t mLockHandle;

public:
	Mutex()
	{
		pthread_mutex_init(&mLockHandle, NULL);
	}
	virtual ~Mutex()
	{
		pthread_mutex_destroy(&mLockHandle);
	}

	int lock()
	{
		return pthread_mutex_lock(&mLockHandle);
	}
	int try_lock()
	{
		if (pthread_mutex_trylock(&mLockHandle) == 16)
			return -1;
		return 0;
	}
	int unlock()
	{
		return pthread_mutex_unlock(&mLockHandle);
	}
};
//----------------------------------------------------------------------

class SingleLock
{
protected:
	Mutex* mMutexHandle;

public:
	SingleLock(Mutex* mutex)
	{
		mMutexHandle = mutex;
		mMutexHandle->try_lock();
	}

	~SingleLock()
	{
		mMutexHandle->unlock();
	}
};
//----------------------------------------------------------------------

#endif

