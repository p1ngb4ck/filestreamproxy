/*
 * eFilePumpThread.h
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#ifndef EFILEPUMPTHREAD_H_
#define EFILEPUMPTHREAD_H_

#include "uThread.h"
//-------------------------------------------------------------------------------

class eFilePumpThread : public uThread
{
private:
	int mFileFd;
	int mDeviceFd;
	bool mTermFlag;

protected:
	void Run();
	void Terminate();

public:
	eFilePumpThread(int aDeviceFd);
	~eFilePumpThread();

	bool Open(std::string aFileName);
	void Close();
	void SeekOffset(int aOffset);
};
//-------------------------------------------------------------------------------

#endif /* EFILEPUMPTHREAD_H_ */
