/*
 * eFilePumpThread.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

//#include "uLogger.h"

//#include "uDemux.h"

#include <poll.h>
#include <stdio.h>

#include "ePreDefine.h"
#include "eFilePumpThread.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eFilePumpThread::eFilePumpThread(int aDeviceFd, std::string aFileName)
	: mDeviceFd(aDeviceFd), mFileName(aFileName), mTermFlag(false), uThread("FilePumpThread", TYPE_DETACHABLE)
{
}
//-------------------------------------------------------------------------------

eFilePumpThread::~eFilePumpThread()
{
}
//-------------------------------------------------------------------------------

void eFilePumpThread::Run()
{
	int rc = 0;
	unsigned char buffer[BUFFER_SIZE];
	FILE* mediafilefd = fopen(mFileName.c_str(), "rb");
	struct pollfd pollevt;

	pollevt.fd      = mDeviceFd;
	pollevt.events  = POLLOUT;
	pollevt.revents = 0;

	mTermFlag = true;
	while(mTermFlag) {
		rc = poll((struct pollfd*)&pollevt, 1, 1000);

		if (pollevt.revents & POLLOUT) {
			rc = fread(buffer, 1, BUFFER_SIZE, mediafilefd);
			if(!rc) break;
#ifdef DEBUG_LOG
			LOG("%d byte write.", rc);
#endif
			rc = write(mDeviceFd, buffer, rc);
		}
	}
	fclose(mediafilefd);
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

void eFilePumpThread::Terminate()
{
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

