/*
 * eParser.h
 *
 *  Created on: 2013. 10. 29.
 *      Author: kos
 */

#ifndef EPARSER_H_
#define EPARSER_H_

#include <vector>
#include <string>

using namespace std;
//-------------------------------------------------------------------------------

class eParser
{
public:
	static bool Authorization(char* aAuthorization);
	static void FileName(char* aRequest, char* aHttp, std::string& aOutData);
	static bool MetaData(std::string aMediaFileName, int& aVideoPid, int& aAudioPid);
	static bool LiveStreamPid(std::string aData, std::vector<unsigned long>& aPidList,
						      int& aDemuxId, int& aVideoPid, int& aAudioPid, int& aPmtPid, std::string& aWWWAuth);
	static std::string ServiceRef(std::string aData, std::string aAuthorization);
};
//-------------------------------------------------------------------------------

#endif /* EPARSER_H_ */
