/*
 * main.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <vector>
#include <string>
#include <iterator>
#include <fstream>

#include "ePreDefine.h"
#include "eURIDecoder.h"
#include "eFilePumpThread.h"
#include "eNetworkPumpThread.h"

#ifdef DEBUG_LOG
FILE* fpLog = fopen("/tmp/filestreamproxy.log", "w");
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

using namespace std;
//-------------------------------------------------------------------------------

int gDeviceFd = 0;

char* ReadRequest(char* aRequest)
{
	return fgets(aRequest, MAX_LINE_LENGTH-1, stdin);
}
//-------------------------------------------------------------------------------

namespace eParser {
	int gVideoPid = 0, gAudioPid = 0;
	std::vector<string> Split(std::string aBuffer, char aDelimiter);
	void FileName(char* aRequest, char* aHttp, std::string& aOutData);
	bool MetaData(std::string aMediaFileName);
};
using namespace eParser;
//-------------------------------------------------------------------------------

/* GET /file?file=/home/kos/work/workspace/filestreamproxy/data/20131023%200630%20-%20FASHION%20TV%20-%20instant%20record.ts HTTP/1.0 */
int main(int argc, char** argv)
{
	char request[MAX_LINE_LENGTH] = {0};

	if (!ReadRequest(request)) {
		RETURN_ERR_400();
	}
#ifdef DEBUG_LOG
	LOG("%s", request);
#endif

	if (strncmp(request, "GET /", 5)) {
		RETURN_ERR_400();
	}

	char* http = strchr(request + 5, ' ');
	if (!http || strncmp(http, " HTTP/1.", 7)) {
		RETURN_ERR_400("Not support request (%s).", http);
	}

	std::string srcfilename = "";
	eParser::FileName(request, http, srcfilename);

	bool isSuccessMeta = eParser::MetaData(srcfilename);

#ifdef DEBUG_LOG
	LOG("meta parsing result : %d, video : %d, audio : %d", isSuccessMeta, eParser::gVideoPid, eParser::gAudioPid);
#endif

	gDeviceFd = open("/dev/bcm_enc0", O_RDWR);
	if(gDeviceFd < 0 ) {
		close(gDeviceFd);
		RETURN_ERR_502("Fail to opne device.");
	}

	if(isSuccessMeta) {
		if(ioctl(gDeviceFd, 1, eParser::gVideoPid)) {
			RETURN_ERR_502("Fail to set video pid");
		}
		if(ioctl(gDeviceFd, 2, eParser::gAudioPid)) {
			RETURN_ERR_502("Fail to set audio pid");
		}
	}

	eFilePumpThread filepump(gDeviceFd, srcfilename);
	filepump.Start();

	sleep(1);

	if(ioctl(gDeviceFd, 100, 0)) {
		RETURN_ERR_502("Fail to start transcoding.");
	}
	eNetworkPumpThread networkpump(gDeviceFd);
	networkpump.Start();

	networkpump.Join();
	filepump.Stop();
	filepump.Join();

	close(gDeviceFd);

#ifdef DEBUG_LOG
	fclose(fpLog);
#endif
	return 0;
}
//-------------------------------------------------------------------------------

std::vector<string> eParser::Split(std::string aBuffer, char aDelimiter)
{
	int b = 0, i = 0, l = aBuffer.length();
	std::vector<string> t;

	while (i++ < l) {
		if (aBuffer[i] == aDelimiter) {
			t.push_back(aBuffer.substr(b, i-b));
			b = i + 1;
			continue;
		}
		if (i == (l - 1)) {
			t.push_back(aBuffer.substr(b, l));
		}
	}
	return t;
}
//-------------------------------------------------------------------------------

void eParser::FileName(char* aRequest, char* aHttp, std::string& aOutData)
{
	char tmp[256] = {0};
	char* file = aRequest + 5;
	if (strncmp(file, "file?file=", strlen("file?file="))) {
		return;
	}
	strncpy(tmp, file+10, aHttp-file-10);
	aOutData = eURIDecoder().Decode(tmp);
}
//-------------------------------------------------------------------------------

namespace eCacheID {
	enum {
		cVPID = 0,
		cAPID,
		cTPID,
		cPCRPID,
		cAC3PID,
		cVTYPE,
		cACHANNEL,
		cAC3DELAY,
		cPCMDELAY,
		cSUBTITLE,
		cacheMax
	};
};
//-------------------------------------------------------------------------------

/* f:40,c:00007b,c:01008f,c:03007b */
bool eParser::MetaData(std::string aMediaFileName)
{
	std::string metafilename = aMediaFileName;
	metafilename += ".meta";

	std::ifstream ifs(metafilename.c_str());

	if (!ifs.is_open()) {
#ifdef DEBUG_LOG
		LOG("metadata is not exists..");
#endif
		return false;
	}

	size_t rc = 0, i = 0;
	char buffer[1024] = {0};
	while (!ifs.eof()) {
		ifs.getline(buffer, 1024);
		if (i++ == 7) {
#ifdef DEBUG_LOG
				LOG("%d [%s]", i, buffer);
#endif
			std::vector<string> tokens = eParser::Split(buffer, ',');
			if(tokens.size() < 3) {
#ifdef DEBUG_LOG
				LOG("pid count size error : %d", tokens.size());
#endif
				return false;
			}

			int setting_done = false;
			for (int ii = 0; ii < tokens.size(); ++ii) {
				std::string token = tokens[ii];
				if(token.at(0) != 'c') continue;

				int cache_id = atoi(token.substr(2,2).c_str());
#ifdef DEBUG_LOG
				LOG("token : %d [%s], chcke_id : [%d]", ii, token.c_str(), cache_id);
#endif
				switch(cache_id) {
					case(eCacheID::cVPID):
						gVideoPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("video pid : %d", gVideoPid);
#endif
						setting_done = (gVideoPid && gAudioPid) ? true : false;
						break;
					case(eCacheID::cAC3PID):
						gAudioPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("audio pid : %d", gAudioPid);
#endif
						break;
					case(eCacheID::cAPID):
						gAudioPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("audio pid : %d", gAudioPid);
#endif
						setting_done = (gVideoPid && gAudioPid) ? true : false;
						break;
				}
				if(setting_done) break;
			}
			break;
		}
	}
	ifs.close();
	return true;
}
//-------------------------------------------------------------------------------
