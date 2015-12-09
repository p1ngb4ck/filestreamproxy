#include "SHMHandler.h"
#include <dirent.h>
#include "SharedMemory.h"

using namespace std;

//-------------------------------------------------------------------------------
SHMHandler::SHMHandler()
{
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

}
//-------------------------------------------------------------------------------
int SHMHandler::get_Pid(std::string aIP) throw(trap)
{
		if (Open() == false)
			throw(trap("session ctrl init fail."));
		for (int i = 0; i < max_encodr_count; i++) {
		if (mShmData[i].ip == aIP) {
			return mShmData[i].pid;
		}
	}
	return 0;
}
//-------------------------------------------------------------------------------
