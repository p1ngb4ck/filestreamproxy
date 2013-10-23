/*
 * eDemuxPumpThread.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

//#include "uLogger.h"
//#include "uDemux.h"

#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "ePreDefine.h"
#include "eNetworkPumpThread.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eNetworkPumpThread::eNetworkPumpThread(int aDeviceFd)
	: mTermFlag(0), mDeviceFd(aDeviceFd), uThread("eNetworkPumpThread")
{
}
//-------------------------------------------------------------------------------

eNetworkPumpThread::~eNetworkPumpThread()
{
}
//-------------------------------------------------------------------------------

void eNetworkPumpThread::Run()
{
	int rc = 0, wc = 0;
	unsigned char buffer[BUFFER_SIZE];
	struct pollfd pollevt;

	pollevt.fd      = mDeviceFd;
	pollevt.events  = POLLIN | POLLHUP;
	pollevt.revents = 0;

	mTermFlag = true;

	const char *c = "\
HTTP/1.0 200 OK\r\n\
Connection: close\r\n\
Content-Type: video/mpeg\r\n\
Server: stream_enigma2\r\n\
\r\n";

	write(1, c, strlen(c));

	while(mTermFlag) {
		rc = poll((struct pollfd*)&pollevt, 1, 1000);
		if (pollevt.revents & POLLIN) {
			rc = read(mDeviceFd, buffer, BUFFER_SIZE);

			if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == EOVERFLOW) {
#ifdef DEBUG_LOG
				LOG("(retry... errno : %d)", errno);
#endif
				continue;
			}
#ifdef DEBUG_LOG
			LOG("%d byte read", rc);
#endif
			wc = write(1, buffer, rc);
#ifdef DEBUG_LOG
			LOG("%d byte write", wc);
#endif
		} else if (pollevt.revents & POLLHUP) {
			ioctl(mDeviceFd, 200, 0);
			break;
		}
	}
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

void eNetworkPumpThread::Terminate()
{
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

