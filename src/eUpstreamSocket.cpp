/*
 * eUpstreamPumpThread.cpp
 *
 *  Created on: 2013. 10. 30.
 *      Author: kos
 */

#include "eUpstreamSocket.h"
#include "ePreDefine.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eUpstreamSocket::eUpstreamSocket()
	: mSockFd(-1)
{
}
//-------------------------------------------------------------------------------

eUpstreamSocket::~eUpstreamSocket()
{
}
//-------------------------------------------------------------------------------

bool eUpstreamSocket::Connect()
{
	mSockFd = socket(PF_INET, SOCK_STREAM, 0);

	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_port = htons(80);
	mSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(mSockFd, (struct sockaddr*)&mSockAddr, sizeof(struct sockaddr_in))) {
		return false;
	}
	return true;
}
//-------------------------------------------------------------------------------

int eUpstreamSocket::Request(std::string aSendData, std::string& aRecvData)
{
#ifdef DEBUG_LOG
	LOG("Send : %s", aSendData.c_str());
#endif
	if(write(mSockFd, aSendData.c_str(), aSendData.length()) > 0) {
		struct pollfd pollevt;
		pollevt.fd      = mSockFd;
		pollevt.events  = POLLIN | POLLERR;
		pollevt.revents = 0;

		while(true) {
			char buffer[4096] = {0};
			if(read(mSockFd, buffer, 4096) <= 0) {
				break;
			}
	#ifdef DEBUG_LOG
			LOG("Buffer : %s", buffer);
	#endif
			aRecvData += buffer;
			if (poll((struct pollfd*)&pollevt, 1, 1000) == 0) {
				break;
			}
		}
		return aRecvData.length();
	}
	return -1;
}
//-------------------------------------------------------------------------------


