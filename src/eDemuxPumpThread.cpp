/*
 * eDemuxPumpThread.cpp
 *
 *  Created on: 2013. 10. 29.
 *      Author: kos
 */

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>

#include "ePreDefine.h"
#include "eDemuxPumpThread.h"

using namespace std;

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eDemuxPumpThread::eDemuxPumpThread()
	: mTermFlag(0), mDemuxFd(0), mDeviceFd(0), mState(eDemuxState::stNotOpened),
#ifdef NORMAL_STREAMPROXY
	  mErrMessage(""), uThread("eDemuxPumpThread")
#else
	  mErrMessage(""), uThread("eDemuxPumpThread", TYPE_DETACHABLE)
#endif
{
}
//-------------------------------------------------------------------------------

eDemuxPumpThread::~eDemuxPumpThread()
{
	Close();
}
//-------------------------------------------------------------------------------

void eDemuxPumpThread::Run()
{
	if(!mState) {
		mTermFlag = false;
		return;
	}
	int rc = 0, wc = 0;
	static unsigned char buffer[BUFFER_SIZE];

	struct pollfd pollevt;
	pollevt.fd      = mDeviceFd;
	pollevt.events  = POLLOUT;

	fd_set demux_readfds;
#ifdef NORMAL_STREAMPROXY
	const char *c = "\
HTTP/1.0 200 OK\r\n\
Connection: close\r\n\
Content-Type: video/mpeg\r\n\
Server: stream_enigma2\r\n\
\r\n";
	wc = write(1, c, strlen(c));
#endif

	mTermFlag = true;
#ifdef DEBUG_LOG
	LOG("demux pump start.");
#endif
	while(mTermFlag) {
		FD_ZERO(&demux_readfds);
		FD_SET(mDemuxFd, &demux_readfds);
		if (select(mDemuxFd + 1, &demux_readfds, 0, 0, 0) < 0) {
			break;
		}
		if(FD_ISSET(mDemuxFd, &demux_readfds)) {
			rc = read(mDemuxFd, buffer, BUFFER_SIZE);
			if(rc < 0) {
#ifdef DEBUG_LOG
				LOG("read fail : %d", errno);
#endif
				if(errno == EOVERFLOW) {
					continue;
				}
				break;
			}
#ifdef NORMAL_STREAMPROXY
			wc = write(1, buffer, rc);
#else
			wc = write(mDeviceFd, buffer, rc);
#endif
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
#ifdef DEBUG_LOG
	LOG("demux pump stoped.");
#endif
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

void eDemuxPumpThread::Terminate()
{
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

bool eDemuxPumpThread::Open(int aDemuxId)
{
	char demuxpath[32] = {0};
	sprintf(demuxpath, "/dev/dvb/adapter0/demux%d", aDemuxId);

	mDemuxFd = open(demuxpath, O_RDWR | O_NONBLOCK);
	if (mDemuxFd < 0) {
		mErrMessage = "DEMUX OPEN FAILED";
		mDemuxFd = 0;
#ifdef DEBUG_LOG
		LOG("demux open failed (%s).", demuxpath);
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("demux open succeed : %s (%d).", demuxpath, mDemuxFd);
#endif

	mState = eDemuxState::stOpened;
	return true;
}
//-------------------------------------------------------------------------------

void eDemuxPumpThread::Close()
{
	mState = eDemuxState::stNotOpened;
	if(mDemuxFd > 0) {
		close(mDemuxFd);
	}
	mDemuxFd = 0;
}
//-------------------------------------------------------------------------------


bool eDemuxPumpThread::ExistPid(std::vector<unsigned long>& aPids, unsigned long aPid)
{
	for(int i = 0; i < aPids.size(); ++i) {
		if(aPids[i] == aPid) {
			return true;
		}
	}
	return false;
}
//-------------------------------------------------------------------------------

bool eDemuxPumpThread::SetFilter(std::vector<unsigned long>& aPidList)
{
	if(!mDemuxFd) {
		mErrMessage = "DMX_SET_PES_FILTER FAILED (Demux is not opened).";
		return false;
	}

	struct dmx_pes_filter_params filter;
	ioctl(mDemuxFd, DMX_SET_BUFFER_SIZE, 1024*1024);

	filter.pid = aPidList[0];
	filter.input = DMX_IN_FRONTEND;
#if DVB_API_VERSION > 3
	filter.output = DMX_OUT_TSDEMUX_TAP;
	filter.pes_type = DMX_PES_OTHER;
#else
	filter.output = DMX_OUT_TAP;
	filter.pes_type = DMX_TAP_TS;
#endif
	filter.flags = DMX_IMMEDIATE_START;

	if (ioctl(mDemuxFd, DMX_SET_PES_FILTER, &filter) < 0) {
		mErrMessage = "DEMUX PES FILTER SET FAILED";
#ifdef DEBUG_LOG
		LOG("demux filter setting failed.");
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("demux filter setting ok.");
#endif

	mState = eDemuxState::stSetedFilter;
	return true;
}
//-------------------------------------------------------------------------------

bool eDemuxPumpThread::SetPidList(std::vector<unsigned long>& aPidList)
{
	if(!mDemuxFd) {
		mErrMessage = "DMX_ADD_PID FAILED (Demux is not opened).";
		return false;
	}

	int rc = 0, i = 0;
	int pidsize = aPidList.size();


	for(i = 0; i < pidsize; ++i) {
		uint16_t pid = aPidList[i];

		if(eDemuxPumpThread::ExistPid(mPidList, pid)) {
			continue;
		}

#if DVB_API_VERSION > 3
		rc = ioctl(mDemuxFd, DMX_ADD_PID, &pid);
#else
		rc = ioctl(mDemuxFd, DMX_ADD_PID, pid);
#endif
#ifdef DEBUG_LOG
		LOG("add [%x]!!!", pid);
#endif
		if(rc < 0) {
			mErrMessage = "DMX_ADD_PID FAILED.";
#ifdef DEBUG_LOG
			LOG("demux add PID failed.");
#endif
			return false;
		}
	}

	pidsize = mPidList.size();
	for(i = 0; i < pidsize; ++i) {
		uint16_t pid = mPidList[i];

		if(eDemuxPumpThread::ExistPid(aPidList, pid)) {
			continue;
		}

		if(i == 4) {
			break;
		}

#if DVB_API_VERSION > 3
		rc = ioctl(mDemuxFd, DMX_REMOVE_PID, &pid);
#else
		rc = ioctl(mDemuxFd, DMX_REMOVE_PID, pid);
#endif
#ifdef DEBUG_LOG
		LOG("remove [%x]!!!", pid);
#endif
		if(rc < 0) {
			mErrMessage = "DMX_REMOVE_PID FAILED.";
#ifdef DEBUG_LOG
			LOG("demux remove PID failed.");
#endif
			return false;
		}
	}
#ifdef DEBUG_LOG
	LOG("demux setting PID ok.");
#endif
	mPidList = aPidList;
	return true;
}
//-------------------------------------------------------------------------------

