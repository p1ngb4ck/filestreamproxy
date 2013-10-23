/*
 * uThread.h
 *
 *  Created on: 2013. 9. 10.
 *      Author: kos
 */

#ifndef UTHREAD_H_
#define UTHREAD_H_

#include <string>
#include <pthread.h>
//-------------------------------------------------------------------------------

class uThread
{
public:
	enum STATE {
		STATE_READY = 0,
		STATE_RUNNING,
		STATE_TERMINATED,
		STATE_ZOMBIE,
		STATE_ABORTED
	};
	enum EXIT_TYPE {
		TYPE_JOINABLE = 0,
		TYPE_DETACHABLE
	};
private:
	uThread(const uThread&);
	uThread& operator=(const uThread&);

	uThread::STATE mState;
	uThread::EXIT_TYPE mExitType;
protected:
	pthread_t mTid;
	std::string mName;

	static void* ThreadMain(void *aParam);

	virtual void Run() = 0;
	virtual void Terminate() = 0;

	bool Join(pthread_t aTid);
	void SetState(STATE aState);

	void SetName(std::string aName) { mName = aName; }
public:
	uThread(std::string aName="", uThread::EXIT_TYPE aExitType=uThread::TYPE_JOINABLE);
	virtual ~uThread();

	void Stop();
	bool Start();

	pthread_t GetTid() { return mTid; }
	uThread::STATE GetState() const;
	uThread::EXIT_TYPE GetExitType() const;
	std::string GetName() const;

	bool IsTerminated() const;
	bool IsRunning() const;
	bool Join();
};
//-------------------------------------------------------------------------------

#endif /* UTHREAD_H_ */
