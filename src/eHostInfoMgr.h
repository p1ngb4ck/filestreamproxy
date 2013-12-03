/*
 * eHostInfoMgr.h
 *
 *  Created on: 2013. 11. 14.
 *      Author: kos
 */

#ifndef EHOSTINFOMGR_H_
#define EHOSTINFOMGR_H_

#include <vector>
#include <string>

#include "uPosixSharedMemory.h"
//-------------------------------------------------------------------------------

typedef struct host_info_t {
	int pid;
	char ip[16];
} eHostInfo;
//-------------------------------------------------------------------------------

class eHostInfoMgr : public uPosixSharedMemory<eHostInfo>
{
private:
	int mInfoCount;
protected:
	void Erease(int aPid);
	bool IsTerminated(std::vector<int>& aList, int aPid);
public:
	eHostInfoMgr(std::string aName, int aInfoCount);
	virtual ~eHostInfoMgr();
	bool Init();
	bool Register(std::string aIpAddr, int aPid);
	void Unregister(std::string aIpAddr);
	void Update(std::string aIpAddr, int aPid);
	int IsExist(std::string aIpAddr);
#ifdef DEBUG_LOG
	void Dump(const char* aMessage);
#endif

	static std::string GetHostAddr();
	std::vector<int> FindPid(std::string aProcName, int aMyPid);
};
//-------------------------------------------------------------------------------

#endif /* EHOSTINFOMGR_H_ */
