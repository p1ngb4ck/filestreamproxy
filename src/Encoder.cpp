/*
 * Encoder.cpp
 *
 *  Created on: 2014. 6. 12.
 *      Author: oskwon
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "Util.h"
#include "Logger.h"
#include "Encoder.h"

using namespace std;
//----------------------------------------------------------------------

Encoder::Encoder() throw(trap)
{
	encoder_id = fd = -1;
	max_encodr_count = state = ENCODER_STAT_INIT;

	DIR* d = opendir("/dev");
	if (d != 0) {
		struct dirent* de;
		while ((de = readdir(d)) != 0) {
			if (strncmp("bcm_enc", de->d_name, 7) == 0) {
				max_encodr_count++;
			}
		}
		closedir(d);
	}

	mSemId = 0;
	mShmFd = 0;
	mShmData = 0;

	mSemName = "/tsp_session_sem";
	mShmName = "/tsp_session_shm";
	mShmSize = sizeof(Session) * max_encodr_count;

	if (Open() == false)
		throw(trap("session ctrl init fail."));
	DEBUG("shm-info : fd [%d], name [%s], size [%d], data [%p]", mShmFd, mShmName.c_str(), mShmSize, mShmData);
	DEBUG("sem-info : id [%p], name [%s]", mSemId, mSemName.c_str());

	std::vector<int> pidlist = Util::find_process_by_name("transtreamproxy", 0);

	session_dump("before init.");

	Wait();
	for (int i = 0; i < max_encodr_count; i++) {
		if (mShmData[i].pid != 0) {
			int pid = mShmData[i].pid;
			if(session_terminated(pidlist, pid)) {
				session_erase(pid);
			}
		}
	}
	Post();

	int mypid = getpid();
	std::string ipaddr = Util::host_addr();
	if (session_already_exist(ipaddr) > 0) {
		encoder_id = session_update(ipaddr, mypid);
	}
	else {
		encoder_id = session_register(ipaddr, mypid);
	}
	DEBUG("encoder_device_id : %d", encoder_id);
}
//----------------------------------------------------------------------

Encoder::~Encoder()
{
	Post();
	encoder_close();
}
//----------------------------------------------------------------------

void Encoder::encoder_close()
{
	if (fd != -1) {
		if (state == ENCODER_STAT_STARTED) {
			DEBUG("stop transcoding..");
			ioctl(IOCTL_STOP_TRANSCODING, 0);
		}
		close(fd);
		fd = -1;
	}
}
//----------------------------------------------------------------------

bool Encoder::encoder_open()
{
	std::string path = "/dev/bcm_enc" + Util::ultostr(encoder_id);
	fd = ::open(path.c_str(), O_RDWR, 0);
	if (fd >= 0) {
		state = ENCODER_STAT_OPENED;
	}
	DEBUG("open encoder : %s, fd : %d", path.c_str(), fd);
	return (state == ENCODER_STAT_OPENED) ? true : false;
}
//----------------------------------------------------------------------

bool Encoder::retry_open(int retry_count, int sleep_time)
{
	for (int i = 0; i < retry_count; ++i) {
		if (encoder_open()) {
			DEBUG("encoder-%d open success..", encoder_id);
			return true;
		}
		WARNING("encoder%d open fail, retry count : %d/%d", encoder_id, i, retry_count);
		sleep(sleep_time);
	}
	ERROR("encoder open fail : %s (%d)", strerror(errno), errno);
	return false;
}
//----------------------------------------------------------------------

bool Encoder::ioctl(int cmd, int value)
{
	int result = ::ioctl(fd, cmd, value);
	DEBUG("ioctl command : %d -> %x, result : %d", cmd, value, result);

	if (result == 0) {
		switch (cmd) {
		case IOCTL_START_TRANSCODING: state = ENCODER_STAT_STARTED; break;
		case IOCTL_STOP_TRANSCODING:  state = ENCODER_STAT_STOPED;  break;
		}
	}

	return (result == 0) ? true : false;
}
//----------------------------------------------------------------------

int Encoder::get_fd()
{
	return fd;
}
//----------------------------------------------------------------------

void Encoder::session_dump(const char* aMessage)
{
	if (Logger::instance()->get_level() >= Logger::INFO) {
		DUMMY(" >> %s", aMessage);
		DUMMY("-------- [ DUMP HOST INFO ] ---------");
		for (int i = 0; i < max_encodr_count; i++) {
			DUMMY("%d : ip [%s], pid [%d]", i,  mShmData[i].ip, mShmData[i].pid);
		}
		DUMMY("-------------------------------------");
	}
}
//----------------------------------------------------------------------

bool Encoder::session_terminated(std::vector<int>& aList, int aPid)
{
	for (int i = 0; i < aList.size(); ++i) {
		if (aList[i] == aPid) {
			return false;
		}
	}
	return true;
}
//----------------------------------------------------------------------

int Encoder::session_register(std::string aIpAddr, int aPid)
{
	int i = 0;
	bool result = false;

	Wait();
	for (; i < max_encodr_count; i++) {
		if (mShmData[i].pid == 0) {
			result = true;
			mShmData[i].pid = aPid;
			strcpy(mShmData[i].ip, aIpAddr.c_str());
			break;
		}
	}
	Post();
	session_dump("after register.");

	return result ? i : -1;
}
//----------------------------------------------------------------------

void Encoder::session_unregister(std::string aIpAddr)
{
	Wait();
	for (int i = 0; i < max_encodr_count; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
			break;
		}
	}
	Post();
	session_dump("after unregister.");
}
//----------------------------------------------------------------------

void Encoder::session_erase(int aPid)
{
	for (int i = 0; i < max_encodr_count; i++) {
		if (mShmData[i].pid == aPid) {
			DEBUG("erase.. %s : %d", mShmData[i].ip, mShmData[i].pid);
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
			break;
		}
	}
}
//----------------------------------------------------------------------

int Encoder::session_update(std::string aIpAddr, int aPid)
{
	int i = 0;
	bool result = false;

	session_dump("before update.");
	Wait();
	for (; i < max_encodr_count; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			result = true;
			Util::kill_process(mShmData[i].pid);
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
			break;
		}
	}
	Post();
	session_register(aIpAddr, aPid);
	return result ? i : -1;
}
//----------------------------------------------------------------------

int Encoder::session_already_exist(std::string aIpAddr)
{
	int existCount = 0;
	Wait();
	for (int i = 0; i < max_encodr_count; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			existCount++;
		}
	}
	Post();
	return existCount;
}
//----------------------------------------------------------------------
