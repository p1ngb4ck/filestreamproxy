/*
 * SessionMap.h
 *
 *  Created on: 2015. 12. 10.
 *      Author: oskwon
 */

#ifndef SESSION_MAP_H_
#define SESSION_MAP_H_

#include <vector>
#include <string>


#include "config.h"

#include "SharedMemory.h"
#include "3rdparty/trap.h"
//-------------------------------------------------------------------------------

typedef struct _session_t {
	int pid;
	char ip[16];
} SessionInfo;
//-------------------------------------------------------------------------------

class SessionMap : public SharedMemory<SessionInfo>
{
protected:
	int max_encoder_count;
	static SessionMap* instance;

	virtual ~SessionMap() {}

public:
	SessionMap() throw(trap);

	void dump(const char* aMessage);

	void cleanup();

	void erase(int aPid);
	int  add(std::string aIpAddr, int aPid);
	void remove(std::string aIpAddr);

	int  update(std::string aIpAddr, int aPid);
	bool terminated(std::vector<int>& aList, int aPid);
	int  already_exist(std::string aIpAddr);
	int  get_pid_by_ip(std::string aIpAddr);
	void post();

	static SessionMap* get() throw(trap)
	{
		if (instance == 0) {
			try {
				instance = new SessionMap();
			}
			catch (const trap &e) {
				throw(e);
			}
		}
		return instance;
	}

};
//----------------------------------------------------------------------

#endif /*SESSION_MAP_H_*/

