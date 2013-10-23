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
	bool mTermFlag;
	int mDeviceFd;
	std::string mFileName;
protected:
	void Run();
	void Terminate();

public:
	eFilePumpThread(int aDeviceFd, std::string aFileName);
	~eFilePumpThread();
};
//-------------------------------------------------------------------------------

#endif /* EFILEPUMPTHREAD_H_ */
