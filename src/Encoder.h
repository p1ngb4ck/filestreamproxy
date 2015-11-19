/*
 * Encoder.h
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include "config.h"

#include <string>

#include "3rdparty/trap.h"

#include "Mutex.h"
#include "SharedMemory.h"
//----------------------------------------------------------------------

typedef struct _session_t {
	int pid;
	char ip[16];
} Session;
//----------------------------------------------------------------------

class Encoder : public SharedMemory<Session>
{
private:
	int fd;
	Mutex encoder_mutex;

public:
	enum {
#ifdef HAVE_EXT_PID
		IOCTL_SET_VPID	 = 11,
		IOCTL_SET_APID	 = 12,
		IOCTL_SET_PMTPID = 13,
#else
		IOCTL_SET_VPID	 = 1,
		IOCTL_SET_APID	 = 2,
		IOCTL_SET_PMTPID = 3,
#endif

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
	int encoder_id;
	int max_encodr_count;

protected:
	void session_dump(const char* aMessage);

	void session_erase(int aPid);
	int  session_register(std::string aIpAddr, int aPid);
	void session_unregister(std::string aIpAddr);

	int  session_update(std::string aIpAddr, int aPid);
	bool session_terminated(std::vector<int>& aList, int aPid);
	int  session_already_exist(std::string aIpAddr);

protected:
	bool encoder_open();

public:
	Encoder() throw(trap);
	virtual ~Encoder();

	int  get_fd();
	bool ioctl(int cmd, int value);
	bool retry_open(int retry_count, int sleep_time);

	void encoder_close();
};
//----------------------------------------------------------------------

#endif /* ENCODER_H_ */
