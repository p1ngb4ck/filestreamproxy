/*
 * eDemuxPumpThread.h
 *
 *  Created on: 2013. 10. 29.
 *      Author: kos
 */

#ifndef EDEMUXPUMPTHREAD_H_
#define EDEMUXPUMPTHREAD_H_

#include <vector>
#include <string>

#include "uThread.h"
//-------------------------------------------------------------------------------

namespace eDemuxState {
	enum {
		stNotOpened = 0,
		stOpened,
		stSetedFilter,
		END
	};
};
//-------------------------------------------------------------------------------

class eDemuxPumpThread : public uThread
{
private:
	int mDemuxFd, mDeviceFd, mState;
	bool mTermFlag;

	std::string mErrMessage;

	std::vector<unsigned long> mPidList;
protected:
	void Run();
	void Terminate();
public:
	eDemuxPumpThread();
	virtual ~eDemuxPumpThread();

	bool Open(int aDemuxId);
	void Close();

	int GetState() { return mState; }
	int GetDemuxFd() { return mDemuxFd; }
	std::string GetMessage() { return mErrMessage; }

	void SetDeviceFd(int aDeviceFd) { mDeviceFd = aDeviceFd; }

	bool SetFilter(std::vector<unsigned long>& aPidList);
	bool SetPidList(std::vector<unsigned long>& aPidList);

	static bool ExistPid(std::vector<unsigned long>& aPids, unsigned long aPid);
};
//-------------------------------------------------------------------------------

#endif /* EDEMUXPUMPTHREAD_H_ */
