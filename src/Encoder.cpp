/*
 * Encoder.cpp
 *
 *  Created on: 2014. 6. 12.
 *      Author: oskwon
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "Util.h"
#include "Logger.h"
#include "Encoder.h"

#include "SessionMap.h"

bool terminated();

using namespace std;
//----------------------------------------------------------------------

Encoder::Encoder() throw(trap)
{
	SingleLock lock(&encoder_mutex);

	encoder_id = fd = -1;
	state = ENCODER_STAT_INIT;

	try {
		SessionMap* session_map = SessionMap::get();
		if (session_map == 0) {
			throw(trap("create session map fail."));
		}

		session_map->dump("before init.");
		session_map->cleanup();

		int mypid = getpid();
		std::string ipaddr = Util::host_addr();
		if (session_map->already_exist(ipaddr) > 0) {
			encoder_id = session_map->update(ipaddr, mypid);
		}
		else {
			encoder_id = session_map->add(ipaddr, mypid);
		}
		DEBUG("encoder_device_id : %d", encoder_id);
	}
	catch (const trap &e) {
		throw(e);
	}
}
//----------------------------------------------------------------------

Encoder::~Encoder()
{
	SingleLock lock(&encoder_mutex);

	try {
		SessionMap* session_map = SessionMap::get();
		if (session_map) {
			session_map->post();
		}
	}
	catch (const trap &e) {
	}
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
	errno = 0;
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
	for (int i = 0; i < retry_count*10; ++i) {
		if (terminated()) {
			break;
		}
		if (encoder_open()) {
			DEBUG("encoder-%d open success..", encoder_id);
			return true;
		}
		WARNING("encoder%d open fail, retry count : %d/%d", encoder_id, i, retry_count);
		usleep(sleep_time*100*1000); /*wait sleep_time ms*/
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

