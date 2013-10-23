/*
 * uThread.cpp
 *
 *  Created on: 2013. 9. 10.
 *      Author: kos
 */

#include "uThread.h"

#include <stdio.h>
#include <pthread.h>

//#include "uLogger.h"

using namespace std;
//-------------------------------------------------------------------------------

uThread::uThread(std::string aName, uThread::EXIT_TYPE aExitType)
	: mTid(0), mState(uThread::STATE_READY), mExitType(aExitType), mName(aName)
{
}
//-------------------------------------------------------------------------------

uThread::~uThread()
{
	if(mState == uThread::STATE_ZOMBIE) {
		Join(mTid);
	}
}
//-------------------------------------------------------------------------------

bool uThread::Start()
{
#ifdef SUPPORT_PTHREAD_ATTR
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(mExitType == uThread::TYPE_JOINABLE) {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	} else {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
#endif /* SUPPORT_PTHREAD_ATTR */
	if(pthread_create(&mTid, NULL, ThreadMain, reinterpret_cast<void *>(this)) == 0) {
#ifndef SUPPORT_PTHREAD_ATTR
		if(mExitType == uThread::TYPE_DETACHABLE) {
			pthread_detach(mTid);
		}
#endif /* !SUPPORT_PTHREAD_ATTR */
	} else {
		SetState(uThread::STATE_ABORTED);
#ifdef SUPPORT_PTHREAD_ATTR
		pthread_attr_destroy(&attr);
#endif /* SUPPORT_PTHREAD_ATTR */
		return false;
	}
	//cout << mTid << endl;
#ifdef SUPPORT_PTHREAD_ATTR
	pthread_attr_destroy(&attr);
#endif /* SUPPORT_PTHREAD_ATTR */
	return true;
}
//-------------------------------------------------------------------------------

void* uThread::ThreadMain(void* aParam)
{
	uThread* thiz = reinterpret_cast<uThread*>(aParam);

	std::string threadName = thiz->GetName();
	//INFO("start thread...[%s]", threadName.c_str());

	thiz->SetState(uThread::STATE_RUNNING);
	thiz->Run();

	if(thiz->GetExitType() == uThread::TYPE_DETACHABLE)
		 thiz->SetState(uThread::STATE_TERMINATED);
	else thiz->SetState(uThread::STATE_ZOMBIE);

	pthread_exit((void*)0);

	//INFO("terminated thread...[%s]", threadName.c_str());
	return NULL;
}
//-------------------------------------------------------------------------------

void uThread::Stop()
{
	switch(mState) {
		case uThread::STATE_RUNNING: {
			switch(mExitType) {
				case uThread::TYPE_DETACHABLE: {
					Terminate();
				} break;
				case uThread::TYPE_JOINABLE: {
					Terminate();
					Join(mTid);
				} break;
			}
		} break;
			case uThread::STATE_ZOMBIE: {
			Join(mTid);
		} break;
	}
}
//-------------------------------------------------------------------------------

bool uThread::Join(pthread_t aTid)
{
	if (!pthread_join(aTid, NULL)) {
		SetState(uThread::STATE_TERMINATED);
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------------

void uThread::SetState(uThread::STATE aState)
{
	mState = aState;
}
//-------------------------------------------------------------------------------

uThread::STATE uThread::GetState() const
{
	return mState;
}
//-------------------------------------------------------------------------------

uThread::EXIT_TYPE uThread::GetExitType() const
{
	return mExitType;
}
//-------------------------------------------------------------------------------

std::string uThread::GetName() const
{
	return mName;
}
//-------------------------------------------------------------------------------

bool uThread::IsTerminated() const
{
	return (mState == uThread::STATE_TERMINATED) ? true : false;
}
//-------------------------------------------------------------------------------

bool uThread::IsRunning() const
{
	return mState == uThread::STATE_RUNNING  ?  true : false;
}
//-------------------------------------------------------------------------------

bool uThread::Join()
{
	return Join(mTid);
}
//-------------------------------------------------------------------------------
