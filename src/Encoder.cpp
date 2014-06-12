/*
 * Encoder.cpp
 *
 *  Created on: 2014. 6. 12.
 *      Author: oskwon
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "Utils.h"
#include "Logger.h"
#include "Encoder.h"

using namespace std;
//----------------------------------------------------------------------

Encoder::Encoder()
{
	fd = -1;
	state = ENCODER_STAT_INIT;
}
//----------------------------------------------------------------------

Encoder::~Encoder()
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

bool Encoder::open(int encoder_id)
{
	std::string path = "/dev/bcm_enc" + ultostr(encoder_id);
	fd = ::open(path.c_str(), O_RDWR, 0);
	if (fd >= 0) {
		state = ENCODER_STAT_OPENED;
	}
	DEBUG("open encoder : %s, fd : %d", path.c_str(), fd);
	return (state == ENCODER_STAT_OPENED) ? true : false;
}
//----------------------------------------------------------------------

bool Encoder::retry_open(int encoder_id, int retry_count, int sleep_time)
{
	for (int i = 0; i < retry_count; ++i) {
		if (open(encoder_id)) {
			DEBUG("encoder-%d open success..", encoder_id);
			return true;
		}
		WARNING("encoder%d open fail, retry count : %d/%d", encoder_id, i, retry_count);
		sleep(sleep_time);
	}
	return false;
}

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
