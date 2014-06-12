/*
 * Encoder.h
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#ifndef ENCODER_H_
#define ENCODER_H_

class Encoder
{
private:
	int fd;

public:
	enum {
		IOCTL_SET_VPID   = 1,
		IOCTL_SET_APID   = 2,
		IOCTL_SET_PMTPID = 3,
		IOCTL_START_TRANSCODING = 100,
		IOCTL_STOP_TRANSCODING  = 200
	};

	enum {
		ENCODER_STAT_INIT = 0,
		ENCODER_STAT_OPENED,
		ENCODER_STAT_STARTED,
		ENCODER_STAT_STOPED,
	};

	int state;
protected:
	bool open(int encoder_id);

public:
	Encoder();
	virtual ~Encoder();

	int  get_fd();
	bool ioctl(int cmd, int value);
	bool retry_open(int encoder_id, int retry_count, int sleep_time);
};
//----------------------------------------------------------------------

#endif /* ENCODER_H_ */
