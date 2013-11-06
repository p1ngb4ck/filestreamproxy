/*
 * eFilePumpThread.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ePreDefine.h"
#include "eFilePumpThread.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eFilePumpThread::eFilePumpThread(int aDeviceFd)
	: mDeviceFd(aDeviceFd), mFileFd(0), mTermFlag(false),
	  uThread("FilePumpThread", TYPE_DETACHABLE)
{
}
//-------------------------------------------------------------------------------

eFilePumpThread::~eFilePumpThread()
{
	Close();
}
//-------------------------------------------------------------------------------

bool eFilePumpThread::Open(std::string aFileName)
{
	mFileFd = open(aFileName.c_str(), O_RDONLY | O_LARGEFILE);
	if(mFileFd <= 0) {
		return false;
	}
	return true;
}
//-------------------------------------------------------------------------------

void eFilePumpThread::Close()
{
	if(mFileFd > 0) {
		close(mFileFd);
	}
	mFileFd = 0;
}
//-------------------------------------------------------------------------------

void eFilePumpThread::SeekOffset(int aOffset)
{
#ifdef DEBUG_LOG
	LOG("lseek to %d", aOffset);
#endif
	lseek(mFileFd, aOffset,  SEEK_SET);
}
//-------------------------------------------------------------------------------

void eFilePumpThread::Run()
{
	int rc = 0, wc = 0;
	unsigned char buffer[BUFFER_SIZE];

	struct pollfd pollevt;

	pollevt.fd     = mDeviceFd;
	pollevt.events = POLLOUT;

	mTermFlag = true;
	while(mTermFlag) {
		pollevt.revents = 0;
		rc = poll((struct pollfd*)&pollevt, 1, 1000);

		if (pollevt.revents & POLLOUT) {
			rc = read(mFileFd, buffer, BUFFER_SIZE);
			if(rc < 0) {
				break;
			}
			wc = write(mDeviceFd, buffer, rc);
			if(wc != rc) {
#ifdef DEBUG_LOG
				LOG("write fail.. rc[%d], wc[%d]", rc, wc);
#endif
				int read_len = rc;
				fd_set device_writefds;

				FD_ZERO(&device_writefds);
				FD_SET(mDeviceFd, &device_writefds);

				for(int i = wc; i < read_len; i += wc) {
					if (select(mDeviceFd + 1, 0, &device_writefds, 0, 0) < 0)
						break;
					wc = write(mDeviceFd, buffer + i, read_len - i);
				}
			}
		}
	}
	Close();
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

void eFilePumpThread::Terminate()
{
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

