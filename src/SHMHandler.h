/*
 * Encoder.h
 *
 *  Created on: 2014. 6. 10.
 *      Author: oskwon
 */

#ifndef SHMHANDLER_H_
#define SHMHANDLER_H_

#include "config.h"
#include "SharedMemory.h"
#include "3rdparty/trap.h"

#include <string.h>
#include <stdio.h>

class SHMHandler : public SharedMemory<Session>
{
public:
	int max_encodr_count;
	SHMHandler();
	~SHMHandler() {}

	int get_Pid(std::string) throw(trap);

	static int GetPidByIp(std::string ip)
	{
		int pid = 0;
		SHMHandler* hwd = new SHMHandler();
		if (hwd != 0) {
			try {
				pid = hwd->get_Pid(ip);
			}
			catch (const http_trap &e) { pid = 0; }
			delete hwd;
		}
		return pid;
	}

};
//----------------------------------------------------------------------

#endif

