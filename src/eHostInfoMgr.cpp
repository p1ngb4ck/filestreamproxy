/*
 * eHostInfoMgr.cpp
 *
 *  Created on: 2013. 11. 14.
 *      Author: kos
 */

#include <sstream>
#include <fstream>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "eHostInfoMgr.h"
#include "ePreDefine.h"

#define KILLPROC(PID) { kill(PID, SIGINT); }

using namespace std;
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif
//-------------------------------------------------------------------------------

eHostInfoMgr::eHostInfoMgr(std::string aName, int aInfoCount)
	: mInfoCount(aInfoCount)
{
	mSemId = 0;
	mSemName = "/" + aName + "_sem";

	mShmFd = 0;
	mShmSize = sizeof(eHostInfo) * aInfoCount;
	mShmName = "/" + aName + "_shm";

	mShmData = 0;
}
//-------------------------------------------------------------------------------

eHostInfoMgr::~eHostInfoMgr()
{
}
//-------------------------------------------------------------------------------

bool eHostInfoMgr::IsTerminated(std::vector<int>& aList, int aPid)
{
	for (int i = 0; i < aList.size(); ++i) {
		if (aList[i] == aPid) {
			return false;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------

bool eHostInfoMgr::Init()
{
	if (Open() == false) {
		return false;
	}
#ifdef DEBUG_LOG
	LOG("shm-info : fd [%d], name [%s], size [%d], data [%p]", mShmFd, mShmName.c_str(), mShmSize, mShmData);
	LOG("sem-info : id [%p], name [%s]", mSemId, mSemName.c_str());
#endif
	std::vector<int> pidlist = FindPid("transtreamproxy", 0);
#ifdef DEBUG_LOG
	Dump("before init.");
#endif
	Wait();
	for (int i = 0; i < mInfoCount; i++) {
		if (mShmData[i].pid != 0) {
			int pid = mShmData[i].pid;
			if(IsTerminated(pidlist, pid)) {
				Erease(pid);
			}
		}
	}
	Post();
	return true;
}
//-------------------------------------------------------------------------------

bool eHostInfoMgr::Register(std::string aIpAddr, int aPid)
{
	bool result = false;

	Wait();
	for (int i = 0; i < mInfoCount; i++) {
		if (mShmData[i].pid == 0) {
			result = true;
			mShmData[i].pid = aPid;
			strcpy(mShmData[i].ip, aIpAddr.c_str());
			break;
		}
	}
	Post();
#ifdef DEBUG_LOG
	Dump("after register.");
#endif
	return result;
}
//-------------------------------------------------------------------------------

void eHostInfoMgr::Unregister(std::string aIpAddr)
{
	Wait();
	for (int i = 0; i < mInfoCount; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
			break;
		}
	}
	Post();
#ifdef DEBUG_LOG
	Dump("after unregister.");
#endif
}
//-------------------------------------------------------------------------------

void eHostInfoMgr::Erease(int aPid)
{
	for (int i = 0; i < mInfoCount; i++) {
		if (mShmData[i].pid == aPid) {
#ifdef DEBUG_LOG
			LOG("erase.. %s : %d", mShmData[i].ip, mShmData[i].pid);
#endif
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
			break;
		}
	}
}
//-------------------------------------------------------------------------------

void eHostInfoMgr::Update(std::string aIpAddr, int aPid)
{
#ifdef DEBUG_LOG
	Dump("before update.");
#endif
	Wait();
	for (int i = 0; i < mInfoCount; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			KILLPROC(mShmData[i].pid);
			memset(mShmData[i].ip, 0, 16);
			mShmData[i].pid = 0;
		}
	}
	Post();
	Register(aIpAddr, aPid);
}
//-------------------------------------------------------------------------------

int eHostInfoMgr::IsExist(std::string aIpAddr)
{
	int existCount = 0;

	Wait();
	for (int i = 0; i < mInfoCount; i++) {
		if (strcmp(mShmData[i].ip, aIpAddr.c_str()) == 0) {
			existCount++;
		}
	}
	Post();

	return existCount;
}
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
void eHostInfoMgr::Dump(const char* aMessage)
{
	LOG(" >> %s", aMessage);
	LOG("-------- [ DUMP HOST INFO ] ---------");
	for (int i = 0; i < mInfoCount; i++) {
		LOG("%d : ip [%s], pid [%d]", i,  mShmData[i].ip, mShmData[i].pid);
	}
	LOG("-------------------------------------");
	Post();
}
#endif
//-------------------------------------------------------------------------------

std::string eHostInfoMgr::GetHostAddr()
{
	std::stringstream ss;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    getpeername(0, (struct sockaddr*)&addr, &addrlen);
    ss << inet_ntoa(addr.sin_addr);// << ":" << addr.sin_port;

    return ss.str();
}
//-------------------------------------------------------------------------------

std::vector<int> eHostInfoMgr::FindPid(std::string aProcName, int aMyPid)
{
	std::vector<int> pidlist;
	char cmdlinepath[256] = {0};
	DIR* d = opendir("/proc");
	if (d != 0) {
		struct dirent* de;
		while ((de = readdir(d)) != 0) {
			int pid = atoi(de->d_name);
			if (pid > 0) {
				sprintf(cmdlinepath, "/proc/%s/cmdline", de->d_name);

				std::string cmdline;
				std::ifstream cmdlinefile(cmdlinepath);
				std::getline(cmdlinefile, cmdline);
				if (!cmdline.empty()) {
					size_t pos = cmdline.find('\0');
					if (pos != string::npos)
					cmdline = cmdline.substr(0, pos);
					pos = cmdline.rfind('/');
					if (pos != string::npos)
					cmdline = cmdline.substr(pos + 1);
					if ((aProcName == cmdline) && ((aMyPid != pid) || (aMyPid == 0))) {
						pidlist.push_back(pid);
					}
				}
			}
		}
		closedir(d);
	}
	return pidlist;
}
//-------------------------------------------------------------------------------
