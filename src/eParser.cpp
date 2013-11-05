/*
 * eParser.cpp
 *
 *  Created on: 2013. 10. 29.
 *      Author: kos
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <vector>
#include <string>
#include <fstream>
#include <iterator>

#include "eParser.h"
#include "ePreDefine.h"
#include "eURIDecoder.h"
#include "uStringTool.h"
#include "eDemuxPumpThread.h"

using namespace std;
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif
//-------------------------------------------------------------------------------

bool eParser::Authorization(char* aAuthorization)
{
	char request[MAX_LINE_LENGTH] = {0};
	while(true) {
		int nullidx = 0;

		request[0] = 0;
		if(!ReadRequest(request)) {
			return 1;
		}
		if(!strncasecmp(request, "Authorization: ", 15)) {
			strcpy(aAuthorization, request);
		}
		nullidx = strlen(request);
		if(nullidx) {
			nullidx = (request[nullidx - 2] == '\r') ? nullidx - 2 : nullidx -1;
			request[nullidx] = 0;
		}
#ifdef DEBUG_LOG
		LOG("[%s](%d)", request, nullidx);
#endif
		if(!request[0]) break;
	}
	return 0;
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
bool eParser::MetaData(std::string aMediaFileName, int& aVideoPid, int& aAudioPid)
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
			std::vector<string> tokens;
			uStringTool::Split(buffer, ',', tokens);
			if(tokens.size() < 3) {
#ifdef DEBUG_LOG
				LOG("pid count size error : %d", tokens.size());
#endif
				return false;
			}

			int setting_done = false;
			for (int ii = 0; ii < tokens.size(); ++ii) {
				std::string token = tokens[ii];
				if(token.length() <= 0) continue;
				if(token.at(0) != 'c') continue;

				int cache_id = atoi(token.substr(2,2).c_str());
#ifdef DEBUG_LOG
				LOG("token : %d [%s], chcke_id : [%d]", ii, token.c_str(), cache_id);
#endif
				switch(cache_id) {
					case(eCacheID::cVPID):
						aVideoPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("video pid : %d", aVideoPid);
#endif
						setting_done = (aVideoPid && aAudioPid) ? true : false;
						break;
					case(eCacheID::cAC3PID):
						aAudioPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("audio pid : %d", aAudioPid);
#endif
						break;
					case(eCacheID::cAPID):
						aAudioPid = strtol(token.substr(4,4).c_str(), NULL, 16);
#ifdef DEBUG_LOG
						LOG("audio pid : %d", aAudioPid);
#endif
						setting_done = (aVideoPid && aAudioPid) ? true : false;
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

unsigned long StripPid(std::string aData, std::string& aOutType)
{
	std::vector<std::string> token;
	if(uStringTool::Split(aData, ':', token)) {
		aOutType = token[1].c_str();
		return strtoul(token[0].c_str(), 0, 0x10);
	}
	return -1;
}
//-------------------------------------------------------------------------------

bool eParser::LiveStreamPid(std::string aData, std::vector<unsigned long>& aPidList,
						    int& aDemuxId, int& aVideoPid, int& aAudioPid, int& aPmtPid, std::string& aWWWAuth)
{
	int state = 0;
	int responsecode = 0;

	std::string reason = "";

	std::vector<std::string> tokens;
	if(uStringTool::Split(aData, '\n', tokens)) {
		int tokenlen = tokens.size();
		for(int i = 0; i < tokenlen; i++) {
			if(!aVideoPid || !aAudioPid || !aPmtPid) {
				aVideoPid = aAudioPid = aPmtPid = 0;
			}
			std::string line = uStringTool::Trim(tokens[i]);
#ifdef DEBUG_LOG
			LOG("[%d] [%s]", state, line.c_str());
#endif
			switch (state) {
			case 0:
				if(strncmp(line.c_str(), "HTTP/1.", 7) || line.length() < 9) {
					return false;
				}
				responsecode = atoi(line.c_str() + 9);
				if(responsecode != 200) {
					reason = line.c_str() + 9;
				}
				++state;
				break;
			case 1:
				if(line.length()) {
					if (!strncasecmp(line.c_str(), "WWW-Authenticate: ", 18)) {
						aWWWAuth = line;
					}
				} else {
					if(responsecode == 200) {
						++state;
					}
				}
				break;
			case 2:
			case 3:
				if(line.length() > 0 && line.at(0) == '+') {
					/*+0:0:pat,17d4:pmt,17de:video,17e8:audio,17e9:audio,17eb:audio,17ea:audio,17f3:subtitle,17de:pcr,17f2:text*/
					aDemuxId = atoi(line.substr(1,1).c_str());

					std::vector<std::string> pidtokens;
					if(uStringTool::Split(line.c_str()+3, ',', pidtokens)) {
						for(int ii = 0; ii < pidtokens.size(); ++ii) {
							std::string pidtype = "";
							unsigned long pid = StripPid(pidtokens[ii], pidtype);

							if(pid == -1) {
								continue;
							}
							if(!aVideoPid || !aAudioPid || !aPmtPid) {
								if(strcmp(pidtype.c_str(), "video") == 0) {
									aVideoPid = pid;
								} else if(strcmp(pidtype.c_str(), "audio") == 0) {
									aAudioPid = pid;
								} else if(strcmp(pidtype.c_str(), "pmt") == 0) {
									aPmtPid = pid;
								}
							}
							if(!eDemuxPumpThread::ExistPid(aPidList, pid)) {
								aPidList.push_back(pid);
							}
#ifdef DEBUG_LOG
							LOG("pid : %s [%04X]", pidtokens[ii].c_str(), pid);
#endif
						}
					}
				}
				break;
			}
		}
	}
#ifdef DEBUG_LOG
	LOG("response code : %d, wwwathenticate : [%s]", responsecode, aWWWAuth.c_str());
#endif
	return true;
}
//-------------------------------------------------------------------------------

/*GET /1:0:19:2B66:3F3:1:C00000:0:0:0: HTTP/1.1*/
std::string eParser::ServiceRef(std::string aData, std::string aAuthorization)
{
	std::string request = "";
	std::vector<std::string> tokens;
	if(uStringTool::Split(aData, ' ', tokens)) {
		request += "GET /web/stream?StreamService=";
		request += tokens[0];
		request += " HTTP/1.0\r\n";
		request += aAuthorization;
		request += "\r\n";
	}
	return request;
}
//-------------------------------------------------------------------------------


